""" Frida IL2CPP Bridge runner + error capture """
import frida
import sys
import os

BRIDGE_PATH = r"C:\frida-il2cpp\dist\index.js"
AGENT_PATH = r"C:\frida-il2cpp\cli\src\dump\agent.js"
OUTPUT_PATH = r"E:\github\rust\frida_dump.txt"

with open(BRIDGE_PATH, "r") as f:
    bridge = f.read()
with open(AGENT_PATH, "r") as f:
    agent = f.read()

# Wrap agent with error catching
code = bridge + "\n\n"
code += """
try {
    const t0 = new Date();
    setTimeout(() => {
        try {
            Il2Cpp.perform(() => {
                send({ action: "init", elapsed_ms: new Date() - t0, application: Il2Cpp.application, unityVersion: Il2Cpp.unityVersion });
                const t1 = new Date();
                Il2Cpp.domain.assemblies.forEach(assembly => {
                    send({ type: "assembly", handle: assembly.handle, name: assembly.name, class_count: assembly.image.classCount });
                    assembly.image.classes.forEach((klass, i) =>
                        send({
                            type: "class", nth: i + 1, assembly_handle: assembly.handle, handle: klass.handle,
                            namespace: klass.namespace, name: klass.name,
                            kind: klass.isEnum ? "enum" : klass.isStruct ? "struct" : klass.isInterface ? "interface" : "class",
                            parent_type_name: klass.parent?.type.name,
                            fields: klass.fields.map(field => ({
                                name: field.name, type_name: field.type.name,
                                is_static: field.isStatic, is_thread_static: field.isThreadStatic, is_literal: field.isLiteral,
                                offset: field.isThreadStatic || field.isLiteral ? undefined : field.offset
                            }))
                        })
                    );
                });
                send({ action: "exit", elapsed_ms: new Date() - t1 });
            }).catch(e => send({ action: "error", error: e.message + "\\n" + (e.stack || "") }));
        } catch(e) {
            send({ action: "error", error: "setTimeout error: " + e.message });
        }
    });
} catch(e) {
    send({ action: "fatal", error: "Top-level error: " + (e.message || String(e)) + "\\n" + (e.stack || "") });
}
"""

out = open(OUTPUT_PATH, "w", encoding="utf-8")
class_count = 0
done = False

def on_message(msg, data):
    global class_count, done
    if msg["type"] == "error":
        err = msg.get("description", msg.get("stack", str(msg)))
        print(f"[ERROR] {err}")
        out.write(f"[ERROR] {err}\n")
        out.flush()
        done = True
        return
    if msg["type"] != "send":
        return
    payload = msg.get("payload", {})
    action = payload.get("action", "")
    err = payload.get("error", "")
    
    if err:
        print(f"[AGENT ERROR] {err}")
        out.write(f"[AGENT ERROR] {err}\n")
        out.flush()
        done = True
        return
    
    if action == "fatal":
        print(f"[FATAL] {payload.get('error', '')}")
        out.write(f"[FATAL] {payload.get('error', '')}\n")
        out.flush()
        done = True
        return
    
    if action == "init":
        print(f"Unity: {payload.get('unityVersion')}  App: {payload.get('application')}")
        out.write(f"=== Frida IL2CPP Full Bridge Dump ===\nUnity: {payload['unityVersion']}\n\n")
    elif payload.get("type") == "assembly":
        print(f"Assembly: {payload['name']} ({payload['class_count']} classes)")
        out.write(f"\n// ====== {payload['name']} ({payload['class_count']} classes) ======\n\n")
    elif payload.get("type") == "class":
        class_count += 1
        klass = payload
        fields = klass.get("fields", [])
        instance = [f for f in fields if not f.get("is_static") and not f.get("is_thread_static") and not f.get("is_literal")]
        if not instance:
            return
        ns = klass.get("namespace", "") or ""
        name = klass.get("name", "")
        full = f"{ns}.{name}" if ns else name
        line = f"// {full} ({len(instance)} fields)"
        print(line[:120])
        out.write(line + "\n")
        safe_ns = ''.join(c if c.isalnum() or c == '_' else '_' for c in ns)
        out.write(f"namespace {safe_ns} {{ // {name}\n")
        for f in fields:
            off = f.get("offset")
            off_str = f"0x{off:04X}" if off is not None else "-----"
            out.write(f"    {off_str}  {f.get('name','?')}  // {f.get('type_name','?')}\n")
        out.write("}\n\n")
    elif action == "exit":
        print(f"Dumped {class_count} classes in {payload.get('elapsed_ms',0)}ms")
        out.write(f"\n=== DUMP COMPLETE: {class_count} classes ===\n")
        done = True

print("Attaching to RustClient.exe...")
session = frida.attach("RustClient.exe")
script = session.create_script(code)
script.on("message", on_message)
script.load()
print("Dumping... (Ctrl+C to stop)")

import time
while not done:
    time.sleep(0.5)

out.close()
print(f"\nOutput saved to {OUTPUT_PATH}")
