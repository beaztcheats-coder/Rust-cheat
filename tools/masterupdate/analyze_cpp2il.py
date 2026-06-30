#!/usr/bin/env python3
"""Analyze Cpp2IL-generated DLLs and produce a class/type reference file.

Rust's GameAssembly is obfuscated, so field names are not recovered reliably.
However, Cpp2IL DLLs still contain:
  - class/struct names and namespaces
  - class sizes (from ClassLayout)
  - base class hierarchy
  - field counts and field type references

We use this to validate Morphine offsets (e.g., does BasePlayer exist,
how big is it, is PlayerEyes a real class) and to heuristically classify
field types for the super prompt.
"""
import json
import os
import subprocess
import sys
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"
CPP2IL_DIR = Path(__file__).parent / "cpp2il"
CPP2IL_EXE = CPP2IL_DIR / "Cpp2IL.exe"
MONO_CECIL_DIR = CPP2IL_DIR / "Mono.Cecil" / "lib" / "netstandard2.0"
OUT_DIR = Path(__file__).parent / "output"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def load_master():
    path = OUT_DIR / "master.json"
    if not path.exists():
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def ensure_cpp2il_output(cfg: dict, dll_out: Path):
    """Run Cpp2IL if the DLL output folder does not exist or is empty."""
    if dll_out.exists() and any(dll_out.glob("*.dll")):
        print(f"[INFO] Using existing Cpp2IL output at {dll_out}")
        return True

    if not CPP2IL_EXE.exists():
        print(f"[ERROR] Cpp2IL not found at {CPP2IL_EXE}")
        print("        Run the download step or place Cpp2IL.exe in tools/masterupdate/cpp2il/")
        return False

    game_path = cfg.get("game_path", "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rust")
    exe_name = cfg.get("game_exe_name", "RustClient")

    print(f"[INFO] Running Cpp2IL on {game_path} ...")
    cmd = [
        str(CPP2IL_EXE),
        "--game-path", game_path,
        "--exe-name", exe_name,
        "--output-as", "dll_default",
        "--output-to", str(dll_out),
    ]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if result.returncode != 0:
        print("[ERROR] Cpp2IL failed:")
        print(result.stdout[-2000:] if len(result.stdout) > 2000 else result.stdout)
        print(result.stderr[-2000:] if len(result.stderr) > 2000 else result.stderr)
        return False

    if not any(dll_out.glob("*.dll")):
        print("[ERROR] Cpp2IL produced no DLLs")
        return False

    print(f"[OK] Cpp2IL output ready at {dll_out}")
    return True


def setup_pythonnet():
    """Configure pythonnet to use the installed .NET 8 runtime."""
    try:
        from clr_loader import get_coreclr
        from pythonnet import set_runtime
    except ImportError as e:
        print(f"[ERROR] pythonnet/clr_loader not installed: {e}")
        return False

    # Find dotnet root
    dotnet_root = os.environ.get("DOTNET_ROOT")
    if not dotnet_root or not Path(dotnet_root).exists():
        candidate = Path(r"C:\Program Files\dotnet")
        if candidate.exists():
            dotnet_root = str(candidate)
        else:
            print("[ERROR] Could not locate .NET runtime. Install .NET 8 SDK/runtime.")
            return False

    # Create a temporary runtimeconfig.json for .NET 8 if needed
    runtimeconfig = OUT_DIR / "runtimeconfig.json"
    if not runtimeconfig.exists():
        runtimeconfig.write_text(
            json.dumps(
                {
                    "runtimeOptions": {
                        "tfm": "net8.0",
                        "framework": {"name": "Microsoft.NETCore.App", "version": "8.0.28"},
                    }
                },
                indent=2,
            ),
            encoding="utf-8",
        )

    try:
        rt = get_coreclr(runtime_config=str(runtimeconfig), dotnet_root=dotnet_root)
        set_runtime(rt)
    except Exception as e:
        print(f"[ERROR] Failed to initialize .NET runtime: {e}")
        return False

    sys.path.append(str(MONO_CECIL_DIR))
    try:
        import clr
        clr.AddReference("Mono.Cecil")
    except Exception as e:
        print(f"[ERROR] Failed to load Mono.Cecil: {e}")
        return False

    return True


def analyze_dlls(dll_out: Path):
    """Scan all Cpp2IL DLLs and build a class/type reference dictionary."""
    from Mono.Cecil import AssemblyDefinition

    classes = {}
    total = 0

    dll_files = list(dll_out.glob("*.dll"))
    print(f"[INFO] Scanning {len(dll_files)} DLLs with Mono.Cecil ...")

    for dll_path in dll_files:
        try:
            asm = AssemblyDefinition.ReadAssembly(str(dll_path))
        except Exception as e:
            print(f"[WARN] Could not read {dll_path.name}: {e}")
            continue

        try:
            for t in asm.MainModule.Types:
                total += 1
                if not t.Name or t.Name.startswith("<"):
                    continue

                # Build full name
                full_name = t.FullName
                namespace = t.Namespace or ""

                # Try to get class size from ClassLayout
                class_size = -1
                if t.ClassSize != 0xFFFFFFFF:
                    class_size = int(t.ClassSize)

                # Base type
                base_type = None
                if t.BaseType:
                    base_type = t.BaseType.FullName

                # Interfaces
                interfaces = [i.InterfaceType.FullName for i in t.Interfaces]

                # Field count
                field_count = t.Fields.Count

                # Static field count
                static_field_count = sum(1 for i in range(t.Fields.Count) if t.Fields[i].IsStatic)

                classes[full_name] = {
                    "name": t.Name,
                    "namespace": namespace,
                    "class_size": class_size,
                    "field_count": field_count,
                    "static_field_count": static_field_count,
                    "base_type": base_type,
                    "interfaces": interfaces,
                    "source_dll": dll_path.name,
                }
        finally:
            asm.Dispose()

    print(f"[OK] Extracted {len(classes)} classes from {len(dll_files)} DLLs (total types: {total})")
    return classes


def save_results(classes: dict):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out_path = OUT_DIR / "cpp2il_types.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(
            {
                "source": "Cpp2IL dll_default output",
                "note": "Field names are obfuscated in Rust's binary; this file contains class-level info only.",
                "class_count": len(classes),
                "classes": classes,
            },
            f,
            indent=2,
        )
    print(f"[OK] Wrote {out_path}")


def main():
    cfg = load_config()
    master = load_master()

    dll_out = OUT_DIR / "cpp2il_dlls"

    if not ensure_cpp2il_output(cfg, dll_out):
        sys.exit(1)

    if not setup_pythonnet():
        print("[ERROR] Cannot analyze DLLs without .NET + pythonnet + Mono.Cecil")
        sys.exit(1)

    classes = analyze_dlls(dll_out)
    save_results(classes)

    # Quick validation report against Morphine classes
    morphine_structs = set(master.get("structs", {}).keys())
    if morphine_structs:
        print("\n[INFO] Morphine struct -> Cpp2IL class lookup sample:")
        for struct_name in sorted(morphine_structs)[:10]:
            # Try a few name transformations
            candidates = [
                struct_name,
                struct_name.replace("_", ""),
                "".join(p.capitalize() for p in struct_name.split("_")),
            ]
            found = next((c for c in candidates if c in {cls["name"] for cls in classes.values()}), None)
            status = f"found as {found}" if found else "NOT FOUND"
            print(f"  {struct_name}: {status}")


if __name__ == "__main__":
    main()
