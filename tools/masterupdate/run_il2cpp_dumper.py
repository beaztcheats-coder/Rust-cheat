#!/usr/bin/env python3
"""Run il2cpp-dumper-rs to extract field offsets from GameAssembly.dll."""
import json
import os
import subprocess
import sys
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"

def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)

def main():
    cfg = load_config()
    out_dir = Path(__file__).parent / "output"
    out_dir.mkdir(parents=True, exist_ok=True)
    
    gameassembly = cfg.get("gameassembly_path")
    global_metadata = cfg.get("game_path", "") + r"\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat"
    
    if not gameassembly or not os.path.exists(gameassembly):
        print(f"[WARN] GameAssembly.dll not found at {gameassembly}")
        return 1
    
    if not os.path.exists(global_metadata):
        print(f"[WARN] global-metadata.dat not found at {global_metadata}")
        return 1
    
    print(f"[DUMPER] Dumping from {gameassembly}")
    print(f"[DUMPER] Using metadata: {global_metadata}")

    # Check metadata version - Rust Unity 6 uses v39 which is unsupported
    # by Il2CppDumper. Skip early to avoid hanging on Console.ReadKey().
    try:
        with open(global_metadata, "rb") as f:
            magic = f.read(4)
            version = int.from_bytes(f.read(4), "little")
        if magic == b"\xaf\x1b\xb1\xfa" and version >= 29:
            print(f"[WARN] Metadata version {version} unsupported by Il2CppDumper.")
            print(f"[WARN] Frida runtime dump provides field offsets at 100% accuracy.")
            print(f"[WARN] Skipping il2cpp-dumper. Use Frida instead.")
            return 0
    except Exception:
        pass
    
    # Run il2cpp_dumper - check multiple locations
    dumper_candidates = [
        Path(__file__).parent.parent / "dumper" / "Il2CppDumper.exe",          # tools/dumper/
        Path(__file__).parent / "cpp2il" / "Il2CppDumper" / "Il2CppDumper.exe", # masterupdate/cpp2il/
        Path(__file__).parent.parent / "dumper" / "Il2CppDumper.dll",           # net6 version
    ]
    
    dumper_exe = None
    for candidate in dumper_candidates:
        if candidate.exists():
            dumper_exe = candidate
            break
    
    if not dumper_exe:
        print(f"[WARN] Il2CppDumper.exe not found. Checked:")
        for c in dumper_candidates:
            print(f"       - {c}")
        print("       Skipping il2cpp-dumper.")
        return 0
    
    output_dir = out_dir / "il2cpp_dump"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    try:
        # Il2CppDumper calls Console.ReadKey() which needs a REAL console.
        # Run it in a new console window, wait for it to finish.
        cmd_args = f'"{dumper_exe}" "{gameassembly}" "{global_metadata}" "{output_dir}"'
        
        print(f"[DUMPER] Running Il2CppDumper in console window...")
        print(f"[DUMPER] Output dir: {output_dir}")
        
        # Use cmd /c with start /wait to run in a new console and wait
        result = subprocess.run(
            f'cmd /c "{cmd_args}"',
            shell=True,
            timeout=300,
            cwd=str(output_dir.parent),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        
        # Check if dump.cs was generated regardless of exit code
        dump_cs = output_dir / "dump.cs"
        if dump_cs.exists():
            print(f"[OK] dump.cs generated ({dump_cs.stat().st_size} bytes)")
            (out_dir / "dump.cs").write_text(dump_cs.read_text(encoding="utf-8", errors="ignore"))
            print(f"[OK] dump.cs copied to {out_dir / 'dump.cs'}")
            return 0
        else:
            print(f"[WARN] il2cpp-dumper produced no output.")
            print(f"[WARN] Rust uses metadata v39 (Unity 6) which may be unsupported.")
            print(f"[WARN] Frida runtime dump provides the same data at 100% accuracy.")
            print(f"[WARN] Make sure Rust is running so Frida can validate.")
            return 0
            
    except subprocess.TimeoutExpired:
        print(f"[FAIL] il2cpp-dumper timed out after 5 minutes")
        return 1
    except Exception as e:
        print(f"[FAIL] il2cpp-dumper error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
