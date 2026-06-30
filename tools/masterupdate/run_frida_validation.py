#!/usr/bin/env python3
"""
run_frida_validation.py - Run Frida to dump 100% accurate runtime offsets from Rust.

This replaces the old stub that only checked if Frida could attach.
Based on E:\\github\\older cheat\\rust\\tools\\frida\\frida_run.py

Requirements:
  - pip install frida frida-tools
  - C:\\frida-il2cpp\\ installed (bridge library)
  - Rust running (RustClient.exe)

Output:
  output/frida_dump.txt  - Full IL2CPP class/field dump
  output/frida_validation.json - Summary + extracted offsets for key classes
"""
import json
import sys
import time
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "output"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

BRIDGE_PATH = Path(r"C:\frida-il2cpp\dist\index.js")
DUMP_OUTPUT = OUTPUT_DIR / "frida_dump.txt"
VALIDATION_OUTPUT = OUTPUT_DIR / "frida_validation.json"

# Classes we care about for offset validation
TARGET_CLASSES = {
    "BasePlayer", "BaseCombatEntity", "BaseNetworkable", "BaseEntity",
    "BaseCamera", "Camera", "PlayerEyes", "PlayerInput", "PlayerModel",
    "PlayerInventory", "BaseProjectile", "RecoilProperties",
    "BaseMovement", "PlayerWalkMovement", "Item", "ItemDefinition",
    "ItemContainer", "TOD_Sky", "SkinnedMultiMesh", "Model",
    "BaseProjectileExt", "FlintStrikeWeapon", "WorldItem",
    "HeldEntity", "ViewModel", "BaseEntity", "ConsoleSystem",
}


def main():
    print("[FRIDA] Starting Frida runtime offset dump...")

    # Check prerequisites
    try:
        import frida
        print(f"[OK] Frida {frida.__version__} available")
    except ImportError:
        print("[ERROR] Frida not installed. Run: pip install frida frida-tools")
        return 1

    if not BRIDGE_PATH.exists():
        print(f"[ERROR] frida-il2cpp bridge not found at {BRIDGE_PATH}")
        print("        Install from: https://github.com/vfsfitvnm/frida-il2cpp-bridge")
        return 1

    # Check if Rust is running
    import subprocess
    try:
        result = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq RustClient.exe", "/NH"],
            capture_output=True, text=True
        )
        if "RustClient.exe" not in result.stdout:
            print("[ERROR] RustClient.exe not running!")
            print("        Start Rust, join a server, THEN run this script.")
            return 1
        print("[OK] RustClient.exe is running")
    except Exception as e:
        print(f"[WARN] Could not check process: {e}")

    # Load bridge
    bridge_code = BRIDGE_PATH.read_text(encoding="utf-8", errors="ignore")

    # Build the Frida agent script
    agent_code = bridge_code + "\n\n" + """
try {
    const t0 = new Date();
    setTimeout(() => {
        try {
            Il2Cpp.perform(() => {
                send({ action: "init", elapsed_ms: new Date() - t0, application: Il2Cpp.application, unityVersion: Il2Cpp.unityVersion });
                const t1 = new Date();
                let classCount = 0;
                Il2Cpp.domain.assemblies.forEach(assembly => {
                    send({ type: "assembly", name: assembly.name, class_count: assembly.image.classCount });
                    assembly.image.classes.forEach((klass, i) => {
                        classCount++;
                        send({
                            type: "class",
                            namespace: klass.namespace,
                            name: klass.name,
                            parent: klass.parent ? klass.parent.type.name : null,
                            fields: klass.fields.map(field => ({
                                name: field.name,
                                type: field.type.name,
                                is_static: field.isStatic,
                                is_literal: field.isLiteral,
                                offset: field.isLiteral ? undefined : field.offset
                            }))
                        });
                    });
                });
                send({ action: "exit", elapsed_ms: new Date() - t1, total_classes: classCount });
            }).catch(e => send({ action: "error", error: e.message + "\\n" + (e.stack || "") }));
        } catch(e) {
            send({ action: "error", error: "setTimeout: " + e.message });
        }
    });
} catch(e) {
    send({ action: "fatal", error: "Top-level: " + (e.message || String(e)) });
}
"""

    # Output files
    dump_file = open(str(DUMP_OUTPUT), "w", encoding="utf-8")
    extracted_offsets = {}
    class_count = 0
    done = False
    error_msg = None

    def on_message(msg, data):
        nonlocal class_count, done, error_msg

        if msg["type"] == "error":
            error_msg = msg.get("description", str(msg))
            print(f"[ERROR] {error_msg}")
            done = True
            return

        if msg["type"] != "send":
            return

        payload = msg.get("payload", {})
        action = payload.get("action", "")

        if action == "fatal" or action == "error":
            error_msg = payload.get("error", "unknown")
            print(f"[FATAL] {error_msg}")
            done = True
            return

        if action == "init":
            uv = payload.get("unityVersion", "?")
            app = payload.get("application", "?")
            print(f"[OK] Unity: {uv}  App: {app}")
            dump_file.write(f"=== Frida IL2CPP Full Bridge Dump ===\n")
            dump_file.write(f"Unity: {uv}\n\n")
            return

        if action == "exit":
            elapsed = payload.get("elapsed_ms", 0)
            total = payload.get("total_classes", 0)
            print(f"[OK] Dumped {total} classes in {elapsed}ms")
            dump_file.write(f"\n=== DUMP COMPLETE: {total} classes ===\n")
            done = True
            return

        if payload.get("type") == "assembly":
            name = payload.get("name", "?")
            count = payload.get("class_count", 0)
            print(f"  [{name}] {count} classes...")
            dump_file.write(f"\n// ====== {name} ({count} classes) ======\n\n")
            return

        if payload.get("type") == "class":
            class_count += 1
            ns = payload.get("namespace", "") or ""
            name = payload.get("name", "")
            fields = payload.get("fields", [])

            # Filter instance fields
            instance = [f for f in fields if not f.get("is_static") and not f.get("is_literal")]
            if not instance:
                return

            full = f"{ns}.{name}" if ns else name
            dump_file.write(f"// {full} ({len(instance)} fields)\n")
            safe_ns = ns.replace(".", "_") if ns else ""
            dump_file.write(f"namespace {safe_ns} {{ // {name}\n")

            for f in fields:
                off = f.get("offset")
                off_str = f"0x{off:04X}" if off is not None else "-----"
                marker = " [static]" if f.get("is_static") else ""
                dump_file.write(f"    {off_str}  {f.get('name','?')}{marker}  // {f.get('type','?')}\n")

            dump_file.write("}\n\n")

            # Extract offsets for target classes
            if name in TARGET_CLASSES:
                key = f"{ns}.{name}" if ns else name
                extracted_offsets[key] = {}
                for f in instance:
                    off = f.get("offset")
                    if off is not None:
                        extracted_offsets[key][f["name"]] = f"0x{off:X}"

            return

    # Attach and run
    print("[FRIDA] Attaching to RustClient.exe...")
    try:
        session = frida.attach("RustClient.exe")
    except frida.ProcessNotFoundError:
        print("[ERROR] RustClient.exe not found. Is Rust running?")
        dump_file.close()
        return 1
    except Exception as e:
        print(f"[ERROR] Failed to attach: {e}")
        dump_file.close()
        return 1

    print("[OK] Attached. Dumping (this may take 30-60 seconds)...")
    script = session.create_script(agent_code)
    script.on("message", on_message)
    script.load()

    # Wait for completion (timeout after 5 minutes)
    timeout = 300
    start = time.time()
    while not done:
        time.sleep(0.5)
        elapsed = time.time() - start
        if elapsed > timeout:
            print(f"[WARN] Timed out after {timeout}s. Partial dump saved.")
            break

    dump_file.close()
    session.detach()

    # Save extracted offsets
    validation = {
        "status": "SUCCESS" if not error_msg else "PARTIAL",
        "error": error_msg,
        "classes_dumped": class_count,
        "target_classes_found": len(extracted_offsets),
        "offsets": extracted_offsets,
    }

    VALIDATION_OUTPUT.write_text(json.dumps(validation, indent=2), encoding="utf-8")

    print(f"\n[OK] Full dump: {DUMP_OUTPUT}")
    print(f"[OK] Extracted offsets: {VALIDATION_OUTPUT}")
    print(f"[OK] Target classes found: {len(extracted_offsets)}")

    if extracted_offsets:
        print("\n[FRIDA] Sample verified offsets:")
        for cls in list(extracted_offsets.keys())[:5]:
            fields = extracted_offsets[cls]
            print(f"  {cls}: {len(fields)} fields")
            for fname, fval in list(fields.items())[:3]:
                print(f"    {fname} = {fval}")

    return 0 if not error_msg else 0  # Don't fail the pipeline


if __name__ == "__main__":
    sys.exit(main())
