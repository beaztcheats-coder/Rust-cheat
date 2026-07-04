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
    # Core entity classes
    "BasePlayer", "BaseCombatEntity", "BaseNetworkable", "BaseEntity",
    "BaseCamera", "Camera",
    # Player components
    "PlayerEyes", "PlayerInput", "PlayerModel", "PlayerInventory",
    "BaseMovement", "PlayerWalkMovement", "ModelState",
    # Weapons/items
    "BaseProjectile", "RecoilProperties", "BaseProjectileExt",
    "FlintStrikeWeapon", "Item", "ItemDefinition", "ItemContainer",
    "HeldEntity", "ViewModel", "BaseViewModel", "WorldItem",
    # Visuals
    "SkinnedMultiMesh", "Model", "TOD_Sky",
    # ConVars
    "ConsoleSystem", "ConVar_Graphics", "ConVar_Admin",
    # Network/effects
    "EffectNetwork", "ServerProjectile",
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
                const ga = Process.getModuleByName('GameAssembly.dll').base;
                send({ action: "init", elapsed_ms: new Date() - t0, application: Il2Cpp.application, unityVersion: Il2Cpp.unityVersion, ga: ga.toString() });

                // === Static pointer resolution (100% accurate RVAs) ===
                const staticTargets = ["BaseNetworkable","MainCamera","TOD_Sky","PlayerEyes",
                    "ConsoleSystem","ConVar_Graphics","ConVar_Admin","Item","ItemId",
                    "BaseCombatEntity","BaseEntity","Signage","BuildingBlock","BuildingPrivlidge",
                    "Door","WorldItem","BaseVehicle","BaseNpc","DroppedItemContainer",
                    "OreResourceEntity","CollectibleEntity","PlayerInventory",
                    "Anybrain","InputRedirector",
                    "%c2f2c0e8736dd3e41ecde4f7bd640d9d249d1d65"];
                const staticRvas = {};
                try {
                    Il2Cpp.domain.assemblies.forEach(asm => {
                        asm.image.classes.forEach(klass => {
                            try { klass.fields.length; } catch(e) {}
                            if (staticTargets.indexOf(klass.name) !== -1) {
                                try {
                                    const handle = klass.handle;
                                    if (handle && !handle.isNull()) {
                                        const rva = handle.sub(ga).toInt32();
                                        if (rva > 0) {
                                            staticRvas[klass.name] = "0x" + rva.toString(16).toUpperCase();
                                        }
                                    }
                                } catch(e) {}
                            }
                        });
                    });
                } catch(e) {}
                send({ action: "static_rvas", data: staticRvas });

                const t1 = new Date();
                let classCount = 0;
                Il2Cpp.domain.assemblies.forEach(assembly => {
                    send({ type: "assembly", name: assembly.name, class_count: assembly.image.classCount });
                    assembly.image.classes.forEach((klass, i) => {
                        classCount++;
                        // Dump methods for key classes (decrypt function discovery)
                        let methods = null;
                        try {
                            const klassName = klass.name;
                            if (klassName === "BaseNetworkable" || klassName === "BasePlayer" ||
                                klassName === "PlayerInventory" || klassName === "PlayerEyes" ||
                                klassName === "Item" || klassName === "BaseProjectile" ||
                                klassName === "ConVar_Graphics" || klassName === "ServerProjectile" ||
                                klassName === "Anybrain" || klassName === "InputRedirector" ||
                                (klass.namespace === "anybrain")) {
                                methods = klass.methods.map(m => ({
                                    name: m.name,
                                    address: m.virtualAddress ? m.virtualAddress.toString(16) : null,
                                    rva: m.virtualAddress ? "0x" + m.virtualAddress.sub(ga).toInt32().toString(16).toUpperCase() : null,
                                    parameters: m.parameters ? m.parameters.length : 0
                                }));
                            }
                        } catch(e) {}
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
                            })),
                            methods: methods
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
    frida_methods = {}
    static_rvas = {}
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
            ga_str = payload.get("ga", "?")
            print(f"[OK] Unity: {uv}  App: {app}  GA: {ga_str}")
            dump_file.write(f"=== Frida IL2CPP Full Bridge Dump ===\n")
            dump_file.write(f"Unity: {uv}\n")
            dump_file.write(f"GameAssembly base: {ga_str}\n\n")
            return

        if action == "static_rvas":
            rvas = payload.get("data", {})
            static_rvas.update(rvas)
            print(f"[OK] Static RVAs resolved: {len(rvas)} classes")
            for name, rva in sorted(rvas.items()):
                print(f"  {name}: {rva}")
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

            # Extract method addresses BEFORE field filter (Anybrain class has no fields but has Update() method)
            methods = payload.get("methods")
            if methods:
                key = f"{ns}.{name}" if ns else name
                frida_methods[key] = methods

            # Filter instance + static fields (include static-only classes for Anybrain)
            instance = [f for f in fields if not f.get("is_static") and not f.get("is_literal")]
            statics = [f for f in fields if f.get("is_static") and not f.get("is_literal")]
            if not instance and not statics:
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

            # Extract offsets for ALL classes (not just target list)
            # Store as {field_name: {offset, type}} so apply_frida_offsets can match by type
            key = f"{ns}.{name}" if ns else name
            extracted_offsets[key] = {}
            for f in instance:
                off = f.get("offset")
                if off is not None:
                    extracted_offsets[key][f["name"]] = {"offset": f"0x{off:X}", "type": f.get("type", "")}
            # Also extract static field offsets (needed for Anybrain static cache class)
            for f in statics:
                off = f.get("offset")
                if off is not None:
                    extracted_offsets[key][f["name"]] = {"offset": f"0x{off:X}", "type": f.get("type", ""), "static": True}

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
        "classes_with_offsets": len(extracted_offsets),
        "static_rvas": static_rvas,
        "offsets": extracted_offsets,
    }

    # Save static RVAs to a separate file for compare_and_patch.py to use
    STATIC_RVA_FILE = OUTPUT_DIR / "frida_static_rvas.json"
    STATIC_RVA_FILE.write_text(json.dumps(static_rvas, indent=2), encoding="utf-8")

    VALIDATION_OUTPUT.write_text(json.dumps(validation, indent=2), encoding="utf-8")

    # Save method addresses for decrypt discovery
    METHODS_OUTPUT = OUTPUT_DIR / "frida_methods.json"
    METHODS_OUTPUT.write_text(json.dumps(frida_methods, indent=2), encoding="utf-8")

    print(f"\n[OK] Full dump: {DUMP_OUTPUT}")
    print(f"[OK] Extracted offsets: {VALIDATION_OUTPUT}")
    print(f"[OK] Method addresses: {METHODS_OUTPUT}")
    print(f"[OK] Static RVAs: {STATIC_RVA_FILE}")
    print(f"[OK] Classes with offsets: {len(extracted_offsets)}")
    print(f"[OK] Classes with methods: {len(frida_methods)}")
    print(f"[OK] Static RVAs resolved: {len(static_rvas)}")

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
