#!/usr/bin/env python3
"""Parse Il2CppInspectorPro output (offsets.json + il2cpp-types-ptr.h) into master.json.
Cross-validates field offsets against current offsets.hpp values."""
import json
import re
import sys
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "output"
IL2CPP_OFFSETS = OUTPUT_DIR / "il2cppinspector" / "offsets.json"
IL2CPP_TYPES_PTR = OUTPUT_DIR / "il2cppinspector" / "cpp" / "appdata" / "il2cpp-types-ptr.h"
CONFIG_PATH = Path(__file__).parent / "config.json"
OFFSETS_HPP = None  # Set from config

# Class name mappings: Il2CppInspector class → offsets.hpp namespace
CLASS_TO_NAMESPACE = {
    "BasePlayer": "BasePlayer",
    "BaseCombatEntity": "BaseCombatEntity",
    "BaseNetworkable": "BaseNetworkable",
    "BaseEntity": "BaseEntity",
    "PlayerEyes": "PlayerEyes",
    "PlayerInput": "PlayerInput",
    "PlayerModel": "PlayerModel",
    "PlayerInventory": "PlayerInventory",
    "Item": "Item",
    "ItemDefinition": "ItemDefinition",
    "BaseProjectile": "BaseProjectile",
    "RecoilProperties": "RecoilProperties",
    "HeldEntity": "HeldEntity",
    "Model": "Model",
    "SkinnedMultiMesh": "SkinnedMultiMesh",
    "TOD_Sky": "TOD_Sky",
    "PlayerWalkMovement": "PlayerWalkMovement",
    "BaseMovement": "BaseMovement2",
    "ViewModel": "HeldEntity",
    "WorldItem": "WorldItem",
    "FlintStrikeWeapon": "FlintStrikeWeapon",
    "Camera": "BaseCamera",
}

# TypeInfo pointer name mappings
TYPEINFO_MAP = {
    "BaseNetworkable": "basenetworkable_pointer",
    "MainCamera": "camera_pointer",
    "TOD_Sky": "TOD_Sky_TypeInfo",
}


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def load_offsets_hpp(cfg):
    """Read current offsets.hpp and extract field offsets per namespace."""
    global OFFSETS_HPP
    OFFSETS_HPP = Path(cfg["offsets_hpp"])
    if not OFFSETS_HPP.exists():
        return {}

    content = OFFSETS_HPP.read_text(encoding="utf-8", errors="ignore")
    result = {}

    # Find ALL namespace declarations (including nested) using re.finditer,
    # then use brace counting to determine each block's body extent.
    # The previous approach used re.match at position i, found the outermost
    # namespace, and skipped past its closing brace — consuming all nested
    # namespaces inside it. re.finditer finds ALL declarations regardless
    # of nesting depth.
    field_pattern = re.compile(r'(?:inline|constexpr)\s+uint64_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', re.DOTALL)

    for ns_match in re.finditer(r'namespace\s+(\w+)\s*\{', content):
        ns_name = ns_match.group(1)
        brace_start = ns_match.end() - 1  # position of '{' in content
        brace_count = 1
        j = brace_start + 1
        while j < len(content) and brace_count > 0:
            if content[j] == '{':
                brace_count += 1
            elif content[j] == '}':
                brace_count -= 1
            j += 1
        body = content[brace_start + 1:j - 1]

        fields = {}
        for fm in field_pattern.finditer(body):
            field_name = fm.group(1)
            field_val = int(fm.group(2), 16)
            fields[field_name] = field_val

        if fields:
            # For nested namespaces, their fields also appear in the parent's body.
            # The innermost namespace's fields take priority — overwrite if already present.
            result[ns_name] = fields

    return result


def parse_typeinfo_pointers(current_offsets):
    """Parse il2cpp-types-ptr.h for DO_TYPEDEF entries.
    Prefers entries whose RVA matches current offsets.hpp values."""
    if not IL2CPP_TYPES_PTR.exists():
        print("[IL2CPP] il2cpp-types-ptr.h not found — skipping TypeInfo RVAs")
        return {}

    content = IL2CPP_TYPES_PTR.read_text(encoding="utf-8", errors="ignore")
    pointers = {}

    # Current offsets.hpp static pointer values (from offsets namespace)
    offsets_ns = current_offsets.get("offsets", {})
    current_ptrs = {
        "basenetworkable_pointer": offsets_ns.get("basenetworkable_pointer", 0),
        "camera_pointer": offsets_ns.get("camera_pointer", 0),
        "TOD_Sky_TypeInfo": offsets_ns.get("TOD_Sky_TypeInfo", 0),
    }

    # Collect ALL matching entries for each target
    all_matches = {}
    pattern = re.compile(r'DO_TYPEDEF\((0x[0-9A-Fa-f]+),\s*(\w+)\)')
    for match in pattern.finditer(content):
        addr = int(match.group(1), 16)
        name = match.group(2)

        for target, cheat_name in TYPEINFO_MAP.items():
            # Only match exact class names — skip obfuscated nested types
            # (names like BaseNetworkable_b302520422a54b5aec31203d0475d8b6506fedf3)
            if name == target:
                all_matches.setdefault(cheat_name, []).append((addr, name))

    # For each target, prefer the entry whose RVA matches current offsets.hpp
    for cheat_name, candidates in all_matches.items():
        current_val = current_ptrs.get(cheat_name, 0)
        chosen = None
        for addr, name in candidates:
            if addr == current_val:
                chosen = (addr, name)
                break
        if not chosen:
            chosen = candidates[0]  # fallback to first
        pointers[cheat_name] = chosen[0]
        if chosen[1] != cheat_name:
            print(f"  [IL2CPP] {cheat_name} -> matched as {chosen[1]} (RVA 0x{chosen[0]:X})")

    return pointers


def parse_field_offsets(current_offsets):
    """Parse offsets.json and match fields to current offsets.hpp values."""
    if not IL2CPP_OFFSETS.exists():
        print("[IL2CPP] offsets.json not found — skipping field offsets")
        return {}, {"matched": 0, "unmatched": 0, "total": 0}

    with open(IL2CPP_OFFSETS, "r", encoding="utf-8") as f:
        data = json.load(f)

    classes = data.get("classes", {})
    structs = {}
    stats = {"matched": 0, "unmatched": 0, "total": 0}

    for class_name, class_data in classes.items():
        if isinstance(class_data, dict) and "error" in class_data:
            continue

        fields = class_data.get("fields", {})
        if not fields:
            continue

        # Find matching namespace in offsets.hpp
        ns_name = CLASS_TO_NAMESPACE.get(class_name)
        if not ns_name:
            continue

        current_fields = current_offsets.get(ns_name, {})
        if not current_fields:
            continue

        # Build reverse map: offset → list of field names in offsets.hpp
        offset_to_names = {}
        for fname, foffset in current_fields.items():
            offset_to_names.setdefault(foffset, []).append(fname)

        # Match Il2CppInspector fields to offsets.hpp by offset value
        matched = {}
        for il2cpp_name, il2cpp_data in fields.items():
            if not isinstance(il2cpp_data, dict):
                continue
            il2cpp_offset = il2cpp_data.get("offset", 0)
            if il2cpp_offset == 0:
                continue

            stats["total"] += 1

            if il2cpp_offset in offset_to_names:
                for hpp_name in offset_to_names[il2cpp_offset]:
                    if hpp_name not in matched:
                        matched[hpp_name] = il2cpp_offset
                        stats["matched"] += 1
                        break
            else:
                stats["unmatched"] += 1

        if matched:
            structs[ns_name] = matched

    return structs, stats


def main():
    print("[IL2CPP] Parsing Il2CppInspectorPro output...")
    cfg = load_config()

    # Load current offsets.hpp for cross-validation
    current_offsets = load_offsets_hpp(cfg)
    print(f"[IL2CPP] Loaded {len(current_offsets)} namespaces from offsets.hpp")

    # Parse TypeInfo pointers
    typeinfo_ptrs = parse_typeinfo_pointers(current_offsets)
    print(f"[IL2CPP] Found {len(typeinfo_ptrs)} TypeInfo RVAs:")
    for name, rva in typeinfo_ptrs.items():
        print(f"  {name}: 0x{rva:X}")

    # Parse field offsets
    structs, stats = parse_field_offsets(current_offsets)
    print(f"\n[IL2CPP] Field offset cross-validation:")
    print(f"  Matched:   {stats['matched']}/{stats['total']}")
    print(f"  Unmatched: {stats['unmatched']}")
    print(f"  Structs:   {len(structs)}")

    # Load or create master.json
    master_path = OUTPUT_DIR / "master.json"
    if master_path.exists():
        with open(master_path, "r", encoding="utf-8") as f:
            master = json.load(f)
    else:
        master = {"structs": {}, "static_pointers": {}, "decrypts": {}}

    if "structs" not in master:
        master["structs"] = {}
    if "static_pointers" not in master:
        master["static_pointers"] = {}

    # Merge TypeInfo pointers into master.json
    for name, rva in typeinfo_ptrs.items():
        master["static_pointers"][name] = rva
        print(f"  [MERGE] static_pointers.{name} = 0x{rva:X}")

    # Merge field offsets into master.json (as il2cpp_validated structs)
    il2cpp_structs = {}
    for ns_name, fields in structs.items():
        il2cpp_structs[ns_name] = fields

    master["structs"]["il2cpp_validated"] = il2cpp_structs

    # Save master.json
    with open(master_path, "w", encoding="utf-8") as f:
        json.dump(master, f, indent=2)
    print(f"\n[IL2CPP] Merged into master.json ({master_path.stat().st_size} bytes)")

    # Write cross-validation report
    report_path = OUTPUT_DIR / "il2cpp_cross_validation.txt"
    with open(report_path, "w", encoding="utf-8") as f:
        f.write("Il2CppInspectorPro Cross-Validation Report\n")
        f.write("=" * 50 + "\n\n")
        f.write(f"Total fields checked: {stats['total']}\n")
        f.write(f"Matched (offset agrees): {stats['matched']}\n")
        f.write(f"Unmatched (offset not found): {stats['unmatched']}\n")
        f.write(f"Match rate: {stats['matched']*100//max(stats['total'],1)}%\n\n")

        f.write("TypeInfo RVAs:\n")
        for name, rva in typeinfo_ptrs.items():
            f.write(f"  {name}: 0x{rva:X}\n")

        f.write("\nValidated structs:\n")
        for ns_name, fields in sorted(il2cpp_structs.items()):
            f.write(f"\n  [{ns_name}] ({len(fields)} fields matched)\n")
            for fname, offset in sorted(fields.items(), key=lambda x: x[1]):
                f.write(f"    {fname}: 0x{offset:X}\n")

    print(f"[IL2CPP] Cross-validation report: {report_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
