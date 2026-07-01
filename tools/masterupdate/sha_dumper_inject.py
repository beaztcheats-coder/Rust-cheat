#!/usr/bin/env python3
"""Build sha-dumper DLL, inject into Rust, read output."""
import subprocess, sys, os, time, json, shutil
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
CONFIG_PATH = SCRIPT_DIR / "config.json"
OUTPUT_DIR = SCRIPT_DIR / "output"

def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)

def find_python():
    for p in ["python", "py", "python3"]:
        try:
            subprocess.run([p, "--version"], capture_output=True, timeout=5)
            return p
        except:
            pass
    return None

def build_sha_dumper(cfg):
    vcxproj = cfg.get("sha_dumper_vcxproj", "")
    msbuild = cfg.get("msbuild_path", "msbuild")
    if not vcxproj or not os.path.exists(vcxproj):
        print("[sha] sha-dumper vcxproj not found, skipping build")
        return False
    print(f"[sha] Building sha-dumper...")
    cmd = [msbuild, vcxproj, "/p:Configuration=Release", "/p:Platform=x64", "/v:minimal", "/nologo"]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode != 0:
            print(f"[sha] Build FAILED (rc={result.returncode})")
            for line in result.stderr.split("\n"):
                if "error" in line.lower():
                    print(f"  {line.strip()}")
            return False
        print("[sha] Build OK")
        return True
    except Exception as e:
        print(f"[sha] Build exception: {e}")
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

def inject_sha_dumper(cfg):
    dll_path = cfg.get("sha_dumper_dll", "")
    ps1_path = cfg.get("sha_dumper_inject_ps1", "")
    if not os.path.exists(dll_path):
        print(f"[sha] DLL not found: {dll_path}")
        return False
    if not os.path.exists(ps1_path):
        print(f"[sha] inject.ps1 not found: {ps1_path}")
        return False
    print("[sha] Injecting into RustClient.exe...")
    cmd = ["powershell", "-ExecutionPolicy", "Bypass", "-File", ps1_path, dll_path]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        print(result.stdout)
        if result.returncode != 0:
            print(f"[sha] Injection may have failed: {result.stderr}")
        return "ok" in result.stdout.lower() or result.returncode == 0
    except Exception as e:
        print(f"[sha] Injection exception: {e}")
        return False

def wait_for_output(timeout=30):
    desktop = Path(os.path.expanduser("~")) / "Desktop" / "sha-dumper_output.txt"
    print(f"[sha] Waiting for output at {desktop}...")
    for i in range(timeout):
        if desktop.exists() and desktop.stat().st_size > 100:
            print(f"[sha] Output found ({desktop.stat().st_size} bytes)")
            return desktop
        time.sleep(1)
    print("[sha] Output not found within timeout")
    return None

def wait_for_mesh_output(timeout=300):
    mesh_log = Path(os.path.expanduser("~")) / "Desktop" / "rust_mesh_dump.log"
    mesh_tri = Path(os.path.expanduser("~")) / "Desktop" / "rust_mesh.tri"
    print(f"[sha] Waiting for mesh dump completion (timeout {timeout}s)...")
    for i in range(timeout):
        if mesh_log.exists() and mesh_log.stat().st_size > 10:
            content = mesh_log.read_text(encoding="utf-8", errors="ignore")
            if "SUCCESS" in content or "ABORT" in content:
                print(f"[sha] Mesh dump finished ({mesh_log.stat().st_size} bytes)")
                return mesh_log
            if mesh_tri.exists() and mesh_tri.stat().st_size > 1000:
                print(f"[sha] Mesh .tri file appeared ({mesh_tri.stat().st_size} bytes)")
                return mesh_log
        time.sleep(1)
    print("[sha] Mesh dump did not complete within timeout")
    return None

def copy_mesh_output(output_dir):
    desktop = Path(os.path.expanduser("~")) / "Desktop"
    mesh_tri = desktop / "rust_mesh.tri"
    mesh_log = desktop / "rust_mesh_dump.log"

    if mesh_tri.exists():
        dst = output_dir / "rust_mesh.tri"
        shutil.copy2(str(mesh_tri), str(dst))
        size_mb = dst.stat().st_size / 1024 / 1024
        tri_count = dst.stat().st_size // 36
        print(f"[mesh] Copied rust_mesh.tri to output/ ({tri_count:,} tris, {size_mb:.1f} MB)")
        return True

    if mesh_log.exists():
        content = mesh_log.read_text(encoding="utf-8", errors="ignore")
        if "ABORT" in content:
            for line in content.splitlines():
                if "ABORT" in line:
                    print(f"[mesh] Mesh dump FAILED: {line.strip()}")
        else:
            print("[mesh] Mesh dump log found but no .tri file — check log for errors")
    else:
        print("[mesh] No mesh dump log found — VisCheck will use fallback")

    return False

def copy_output(src, dst):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(str(src), str(dst))
    print(f"[sha] Copied to {dst}")

def main():
    cfg = load_config()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Step 1: Build
    if not build_sha_dumper(cfg):
        print("[sha] Build failed — using Morphine offsets only")
        return 0

    # Step 2: Check if Rust is running
    if not is_rust_running():
        print("[sha] Rust not running — sha-dumper skipped (Morphine offsets sufficient)")
        return 0

    # Step 3: Inject
    injected = inject_sha_dumper(cfg)
    if not injected:
        print("[sha] Injection check returned false — but output may still appear on Desktop")

    # Step 4: Wait for output (always check, even if injection check failed)
    output_path = wait_for_output(60)
    if not output_path:
        print("[sha] No output on Desktop — sha-dumper may not have been injected")
        return 0

    # Step 5: Copy to output/
    dst = Path(cfg.get("sha_dumper_output", str(OUTPUT_DIR / "sha-dumper-output.txt")))
    copy_output(output_path, dst)

    # Clean up stale underscore version if it exists (naming inconsistency from older runs)
    stale = OUTPUT_DIR / "sha_dumper_output.txt"
    if stale.exists() and stale != dst:
        stale.unlink()
        print(f"[sha] Removed stale {stale.name}")

    # Step 6: Wait for mesh dump output
    mesh_log_path = wait_for_mesh_output(300)
    if mesh_log_path:
        copy_mesh_output(OUTPUT_DIR)
        mesh_log_dst = OUTPUT_DIR / "rust_mesh_dump.log"
        if mesh_log_dst != mesh_log_path:
            shutil.copy2(str(mesh_log_path), str(mesh_log_dst))
            print(f"[mesh] Copied mesh log to {mesh_log_dst}")

    print("[sha] Done — sha-dumper output ready")
    return 0

if __name__ == "__main__":
    sys.exit(main())
