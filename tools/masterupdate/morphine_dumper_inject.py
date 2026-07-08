#!/usr/bin/env python3
"""morphine_dumper_inject.py - Build morphine-dumper DLL, inject into Rust, read output.

Primary source for offsets, decrypts, and encrypts.
Produces morphine-dump.h + morphine-dump.json on Desktop.
"""
import subprocess, sys, os, time, json, shutil
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
CONFIG_PATH = SCRIPT_DIR / "config.json"
OUTPUT_DIR = SCRIPT_DIR / "output"

def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)

def build_morphine_dumper(cfg):
    vcxproj = cfg.get("morphine_dumper_vcxproj", "")
    msbuild = cfg.get("msbuild_path", "msbuild")
    if not vcxproj or not os.path.exists(vcxproj):
        print("[morphine] vcxproj not found, skipping build")
        return False
    print(f"[morphine] Building morphine-dumper...")
    cmd = [msbuild, vcxproj, "/p:Configuration=Release", "/p:Platform=x64", "/v:minimal", "/nologo"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            print(f"[morphine] Build FAILED (rc={result.returncode})")
            for line in result.stderr.split("\n"):
                if "error" in line.lower():
                    print(f"  {line.strip()}")
            return False
        print("[morphine] Build OK")
        return True
    except Exception as e:
        print(f"[morphine] Build exception: {e}")
        return False

def is_rust_running():
    try:
        result = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq RustClient.exe"],
            capture_output=True, text=True, timeout=5
        )
        return "RustClient.exe" in result.stdout
    except:
        return False

def inject_morphine_dumper(cfg):
    dll_path = cfg.get("morphine_dumper_dll", "")
    ps1_path = cfg.get("sha_dumper_inject_ps1", "")  # reuse same inject.ps1
    if not os.path.exists(dll_path):
        print(f"[morphine] DLL not found: {dll_path}")
        return False
    if not os.path.exists(ps1_path):
        print(f"[morphine] inject.ps1 not found: {ps1_path}")
        return False
    print("[morphine] Injecting into RustClient.exe...")
    cmd = ["powershell", "-ExecutionPolicy", "Bypass", "-File", ps1_path, dll_path]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        print(result.stdout)
        if result.returncode != 0:
            print(f"[morphine] Injection may have failed: {result.stderr}")
        return "ok" in result.stdout.lower() or result.returncode == 0
    except Exception as e:
        print(f"[morphine] Injection exception: {e}")
        return False

def wait_for_output(timeout=120):
    desktop = Path(os.path.expanduser("~")) / "Desktop"
    h_path = desktop / "morphine-dumper_output.h"
    json_path = desktop / "morphine-dumper_output.json"
    print(f"[morphine] Waiting for output (timeout {timeout}s)...")
    for i in range(timeout):
        if h_path.exists() and h_path.stat().st_size > 100:
            size = h_path.stat().st_size
            print(f"[morphine] .h output found ({size} bytes)")
            if json_path.exists() and json_path.stat().st_size > 100:
                jsize = json_path.stat().st_size
                print(f"[morphine] .json output found ({jsize} bytes)")
                return h_path, json_path
            else:
                print("[morphine] .json not found yet — waiting for JSON writer...")
        time.sleep(1)
    print("[morphine] Output not found within timeout")
    if h_path.exists():
        return h_path, None
    return None, None

def copy_output(h_src, json_src, cfg):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    h_dst = Path(cfg.get("morphine_dumper_output_h", str(OUTPUT_DIR / "morphine-dump.h")))
    json_dst = Path(cfg.get("morphine_dumper_output_json", str(OUTPUT_DIR / "morphine-dump.json")))

    if h_src and h_src.exists():
        shutil.copy2(str(h_src), str(h_dst))
        print(f"[morphine] Copied .h to {h_dst}")

    if json_src and json_src.exists():
        shutil.copy2(str(json_src), str(json_dst))
        print(f"[morphine] Copied .json to {json_dst}")
        return True

    return False

def main():
    cfg = load_config()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Step 1: Build
    if not build_morphine_dumper(cfg):
        print("[morphine] Build failed — cannot proceed with morphine-dumper")
        return 1

    # Step 2: Check if Rust is running
    if not is_rust_running():
        print("[morphine] Rust not running — morphine-dumper requires game to be running")
        print("[morphine] Start Rust, join a server, then re-run getnewoffsets.bat")
        return 1

    # Step 3: Inject
    injected = inject_morphine_dumper(cfg)
    if not injected:
        print("[morphine] Injection check returned false — but output may still appear on Desktop")

    # Step 4: Wait for output (morphine-dumper takes longer than sha-dumper due to runtime verification)
    h_path, json_path = wait_for_output(120)
    if not h_path:
        print("[morphine] No output on Desktop — morphine-dumper may not have been injected")
        return 1

    # Step 5: Copy to output/
    has_json = copy_output(h_path, json_path, cfg)
    if not has_json:
        print("[morphine] WARNING: JSON not produced — .h file will be parsed instead")

    print("[morphine] Done — morphine-dumper output ready")
    return 0

if __name__ == "__main__":
    sys.exit(main())
