#!/usr/bin/env python3
"""
frida_hash_mapper.py - Build and use {field_hash → readable_name} mapping.

This is the KEY to going 100% Morphine-independent for field offsets.

Rust obfuscates field names with stable SHA hashes. Once we build a mapping
{hash → readable_name} by correlating offsets with a known-good source (validict),
we can resolve ANY future Frida dump by hash lookup — no Morphine needed.

Pipeline:
1. build_mapping() — reads frida_validation.json + offsets.hpp, matches by offset
2. apply_mapping() — reads mapping + new frida_validation.json, resolves hashes → names
3. Generates offset patches in same format as apply_frida_offsets.py
"""
import json
import re
import sys
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "output"
FRIDA_FILE = OUTPUT_DIR / "frida_validation.json"
MAPPING_FILE = OUTPUT_DIR / "field_hash_mapping.json"
PATCH_FILE = OUTPUT_DIR / "frida_offset_patches.txt"
CFG_PATH = Path(__file__).parent / "config.json"


def parse_offsets_hpp(content):
    """Parse offsets.hpp into {namespace: {field_name: offset_value}}."""
    result = {}
    ns_pattern = re.compile(r'\bnamespace\s+(\w+)\s*\{')
    ns_matches = list(ns_pattern.finditer(content))
    
    for i, ns_match in enumerate(ns_matches):
        ns_name = ns_match.group(1)
        if ns_name in ("offsets", "decrypt"):
            continue
        
        brace_start = ns_match.end() - 1
        depth = 1
        pos = brace_start + 1
        while pos < len(content) and depth > 0:
            if content[pos] == '{':
                depth += 1
            elif content[pos] == '}':
                depth -= 1
            pos += 1
        block = content[ns_match.start():pos]
        
        field_pattern = re.compile(
            r'(?:inline|constexpr)\s+(?:uint64_t|std::uintptr_t)\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)(;)'
        )
        for m in field_pattern.finditer(block):
            field_name = m.group(1)
            field_val = int(m.group(2), 0)
            if ns_name not in result:
                result[ns_name] = {}
            result[ns_name][field_name] = field_val
    
    # Also parse top-level offsets namespace
    offsets_pattern = re.compile(
        r'inline\s+uint64_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;'
    )
    for m in offsets_pattern.finditer(content[:5000]):  # Only first 5000 chars for top-level
        field_name = m.group(1)
        field_val = int(m.group(2), 0)
        if "offsets" not in result:
            result["offsets"] = {}
        result["offsets"][field_name] = field_val
    
    return result


def build_mapping():
    """Build {class: {hash: {name, offset, type}}} from Frida dump + offsets.hpp correlation."""
    with open(CFG_PATH, "r", encoding="utf-8") as f:
        cfg = json.load(f)
    offsets_hpp = Path(cfg["offsets_hpp"]).read_text(encoding="utf-8", errors="ignore")
    
    if not FRIDA_FILE.exists():
        print("[ERROR] frida_validation.json not found")
        return None
    
    with open(FRIDA_FILE, "r", encoding="utf-8") as f:
        frida_data = json.load(f)
    frida_offsets = frida_data.get("offsets", {})
    
    hpp_data = parse_offsets_hpp(offsets_hpp)
    
    # Class name mapping: offsets.hpp namespace → Frida class name
    CLASS_MAP = {
        "BasePlayer": "BasePlayer",
        "BaseNetworkable": "BaseNetworkable",
        "PlayerEyes": "PlayerEyes",
        "PlayerInput": "PlayerInput",
        "PlayerModel": "PlayerModel",
        "PlayerInventory": "PlayerInventory",
        "BaseMovement": "BaseMovement",
        "BaseProjectile": "BaseProjectile",
        "BaseCombatEntity": "BaseCombatEntity",
        "BaseEntity": "BaseEntity",
        "Item": "Item",
        "ItemDefinition": "ItemDefinition",
        "ItemContainer": "ItemContainer",
        "HeldEntity": "HeldEntity",
        "BaseViewModel": "BaseViewModel",
        "WorldItem": "WorldItem",
        "FlintStrikeWeapon": "FlintStrikeWeapon",
        "SkinnedMultiMesh": "SkinnedMultiMesh",
        "Model": "Model",
        "TOD_Sky": "TOD_Sky",
        "RecoilProperties": "RecoilProperties",
        "ServerProjectile": "ServerProjectile",
        "EffectNetwork": "EffectNetwork",
        "ModelState": "ModelState",
    }
    
    mapping = {}
    matched = 0
    unmatched = 0
    
    for hpp_ns, frida_class_name in CLASS_MAP.items():
        hpp_fields = hpp_data.get(hpp_ns, {})
        if not hpp_fields:
            continue
        
        # Find Frida class
        frida_class = None
        for key in frida_offsets:
            if key == frida_class_name or key.endswith("." + frida_class_name):
                if "Steamworks" in key or "UnityEngine.InputSystem" in key:
                    continue
                frida_class = frida_offsets[key]
                break
        
        if not frida_class:
            continue
        
        # Build {offset → frida_field_info} for this class
        frida_by_offset = {}
        for fname, fdata in frida_class.items():
            if isinstance(fdata, dict):
                off_str = fdata.get("offset", "")
                try:
                    off = int(off_str, 16)
                except:
                    continue
                frida_by_offset[off] = {
                    "hash": fname,
                    "type": fdata.get("type", ""),
                }
            else:
                try:
                    off = int(fdata, 16) if isinstance(fdata, str) else int(fdata)
                except:
                    continue
                frida_by_offset[off] = {"hash": fname, "type": ""}
        
        # Match offsets.hpp fields to Frida hashes by offset value
        if hpp_ns not in mapping:
            mapping[hpp_ns] = {}
        
        for field_name, hpp_offset in hpp_fields.items():
            if hpp_offset == 0:
                continue
            
            # Direct offset match
            if hpp_offset in frida_by_offset:
                frida_info = frida_by_offset[hpp_offset]
                mapping[hpp_ns][frida_info["hash"]] = {
                    "name": field_name,
                    "offset": hpp_offset,
                    "type": frida_info.get("type", ""),
                }
                matched += 1
            else:
                unmatched += 1
    
    print(f"[MAPPING] Matched {matched} fields, {unmatched} unmatched")
    
    # Save mapping
    MAPPING_FILE.write_text(json.dumps(mapping, indent=2), encoding="utf-8")
    print(f"[OK] Mapping saved to {MAPPING_FILE}")
    
    return mapping


def apply_mapping(existing_mapping=None):
    """Use the {hash → name} mapping to resolve Frida dump into readable offsets.
    
    This is called on FUTURE updates when we have a new Frida dump but NO Morphine.
    The hashes are stable across builds, so we look up each hash in the mapping.
    """
    if existing_mapping is None:
        if not MAPPING_FILE.exists():
            print("[ERROR] No mapping file. Run build_mapping() first.")
            return None
        with open(MAPPING_FILE, "r") as f:
            existing_mapping = json.load(f)
    
    if not FRIDA_FILE.exists():
        print("[ERROR] frida_validation.json not found")
        return None
    
    with open(FRIDA_FILE, "r") as f:
        frida_data = json.load(f)
    frida_offsets = frida_data.get("offsets", {})
    
    # Build reverse lookup: {hash → (namespace, field_name, old_offset)}
    hash_to_name = {}
    for ns, fields in existing_mapping.items():
        for fhash, info in fields.items():
            hash_to_name[fhash] = {
                "namespace": ns,
                "name": info["name"],
                "old_offset": info["offset"],
            }
    
    # Class mapping
    CLASS_MAP = {
        "BasePlayer": "BasePlayer",
        "BaseNetworkable": "BaseNetworkable",
        "PlayerEyes": "PlayerEyes",
        "PlayerInput": "PlayerInput",
        "PlayerModel": "PlayerModel",
        "PlayerInventory": "PlayerInventory",
        "BaseMovement": "BaseMovement",
        "BaseProjectile": "BaseProjectile",
        "BaseCombatEntity": "BaseCombatEntity",
        "BaseEntity": "BaseEntity",
        "Item": "Item",
        "ItemDefinition": "ItemDefinition",
        "ItemContainer": "ItemContainer",
        "HeldEntity": "HeldEntity",
        "BaseViewModel": "BaseViewModel",
        "WorldItem": "WorldItem",
        "FlintStrikeWeapon": "FlintStrikeWeapon",
        "SkinnedMultiMesh": "SkinnedMultiMesh",
        "Model": "Model",
        "TOD_Sky": "TOD_Sky",
        "RecoilProperties": "RecoilProperties",
        "ServerProjectile": "ServerProjectile",
        "EffectNetwork": "EffectNetwork",
        "ModelState": "ModelState",
    }
    
    patches = []
    resolved = 0
    not_in_mapping = 0
    new_fields = []
    
    for hpp_ns, frida_class_name in CLASS_MAP.items():
        # Find Frida class
        frida_class = None
        for key in frida_offsets:
            if key == frida_class_name or key.endswith("." + frida_class_name):
                if "Steamworks" in key or "UnityEngine.InputSystem" in key:
                    continue
                frida_class = frida_offsets[key]
                break
        
        if not frida_class:
            continue
        
        # For each field in Frida dump, look up hash in mapping
        for fhash, fdata in frida_class.items():
            if fhash not in hash_to_name:
                # NEW FIELD DETECTED — not in our mapping
                if isinstance(fdata, dict):
                    ftype = fdata.get("type", "?")
                    foff = fdata.get("offset", "?")
                else:
                    ftype = "?"
                    foff = fdata
                new_fields.append({
                    "class": hpp_ns,
                    "hash": fhash,
                    "offset": foff,
                    "type": ftype,
                })
                continue
            
            info = hash_to_name[fhash]
            if info["namespace"] != hpp_ns:
                continue
            
            field_name = info["name"]
            old_offset = info["old_offset"]
            
            # Get new offset from Frida
            if isinstance(fdata, dict):
                new_off_str = fdata.get("offset", "")
            else:
                new_off_str = fdata
            
            try:
                new_offset = int(new_off_str, 16) if isinstance(new_off_str, str) else int(new_off_str)
            except:
                continue
            
            if new_offset == 0:
                continue
            
            resolved += 1
            
            if new_offset != old_offset:
                patches.append({
                    "name": field_name,
                    "namespace": hpp_ns,
                    "old_value": old_offset,
                    "new_value": new_offset,
                    "hash": fhash,
                })
    
    # Write patches
    with open(PATCH_FILE, "w", encoding="utf-8") as f:
        f.write("// ============================================================\n")
        f.write("// Frida hash-mapped offset patches ( Morphine-independent )\n")
        f.write(f"// Fields resolved by hash: {resolved}\n")
        f.write(f"// Fields changed: {len(patches)}\n")
        f.write("// ============================================================\n\n")
        
        # Read offsets.hpp for OLD/NEW lines
        with open(CFG_PATH, "r") as cfg_f:
            cfg = json.load(cfg_f)
        content = Path(cfg["offsets_hpp"]).read_text(encoding="utf-8", errors="ignore")
        
        for p in patches:
            old_pattern = re.compile(
                rf'((?:inline|constexpr)\s+(?:uint64_t|std::uintptr_t)\s+{re.escape(p["name"])}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)',
                re.IGNORECASE
            )
            m = old_pattern.search(content)
            if m:
                old_line = m.group(0).strip()
                new_line = old_pattern.sub(
                    rf'\g<1>0x{p["new_value"]:X}\g<3>',
                    old_line,
                )
                f.write(f'// {p["name"]} (namespace: {p["namespace"]}): 0x{p["old_value"]:X} -> 0x{p["new_value"]:X}\n')
                f.write(f'// NS: {p["namespace"]}\n')
                f.write(f'// OLD: {old_line}\n')
                f.write(f'// NEW: {new_line}\n\n')
    
    print(f"\n{'='*60}")
    print(f"  HASH-MAPPED OFFSET PATCHES")
    print(f"{'='*60}")
    print(f"  Fields resolved: {resolved}")
    print(f"  Fields changed:  {len(patches)}")
    if new_fields:
        print(f"  NEW fields detected: {len(new_fields)} (need one-time manual mapping)")
    print(f"  Output: {PATCH_FILE}")
    print(f"{'='*60}")
    
    if patches:
        print("\nChanged offsets:")
        for p in patches:
            print(f"  {p['namespace']}::{p['name']}: 0x{p['old_value']:X} -> 0x{p['new_value']:X}")
    
    if new_fields:
        print(f"\n[WARN] {len(new_fields)} NEW unmapped fields detected:")
        for nf in new_fields[:20]:
            print(f"  {nf['class']}  offset={nf['offset']}  type={nf['type'][:40]}  hash={nf['hash'][:20]}...")
        if len(new_fields) > 20:
            print(f"  ... and {len(new_fields) - 20} more")
        print("\n  These need one-time manual mapping. Add them to field_hash_mapping.json")
        print("  or provide validict dump for final mapping.")
    
    return patches


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Frida hash → name field mapper")
    parser.add_argument("--build", action="store_true", help="Build mapping from current offsets.hpp + Frida dump")
    parser.add_argument("--apply", action="store_true", help="Apply existing mapping to generate patches")
    args = parser.parse_args()
    
    if args.build:
        mapping = build_mapping()
        if mapping:
            # Also run apply to show current state
            print("\n--- Verifying mapping ---")
            apply_mapping(mapping)
    elif args.apply:
        apply_mapping()
    else:
        # Default: build then apply
        mapping = build_mapping()
        if mapping:
            print("\n--- Applying mapping ---")
            apply_mapping(mapping)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
