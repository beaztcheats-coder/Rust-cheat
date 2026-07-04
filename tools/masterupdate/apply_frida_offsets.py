#!/usr/bin/env python3
"""
apply_frida_offsets.py v3 - Generate offset patches from Frida runtime dump.

Matches offsets.hpp fields to Frida dump fields by TYPE (class reference).
Rust obfuscates field names (SHA hashes) but TYPES remain readable.
Uses field position within class as secondary matching for common types.

100% Morphine-independent for FIELD OFFSETS.
"""
import json
import re
import sys
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "output"
FRIDA_FILE = OUTPUT_DIR / "frida_validation.json"
PATCH_FILE = OUTPUT_DIR / "frida_offset_patches.txt"
CFG_PATH = Path(__file__).parent / "config.json"

# Fields that can be matched purely by TYPE (unique types within their class)
# Format: "offsets.hpp_field_name": ["type_substring1", "type_substring2"]
# match_by_type prefers exact match, then falls back to substring
#
# VERIFIED against Frida dump (build 24037537):
# - Each entry confirmed to have a UNIQUE type within its Frida class
# - Entries with multiple fields of same type are EXCLUDED (need hash mapping)
TYPE_MATCH = {
    # === BasePlayer - verified unique types ===
    "PlayerEyes": ["PlayerEyes"],
    "PlayerInput": ["PlayerInput"],
    "PlayerModel": ["PlayerModel"],
    "BaseMovement": ["BaseMovement"],
    "PlayerInventory": ["PlayerInventory"],
    "PlayerRigidbody": ["Rigidbody"],
    "PlayerFlags": ["PlayerFlags"],
    # NOT MATCHABLE (ambiguous types):
    #   ClActiveItem - type is fully obfuscated generic, "Item" not in type string
    #   ModelState - field at 0x398 has type System.Action, not ModelState
    #   Mounted - type is BasePlayer (self-ref), not Mountable
    #   CurrentGesture - 2 GestureConfig fields (0x300, 0x658)
    #   DisplayName - multiple System.String fields
    #   CurrentTeam/SteamID - multiple System.UInt64 fields
    #   Frozen/ClothingBlocksAiming - multiple System.Boolean fields

    # === BaseCombatEntity ===
    "Lifestate": ["LifeState"],  # type: BaseCombatEntity.LifeState — unique!

    # === BaseEntity ===
    "model": ["Model"],  # type: Model — unique within BaseEntity (at 0x1A8)
    "bounds": ["Bounds"],  # type: UnityEngine.Bounds — unique! (at 0x17C)

    # === PlayerEyes ===
    # bodyRotation REMOVED — 3 Quaternion fields (0x50, 0x6C, 0x7C), not unique
    # viewOffset/headPosition = Vector3, not unique

    # === PlayerInput ===
    # bodyAngles REMOVED — false positive. Frida found Vector2 at 0x90, but
    # aimbot works with 0x44 (Vector3 field). The Vector2 at 0x90 is a
    # different field (likely input delta), not bodyAngles.

    # === PlayerModel ===
    "SkinnedMultiMesh": ["SkinnedMultiMesh"],  # unique type

    # === PlayerInventory ===
    "loot": ["PlayerLoot"],  # type: PlayerLoot — unique! (at 0x48)
    # Belt/Wear/Main = all same obfuscated type, NOT matchable by type

    # === Model ===
    "boneTransforms": ["Transform[]"],  # type: UnityEngine.Transform[] — unique in Model (at 0x50)
    # rootBone/headBone/eyeBone = all UnityEngine.Transform, not unique

    # === BaseProjectile ===
    "recoil": ["RecoilProperties"],  # type: RecoilProperties — unique! (at 0x3F0)
    "primaryMagazine": ["Magazine"],  # type: BaseProjectile.Magazine — unique! (at 0x3C8)

    # === HeldEntity ===
    "viewModel": ["ViewModel"],  # type: ViewModel — unique! (at 0x240)

    # === RecoilProperties ===
    "NewRecoilOverride": ["RecoilProperties"],  # self-ref — unique! (at 0x80)
    # AimconeCurveScale REMOVED — 4 AnimationCurve fields, not unique

    # === TOD_Sky — all unique TOD_ parameter types ===
    "CycleParameters": ["TOD_CycleParameters"],
    "AtmosphereParameters": ["TOD_AtmosphereParameters"],
    "DayParameters": ["TOD_DayParameters"],
    "NightParameters": ["TOD_NightParameters"],
    "SunParameters": ["TOD_SunParameters"],
    "MoonParameters": ["TOD_MoonParameters"],
    "StarParameters": ["TOD_StarParameters"],
    "Clouds": ["TOD_CloudParameters"],  # must be specific — "TOD_Cloud" also matches TOD_CloudQualityType
    "AmbientParameters": ["TOD_AmbientParameters"],
}

# Fields matched by TYPE + position (index within fields of same type)
# Format: "field_name": (type_substring, index_within_type)
# e.g., "DisplayName" is the 2nd System.String field in BasePlayer
POSITION_MATCH = {
    # DISABLED — position matching is unreliable due to inherited fields.
    # Only TYPE_MATCH is used (100% accurate for unique types).
    # Common-type fields (float, int, bool) must be verified via sha-dumper or validict.
}


def load_frida_data():
    if not FRIDA_FILE.exists():
        print(f"[ERROR] {FRIDA_FILE} not found")
        return None
    with open(FRIDA_FILE, "r", encoding="utf-8") as f:
        data = json.load(f)
    return data.get("offsets", {})


def find_frida_class(frida_offsets, candidates):
    for candidate in candidates:
        if candidate in frida_offsets:
            return frida_offsets[candidate]
        for key in frida_offsets:
            if key.endswith("." + candidate):
                if "Steamworks" in key or "UnityEngine.InputSystem" in key:
                    continue
                return frida_offsets[key]
    return None


def get_fields_by_type(frida_class):
    """Group fields by type, sorted by offset. Returns {type: [(offset, name), ...]}"""
    by_type = {}
    for name, data in frida_class.items():
        if not isinstance(data, dict):
            continue
        off_str = data.get("offset", "")
        typ = data.get("type", "")
        try:
            off = int(off_str, 16)
        except:
            continue
        if off == 0:
            continue
        if typ not in by_type:
            by_type[typ] = []
        by_type[typ].append((off, name))
    # Sort each type group by offset
    for typ in by_type:
        by_type[typ].sort()
    return by_type


def match_by_type(frida_class, type_substrings):
    """Find the offset of the field whose type contains one of the substrings.
    Prefers exact type matches over substring matches to avoid false positives
    (e.g., 'Item' should not match 'ItemDefinition')."""
    # First pass: exact type match (type == substring or type ends with ".substring")
    for name, data in frida_class.items():
        if not isinstance(data, dict):
            continue
        typ = data.get("type", "")
        for sub in type_substrings:
            if typ == sub or typ.endswith("." + sub):
                return data.get("offset")
    # Second pass: substring match (fallback for generics, arrays, etc.)
    for name, data in frida_class.items():
        if not isinstance(data, dict):
            continue
        typ = data.get("type", "")
        for sub in type_substrings:
            if sub in typ:
                return data.get("offset")
    return None


def match_by_position(frida_class, type_substring, index):
    """Find the Nth field of a given type (sorted by offset)."""
    by_type = get_fields_by_type(frida_class)
    for typ, fields in by_type.items():
        if type_substring in typ:
            if index >= 0 and index < len(fields):
                return f"0x{fields[index][0]:X}"
            elif index < 0 and abs(index) <= len(fields):
                return f"0x{fields[index][0]:X}"
    return None


def parse_offset_from_line(line):
    pattern = r'((?:inline|constexpr)\s+(?:uint64_t|std::uintptr_t)\s+(\w+)\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)'
    match = re.search(pattern, line)
    if match:
        return match.group(2), int(match.group(3), 0), match.group(0)
    return None


# Class mapping
FRIDA_CLASS_MAP = {
    "BasePlayer": ["BasePlayer"],
    "BaseNetworkable": ["BaseNetworkable"],
    "BaseCamera": ["Camera"],
    "PlayerEyes": ["PlayerEyes"],
    "PlayerInput": ["PlayerInput"],
    "PlayerModel": ["PlayerModel"],
    "PlayerInventory": ["PlayerInventory"],
    "BaseMovement": ["BaseMovement"],
    "BaseProjectile": ["BaseProjectile"],
    "RecoilProperties": ["RecoilProperties"],
    "Item": ["Item"],
    "ItemDefinition": ["ItemDefinition"],
    "ItemContainer": ["ItemContainer"],
    "HeldEntity": ["HeldEntity"],
    "BaseViewModel": ["BaseViewModel"],
    "WorldItem": ["WorldItem"],
    "FlintStrikeWeapon": ["FlintStrikeWeapon"],
    "SkinnedMultiMesh": ["SkinnedMultiMesh"],
    "Model": ["Model"],
    "TOD_Sky": ["TOD_Sky"],
    "BaseCombatEntity": ["BaseCombatEntity"],
    "BaseEntity": ["BaseEntity"],
    "EffectNetwork": ["EffectNetwork"],
    "ServerProjectile": ["ServerProjectile"],
    "ModelState": ["ModelState"],
}


def main():
    with open(CFG_PATH, "r", encoding="utf-8") as f:
        cfg = json.load(f)

    offsets_hpp = Path(cfg["offsets_hpp"])
    if not offsets_hpp.exists():
        print(f"[ERROR] {offsets_hpp} not found")
        return 1

    content = offsets_hpp.read_text(encoding="utf-8", errors="ignore")
    frida_data = load_frida_data()
    if not frida_data:
        return 1

    print(f"[FRIDA-OFFSETS] Loaded {len(frida_data)} classes from Frida dump")

    patches = []
    found_classes = 0
    matched = 0
    changed = 0
    not_found = 0

    ns_pattern = re.compile(r'\bnamespace\s+(\w+)\s*\{')
    ns_matches = list(ns_pattern.finditer(content))

    for i, ns_match in enumerate(ns_matches):
        ns_name = ns_match.group(1)
        if ns_name in ("offsets", "decrypt", "base_player_flags", "base_entity_flags",
                       "model_state", "base_view_model", "admin_movement",
                       "system_list", "unity_string", "unity_object", "unity_component",
                       "unity_game_object", "unity_transform", "unity_transform_native",
                       "transform_data", "klass_layout", "console_system",
                       "tod_cycle_parameters", "tod_ambient_parameters", "tod_night_parameters",
                       "singleton_typeinfos", "convar_graphics", "convar_admin",
                       "server_projectile", "item_icon", "ViewModel"):
            continue

        # Get namespace block via brace matching
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
        block_lines = block.split('\n')

        candidates = FRIDA_CLASS_MAP.get(ns_name, [ns_name])
        frida_class = find_frida_class(frida_data, candidates)
        if not frida_class:
            continue

        found_classes += 1

        for line in block_lines:
            parsed = parse_offset_from_line(line.strip())
            if not parsed:
                continue

            field_name, old_value, old_line = parsed
            frida_val_str = None

            # Strategy 1: Match by unique type
            if field_name in TYPE_MATCH:
                type_subs = TYPE_MATCH[field_name]
                frida_val_str = match_by_type(frida_class, type_subs)
                if frida_val_str:
                    matched += 1

            # Strategy 2: Match by type + position
            if not frida_val_str and field_name in POSITION_MATCH:
                pos_data = POSITION_MATCH[field_name]
                if isinstance(pos_data, tuple):
                    type_sub, idx = pos_data
                    frida_val_str = match_by_position(frida_class, type_sub, idx)
                elif isinstance(pos_data, list):
                    frida_val_str = match_by_type(frida_class, pos_data)
                if frida_val_str:
                    matched += 1

            if not frida_val_str:
                not_found += 1
                continue

            try:
                frida_val = int(frida_val_str, 16) if isinstance(frida_val_str, str) else int(frida_val_str)
            except (ValueError, TypeError):
                continue

            if frida_val == 0:
                continue

            if frida_val != old_value:
                changed += 1
                new_line = re.sub(
                    rf'((?:inline|constexpr)\s+(?:uint64_t|std::uintptr_t)\s+{re.escape(field_name)}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)',
                    rf'\g<1>0x{frida_val:X}\g<3>',
                    old_line,
                    flags=re.IGNORECASE,
                )
                patches.append({
                    "name": field_name,
                    "namespace": ns_name,
                    "old_value": old_value,
                    "new_value": frida_val,
                    "old_line": old_line.strip(),
                    "new_line": new_line.strip(),
                })

    # Write patch file
    with open(PATCH_FILE, "w", encoding="utf-8") as f:
        f.write("// ============================================================\n")
        f.write("// Frida-validated offset patches (100% accurate runtime data)\n")
        f.write(f"// Classes matched: {found_classes}\n")
        f.write(f"// Fields matched: {matched}\n")
        f.write(f"// Fields changed: {changed}\n")
        f.write(f"// Fields not found: {not_found}\n")
        f.write("// ============================================================\n\n")
        for p in patches:
            f.write(f"// {p['name']} (namespace: {p['namespace']}): 0x{p['old_value']:X} -> 0x{p['new_value']:X}\n")
            f.write(f"// NS: {p['namespace']}\n")
            f.write(f"// OLD: {p['old_line']}\n")
            f.write(f"// NEW: {p['new_line']}\n\n")

    print(f"\n{'='*60}")
    print(f"  FRIDA OFFSET PATCH GENERATION COMPLETE")
    print(f"{'='*60}")
    print(f"  Classes: {found_classes} | Matched: {matched} | Changed: {changed} | Not found: {not_found}")
    print(f"  Output: {PATCH_FILE}")
    print(f"{'='*60}")
    if patches:
        print("\nChanged offsets:")
        for p in patches:
            print(f"  {p['namespace']}::{p['name']}: 0x{p['old_value']:X} -> 0x{p['new_value']:X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
