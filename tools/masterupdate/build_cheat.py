#!/usr/bin/env python3
"""Build the cheat DLL with MSBuild."""
import json
import subprocess
import sys
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def main():
    cfg = load_config()
    project_path = Path(cfg["project_root"]) / cfg["vcxproj"]
    msbuild = cfg.get("msbuild_path", "msbuild")

    # Support multiple flavors via command-line arg or config
    import sys
    flavors = []
    if len(sys.argv) > 1:
        flavors = sys.argv[1:]
    elif cfg.get("build_flavors"):
        flavors = cfg["build_flavors"]
    else:
        flavors = ["Rust Prv Ext", "Rust Prv Ext_debug", "bomzarust", "bettercheats"]

    out_dir = Path(__file__).parent / "output"
    out_dir.mkdir(parents=True, exist_ok=True)

    all_success = True
    for target in flavors:
        build_log = out_dir / f"build_log_{target}.txt"

        cmd = [
            msbuild,
            str(project_path),
            "/p:Configuration=Release",
            "/p:Platform=x64",
            f"/p:TargetName={target}",
            "/m",
            "/nologo",
            "/v:minimal",
        ]

        print(f"[BUILD] Flavor: {target}")
        print(f"[BUILD] {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)

        log_text = f"STDOUT:\n{result.stdout}\n\nSTDERR:\n{result.stderr}\n\nRETURN CODE: {result.returncode}"
        build_log.write_text(log_text, encoding="utf-8")

        if result.returncode != 0:
            print(f"[ERROR] Build failed for {target}. See {build_log}")
            print("    Errors:")
            for line in result.stderr.splitlines():
                if "error" in line.lower():
                    print(f"      {line}")
            all_success = False
            continue

        dll_path = Path(cfg["project_root"]) / "x64" / "Release" / f"{target}.dll"
        print(f"[OK] Build succeeded: {dll_path}")

    # Update summary
    summary_path = out_dir / "summary.txt"
    if summary_path.exists():
        text = summary_path.read_text(encoding="utf-8")
        status = "SUCCESS (all flavors)" if all_success else "FAILED (one or more flavors)"
        text = text.replace("Build status: NOT YET BUILT", f"Build status: {status}")
        summary_path.write_text(text, encoding="utf-8")

    if not all_success:
        sys.exit(1)


if __name__ == "__main__":
    main()
