#!/usr/bin/env python3
"""Apply generated patches to source files. Only runs if config auto_apply=true."""
import json
import re
import sys
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def extract_namespace_block(content: str, namespace: str) -> tuple:
    """Find the start/end indices of a namespace block using brace counting.
    Returns (block_start, block_end) where block_end is AFTER the closing brace.
    Returns None if namespace not found."""
    pattern = rf'\bnamespace\s+{re.escape(namespace)}\s*\{{'
    match = re.search(pattern, content)
    if not match:
        return None

    block_start = match.start()
    brace_pos = match.end() - 1  # position of opening '{'
    depth = 1
    pos = brace_pos + 1
    while pos < len(content) and depth > 0:
        if content[pos] == '{':
            depth += 1
        elif content[pos] == '}':
            depth -= 1
        pos += 1

    if depth != 0:
        return None  # unbalanced braces

    return (block_start, pos)


def apply_offset_patches(cfg: dict) -> int:
    """Apply offset patches from offsets_patch.txt to offsets.hpp using namespace-scoped replacement."""
    offsets_path = Path(cfg["offsets_hpp"])
    patch_path = Path(__file__).parent / "output" / "offsets_patch.txt"

    if not offsets_path.exists():
        print(f"[!] {offsets_path} not found")
        return 0, 0
    if not patch_path.exists():
        print("[!] output/offsets_patch.txt not found")
        return 0, 0

    content = offsets_path.read_text(encoding="utf-8", errors="ignore")
    patch_text = patch_path.read_text(encoding="utf-8", errors="ignore")

    applied = 0
    skipped = 0
    # Extract NS/OLD/NEW triples from patch file
    ns_lines = re.findall(r"// NS: (.+)", patch_text)
    old_lines = re.findall(r"// OLD: (.+)", patch_text)
    new_lines = re.findall(r"// NEW: (.+)", patch_text)

    for ns, old_line, new_line in zip(ns_lines, old_lines, new_lines):
        ns = ns.strip()

        # Try namespace-scoped replacement first
        ns_block = extract_namespace_block(content, ns)
        if ns_block:
            block_start, block_end = ns_block
            block_content = content[block_start:block_end]
            if old_line in block_content:
                new_block = block_content.replace(old_line, new_line, 1)
                content = content[:block_start] + new_block + content[block_end:]
                applied += 1
                print(f"[APPLY] [{ns}] {old_line.strip()} -> {new_line.strip()}")
            else:
                print(f"[SKIP] [{ns}] Line not found in namespace: {old_line.strip()}")
                skipped += 1
        else:
            # Fallback: namespace block not found, try global replacement
            if old_line in content:
                content = content.replace(old_line, new_line, 1)
                applied += 1
                print(f"[APPLY] [global fallback] {old_line.strip()} -> {new_line.strip()}")
            else:
                print(f"[SKIP] Could not find: {old_line.strip()}")
                skipped += 1

    offsets_path.write_text(content, encoding="utf-8")
    print(f"[OK] Applied {applied} offset patches ({skipped} skipped) to {offsets_path}")
    return applied, skipped


def apply_sdk_patches(cfg: dict) -> int:
    """Apply sdk.hpp patches if any exist."""
    sdk_path = Path(cfg["sdk_hpp"])
    patch_path = Path(__file__).parent / "output" / "sdk_patch.txt"

    if not sdk_path.exists() or not patch_path.exists():
        return 0

    content = sdk_path.read_text(encoding="utf-8", errors="ignore")
    patch_text = patch_path.read_text(encoding="utf-8", errors="ignore")

    applied = 0
    old_lines = re.findall(r"// OLD: (.+)", patch_text)
    new_lines = re.findall(r"// NEW: (.+)", patch_text)

    for old_line, new_line in zip(old_lines, new_lines):
        if old_line in content:
            content = content.replace(old_line, new_line, 1)
            applied += 1
            print(f"[APPLY SDK] {old_line.strip()} -> {new_line.strip()}")

    sdk_path.write_text(content, encoding="utf-8")
    print(f"[OK] Applied {applied} sdk.hpp patches")
    return applied


def apply_decrypt_patch(cfg: dict) -> bool:
    """Apply decrypt functions to sdk.hpp by replacing the entire decrypt namespace."""
    sdk_path = Path(cfg["sdk_hpp"])
    patch_path = Path(__file__).parent / "output" / "sdk_decrypt_patch.txt"

    if not sdk_path.exists() or not patch_path.exists():
        return False

    content = sdk_path.read_text(encoding="utf-8", errors="ignore")
    patch = patch_path.read_text(encoding="utf-8", errors="ignore")

    # Find existing decrypt namespace using regex (tolerant of spacing variations)
    start_match = re.search(r'\bnamespace\s+decrypt\s*\{', content)
    if not start_match:
        print("[!] Could not find decrypt namespace in sdk.hpp. Apply manually.")
        return False

    start_idx = start_match.start()
    brace_start = start_match.end() - 1  # position of '{'
    brace_count = 1
    i = brace_start + 1
    while i < len(content) and brace_count > 0:
        if content[i] == '{':
            brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
        i += 1

    if brace_count != 0:
        print("[!] Could not find matching closing brace for decrypt namespace. Apply manually.")
        return False

    content = content[:start_idx] + patch + content[i:]
    sdk_path.write_text(content, encoding="utf-8")
    print("[OK] Replaced decrypt namespace in sdk.hpp")
    return True


def apply_offsetmanager_patch(cfg: dict) -> bool:
    """Apply OffsetManager.hpp patch: DecryptConfig struct, LOAD_DEC, and SaveDecryptConfig."""
    om_path = Path(cfg.get("offsets_hpp", "")).parent / "OffsetManager.hpp"
    patch_path = Path(__file__).parent / "output" / "offsetmanager_patch.txt"

    if not om_path.exists():
        print(f"[!] {om_path} not found")
        return False
    if not patch_path.exists():
        print("[!] output/offsetmanager_patch.txt not found")
        return False

    content = om_path.read_text(encoding="utf-8", errors="ignore")
    patch = patch_path.read_text(encoding="utf-8", errors="ignore")

    applied = 0

    # 1. Replace DecryptConfig struct body
    struct_match = re.search(r'(struct\s+DecryptConfig\s*\{)(.*?)(\};)', content, re.DOTALL)
    if struct_match:
        new_struct_body = re.search(r'// === DECRYPTCONFIG_STRUCT ===\n// Replace struct DecryptConfig body[^\n]*\n(.*?)(?=\n// === LOAD_DEC_BLOCK)', patch, re.DOTALL)
        if new_struct_body:
            new_struct = new_struct_body.group(1).strip()
            content = content[:struct_match.start(2)] + "\n" + new_struct + "\n    " + content[struct_match.start(3):]
            applied += 1
            print("[APPLY] OffsetManager DecryptConfig struct")
    else:
        print("[SKIP] Could not find DecryptConfig struct")

    # 2. Replace LOAD_DEC block
    # Use regex to find the full #define LOAD_DEC(...) line (handles any parameter names)
    load_def_match = re.search(r'#define\s+LOAD_DEC\b[^\n]*', content)
    load_end = content.find("#undef LOAD_DEC")
    if load_def_match and load_end != -1:
        new_load = re.search(r'// === LOAD_DEC_BLOCK ===\n// Replace between.*?\n\n(.*?)(?=\n// === SAVE_BLOCK)', patch, re.DOTALL)
        if new_load:
            # Preserve the #define line, replace only the body between define and undef
            define_line = load_def_match.group(0)
            content = content[:load_def_match.end()] + "\n\n" + new_load.group(1).strip() + "\n\n                        " + content[load_end:]
            applied += 1
            print("[APPLY] OffsetManager LOAD_DEC block")
    else:
        print("[SKIP] Could not find LOAD_DEC block")

    # 3. Replace SaveDecryptConfig W() calls
    save_match = re.search(r'(auto W = \[&\]\(const char\* key, uint32_t val\)\s*\{.*?\};)', content, re.DOTALL)
    if save_match:
        save_end = content.find("CloseHandle(h);", save_match.end())
        if save_end != -1:
            new_save = re.search(r'// === SAVE_BLOCK ===\n// Replace between.*?\n\n(.*)', patch, re.DOTALL)
            if new_save:
                save_block = new_save.group(1).strip()
                content = content[:save_match.end()] + "\n\n" + save_block + "\n        " + content[save_end:]
                applied += 1
                print("[APPLY] OffsetManager SaveDecryptConfig")
    else:
        print("[SKIP] Could not find SaveDecryptConfig W() calls")

    if applied > 0:
        om_path.write_text(content, encoding="utf-8")
        print(f"[OK] Applied {applied} OffsetManager patches to {om_path}")
    return applied > 0


def copy_rust_decrypts(cfg: dict):
    """Copy generated rust_decrypts.dat to C:\rust_decrypts.dat."""
    src = Path(__file__).parent / "output" / "rust_decrypts.dat"
    dst = Path(cfg["decrypts_dat"])
    if not src.exists():
        return
    try:
        dst.write_text(src.read_text(encoding="utf-8"), encoding="utf-8")
        print(f"[OK] Copied rust_decrypts.dat to {dst}")
    except PermissionError:
        print(f"[WARN] Could not write {dst} — file is likely locked by the running game.")
        print(f"       Close Rust and copy manually from {src}")
    except Exception as e:
        print(f"[WARN] Could not copy rust_decrypts.dat: {e}")


def copy_materials(cfg: dict):
    """Copy materials.hpp if apply_materials is true."""
    if not cfg.get("apply_materials", False):
        return
    src = Path(__file__).parent / "output" / "materials.hpp"
    dst = Path(cfg["materials_hpp"])
    if src.exists() and src.stat().st_size > 0:
        dst.write_text(src.read_text(encoding="utf-8"), encoding="utf-8")
        print(f"[OK] Copied materials.hpp to {dst}")


def apply_frida_patches(cfg: dict) -> int:
    """Apply Frida-validated offset patches from frida_patches.txt to offsets.hpp.
    Uses the same NS/OLD/NEW format as offsets_patch.txt."""
    offsets_path = Path(cfg["offsets_hpp"])
    patch_path = Path(__file__).parent / "output" / "frida_patches.txt"

    if not offsets_path.exists():
        print(f"[!] {offsets_path} not found")
        return 0, 0
    if not patch_path.exists():
        print("[INFO] output/frida_patches.txt not found — skipping Frida patches")
        return 0, 0

    content = offsets_path.read_text(encoding="utf-8", errors="ignore")
    patch_text = patch_path.read_text(encoding="utf-8", errors="ignore")

    if "No Frida-validated offset changes" in patch_text:
        print("[OK] No Frida patches to apply (all offsets match)")
        return 0, 0

    applied = 0
    skipped = 0
    ns_lines = re.findall(r"// NS: (.+)", patch_text)
    old_lines = re.findall(r"// OLD: (.+)", patch_text)
    new_lines = re.findall(r"// NEW: (.+)", patch_text)

    for ns, old_line, new_line in zip(ns_lines, old_lines, new_lines):
        ns = ns.strip()
        ns_block = extract_namespace_block(content, ns)
        if ns_block:
            block_start, block_end = ns_block
            block_content = content[block_start:block_end]
            if old_line in block_content:
                new_block = block_content.replace(old_line, new_line, 1)
                content = content[:block_start] + new_block + content[block_end:]
                applied += 1
                print(f"[APPLY FRIDA] [{ns}] {old_line.strip()} -> {new_line.strip()}")
            else:
                print(f"[SKIP FRIDA] [{ns}] Line not found: {old_line.strip()}")
                skipped += 1
        else:
            if old_line in content:
                content = content.replace(old_line, new_line, 1)
                applied += 1
                print(f"[APPLY FRIDA] [global] {old_line.strip()} -> {new_line.strip()}")
            else:
                print(f"[SKIP FRIDA] Could not find: {old_line.strip()}")
                skipped += 1

    offsets_path.write_text(content, encoding="utf-8")
    print(f"[OK] Applied {applied} Frida patches ({skipped} skipped) to {offsets_path}")
    return applied, skipped


def main():
    cfg = load_config()

    if not cfg.get("auto_apply", False):
        print("[INFO] auto_apply is false in config.json.")
        print("      Patches were generated but not applied.")
        print("      Review output/offsets_patch.txt and output/sdk_decrypt_patch.txt,")
        print("      then set auto_apply=true and re-run, or apply manually.")
        return

    print("[INFO] auto_apply is true. Applying patches...")

    total_applied = 0
    total_skipped = 0

    applied, skipped = apply_offset_patches(cfg)
    total_applied += applied
    total_skipped += skipped

    applied, skipped = apply_frida_patches(cfg)
    total_applied += applied
    total_skipped += skipped

    # Apply Frida offset patches (100% accurate field offsets from runtime dump)
    frida_offset_patch = Path(__file__).parent / "output" / "frida_offset_patches.txt"
    if frida_offset_patch.exists():
        offsets_path = Path(cfg["offsets_hpp"])
        content = offsets_path.read_text(encoding="utf-8", errors="ignore")
        patch_text = frida_offset_patch.read_text(encoding="utf-8", errors="ignore")
        ns_lines = re.findall(r"// NS: (.+)", patch_text)
        old_lines = re.findall(r"// OLD: (.+)", patch_text)
        new_lines = re.findall(r"// NEW: (.+)", patch_text)
        applied_fo = 0
        skipped_fo = 0
        for ns, old_line, new_line in zip(ns_lines, old_lines, new_lines):
            ns = ns.strip()
            ns_block = extract_namespace_block(content, ns)
            if ns_block:
                block_start, block_end = extract_namespace_block(content, ns)
                block_content = content[block_start:block_end]
                if old_line in block_content:
                    new_block = block_content.replace(old_line, new_line, 1)
                    content = content[:block_start] + new_block + content[block_end:]
                    applied_fo += 1
                    print(f"[APPLY FRIDA-OFFSET] [{ns}] {old_line.strip()} -> {new_line.strip()}")
                else:
                    if old_line in content:
                        content = content.replace(old_line, new_line, 1)
                        applied_fo += 1
                        print(f"[APPLY FRIDA-OFFSET] [global] {old_line.strip()} -> {new_line.strip()}")
                    else:
                        print(f"[SKIP FRIDA-OFFSET] Could not find: {old_line.strip()}")
                        skipped_fo += 1
        if applied_fo > 0:
            offsets_path.write_text(content, encoding="utf-8")
            print(f"[OK] Applied {applied_fo} Frida offset patches ({skipped_fo} skipped)")
            total_applied += applied_fo
            total_skipped += skipped_fo
    else:
        print("[INFO] No Frida offset patches file found — skipping")

    applied = apply_sdk_patches(cfg)
    total_applied += applied

    if apply_decrypt_patch(cfg):
        total_applied += 1

    if apply_offsetmanager_patch(cfg):
        total_applied += 1

    copy_rust_decrypts(cfg)
    copy_materials(cfg)

    print(f"\n[DONE] Applied {total_applied} patches total.")
    if total_skipped > 0:
        print(f"[WARN] {total_skipped} patches were skipped — review output above.")
    print("[OK] All patches applied.")


if __name__ == "__main__":
    main()
