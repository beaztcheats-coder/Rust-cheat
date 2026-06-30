#!/usr/bin/env python3
"""Parse sha-dumper INI output into master.json format (Morphine-compatible).

Works as a Morphine fallback: if Morphine is down, this parser converts
sha-dumper output to the same master.json format that compare_and_patch.py expects.

If master.json already exists (from Morphine), this MERGES sha-dumper data
into it, filling any gaps (native camera, TOD instances, extra ciphers).

Fixes:
- Keeps ALL duplicate cipher sections (doesn't blindly pick first)
- Fingerprint matching for missing cipher functions (nk2, fov)
- Never emits 0x0 offset values (prevents breaking offsets from name obfuscation)
"""
import json
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
CONFIG_PATH = SCRIPT_DIR / "config.json"
OUTPUT_DIR = SCRIPT_DIR / "output"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def parse_int(s: str) -> int:
    s = s.strip()
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    try:
        return int(s)
    except ValueError:
        return 0


def parse_ini(text: str):
    """Parse INI-style [section] / key = value format.

    Returns (sections, cipher_variants):
      sections: {name: {key: value_str}}  — first occurrence of each non-cipher section
      cipher_variants: {tag: [{key: value_str}, ...]}  — ALL cipher section variants
    """
    sections = {}
    cipher_variants = {}
    current = None
    current_dict = None

    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("//") or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            name = line[1:-1]
            if name.startswith("cipher_"):
                tag = name[len("cipher_"):]
                if tag not in cipher_variants:
                    cipher_variants[tag] = []
                cipher_variants[tag].append({})
                current_dict = cipher_variants[tag][-1]
            elif name in sections:
                current_dict = None  # Skip non-cipher duplicate
            else:
                sections[name] = {}
                current_dict = sections[name]
        elif current_dict is not None and "=" in line:
            key, _, val = line.partition("=")
            current_dict[key.strip()] = val.strip()

    return sections, cipher_variants


def parse_cipher_block(tag: str, fields: dict) -> dict:
    """Parse a [cipher_*] block into a decrypt function dict."""
    ops = []
    op_count = parse_int(fields.get("op_count", "0"))
    for i in range(op_count):
        key = f"op[{i}]"
        if key not in fields:
            continue
        raw = fields[key].strip()
        parts = raw.split(None, 1)
        if len(parts) == 2:
            op_name = parts[0].lower()
            op_val = parse_int(parts[1])
            ops.append({"op": op_name, "value": op_val})

    result = {
        "morphine_name": tag,
        "input_style": "pointer" if tag.startswith("bn_") else "raw",
        "return_style": "handle" if tag.startswith("bn_") else "raw",
        "read_offset": None,
        "ops": ops,
    }

    hv = fields.get("hv_offset")
    if hv:
        result["read_offset"] = parse_int(hv)

    return result


# Tag mapping: sha-dumper cipher tag -> Morphine decrypt function name
CIPHER_TAG_MAP = {
    "bn_0": "base_networkable_0",
    "bn_1": "base_networkable_1",
    "fov": "decrypt_fov",
    "cl_active_item": "cl_active_item",
    "inventory": "player_inventory",
    "eyes": "player_eyes",
}

# Expected operation sequences for fingerprint matching
# Used when a cipher function is NOT found by name (tag not in CIPHER_TAG_MAP)
# Maps Morphine name -> expected op sequence
# nk2: re-enabled with exact constant matching (not just pattern — pattern alone has 13 candidates)
# fov: uses ROL-value matching to distinguish from inventory ciphers
FINGERPRINT_PATTERNS = {
    "base_networkable_1": ["rol", "xor", "add"],
    "decrypt_fov": ["rol", "sub", "xor"],
}

# Last known good decrypt constants (Morphine build 23824285, confirmed working).
# Used by pick_best_variant() to cross-check sha-dumper variants when .dat is locked.
FALLBACK_DECRYPT_OPS = {
    "base_networkable_0": [("rol", 2), ("xor", 0x111B9118), ("add", 0x79300E2E)],
    "base_networkable_1": [("rol", 6), ("xor", 0xC5D748E1), ("add", 0x48498B34)],
    "cl_active_item": [("add", 0x9420FF13), ("rol", 16), ("add", 0xEDC489FD), ("rol", 6)],
    "decrypt_fov": [("rol", 31), ("sub", 0x270C779), ("xor", 0x93DAED41)],
    "player_inventory": [("rol", 30), ("sub", 0x2D9831F6), ("xor", 0xDBFF84AD)],
    "player_eyes": [("sub", 0x21A1F11F), ("xor", 0x749EF0FA), ("rol", 14), ("add", 0x3CA56202)],
}

# Static pointer field -> Morphine offset/klass_rva name
STATIC_PTR_MAP = {
    "basenetworkable": "BaseNetworkable",
    "main_camera": "MainCamera",
    "tod_sky": "TOD_Sky",
    "convar_graphics": "convar_graphics",
    "effect_network": "effect_network",
    "ui_loading_screen": "SingletonComponent_UI_LoadingScreen",
    "mixer_snapshot_mgr": "Class_SingletonComponent_MixerSnapshotManager__c",
    "convar_admin": "convar_admin",
    "gchandle_array": "il2cpphandle",
}

# Curated section -> Morphine struct name
CURATED_SECTIONS = {
    "base_networkable": "base_networkable",
    "base_player": "base_player",
    "base_combat_entity": "base_combat_entity",
    "base_entity": "base_entity",
    "player_eyes": "player_eyes",
    "base_movement": "base_movement",
    "item": "item",
    "item_definition": "item_definition",
    "item_container": "item_container",
    "player_inventory": "player_inventory",
}

# Weapon section splits into two Morphine structs
WEAPON_FIELDS = {
    "owner_item_uid": ("held_entity", "ownerItemUID"),
    "primary_mag": ("base_projectile", "primaryMagazine"),
    "recoil": ("base_projectile", "recoil"),
    "mag_contents": ("magazine", "contents"),
    "mag_capacity": ("magazine", "capacity"),
}

# Camera section
CAMERA_FIELD_MAP = {
    "slot_rva": None,
}

# Native camera fields -> BaseCamera struct fields
NATIVE_CAM_MAP = {
    "field_of_view": "field_of_view",
    "view_matrix": "view_matrix",
    "projection_matrix": "projection_matrix",
    "world_position": "world_position",
    "culling_mask": "culling_mask",
}


def camel_to_snake(name: str) -> str:
    """Convert CamelCase to snake_case."""
    s1 = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", name)
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s1).lower()


def ops_to_str(ops):
    """Convert ops list to a comparable string like 'rol-xor-add'."""
    return "-".join(o["op"] for o in ops)


def ops_to_values(ops):
    """Convert ops list to a comparable tuple of (op, value) pairs."""
    return tuple((o["op"], o["value"]) for o in ops)


def pick_best_variant(tag, variants, current_values=None, fallback_ops=None):
    """Pick the best cipher variant from duplicates.

    Strategy:
    1. If only one variant -> use it (high confidence)
    2. If multiple variants and current_values provided -> pick the one matching current
    3. If multiple variants and no current match but fallback_ops -> pick matching fallback (high)
    4. If multiple variants, no match at all, fallback_ops -> use fallback directly (fallback confidence)
    5. If multiple variants, no match, no fallback -> pick first, mark low confidence
    """
    parsed = [parse_cipher_block(tag, v) for v in variants]

    if len(parsed) == 1:
        parsed[0]["confidence"] = "high"
        parsed[0]["duplicate_count"] = 1
        return parsed[0]

    # Multiple variants — try to match against current values
    if current_values:
        current_seq = ops_to_values(current_values)
        for p in parsed:
            if ops_to_values(p["ops"]) == current_seq:
                p["confidence"] = "high"
                p["duplicate_count"] = len(parsed)
                p["note"] = f"Matched current values (1 of {len(parsed)} variants)"
                return p

    # No match against current — try fallback values
    if fallback_ops:
        fallback_seq = tuple(fallback_ops) if isinstance(fallback_ops[0], tuple) else ops_to_values(fallback_ops)
        for p in parsed:
            if ops_to_values(p["ops"]) == fallback_seq:
                p["confidence"] = "high"
                p["duplicate_count"] = len(parsed)
                p["note"] = f"Matched fallback values (1 of {len(parsed)} variants)"
                return p

        # No variant matches fallback — use fallback directly to avoid wrong values
        return {
            "ops": [{"op": o[0], "value": o[1]} for o in fallback_ops] if isinstance(fallback_ops[0], tuple) else fallback_ops,
            "input_style": "pointer" if tag.startswith("bn_") else "raw",
            "return_style": "handle" if tag.startswith("bn_") else "raw",
            "read_offset": None,
            "confidence": "fallback",
            "duplicate_count": len(parsed),
            "note": f"No variant matched fallback ({len(parsed)} variants) — using last known good",
        }

    # No fallback — pick first, mark low confidence
    best = parsed[0]
    best["confidence"] = "low"
    best["duplicate_count"] = len(parsed)
    best["note"] = f"No variant matched current values ({len(parsed)} variants found)"
    best["all_variants"] = [ops_to_str(p["ops"]) for p in parsed[1:]]
    return best


def fingerprint_match(morph_name, cipher_variants, known_tags):
    """Search ALL cipher blocks (including unknown/inventory) for a matching cipher.

    Strategy:
    1. Try EXACT constant match against fallback values (highest confidence)
    2. For fov: try ROL-value match (fov uses ROL=31, distinct from inventory ROL values)
    3. For nk2: pattern-only match as last resort (low confidence)
    """
    expected_pattern = FINGERPRINT_PATTERNS.get(morph_name)
    fallback_ops = FALLBACK_DECRYPT_OPS.get(morph_name)
    fallback_seq = tuple((o[0], o[1]) for o in fallback_ops) if fallback_ops else None

    # Search ALL cipher variants (including known tags like inventory, unknown_N)
    all_tags = list(cipher_variants.keys())
    exact_matches = []
    pattern_matches = []

    for tag in all_tags:
        for variant_fields in cipher_variants[tag]:
            parsed = parse_cipher_block(tag, variant_fields)
            ops = parsed.get("ops", [])
            op_seq = [o["op"] for o in ops]

            if expected_pattern is not None and op_seq != expected_pattern:
                continue

            # Try exact constant match
            if fallback_seq:
                variant_seq = tuple((o["op"], o["value"]) for o in ops)
                if variant_seq == fallback_seq:
                    parsed["confidence"] = "high"
                    parsed["source_tag"] = tag
                    parsed["note"] = f"Exact constant match in tag '{tag}' (fallback values confirmed)"
                    exact_matches.append(parsed)
                    continue

            # Pattern matches (but not exact constants)
            parsed["confidence"] = "medium"
            parsed["source_tag"] = tag
            parsed["note"] = f"Pattern match '{'-'.join(expected_pattern)}' in tag '{tag}'"
            pattern_matches.append(parsed)

    if exact_matches:
        return exact_matches[0]

    # For fov: try ROL-value match (fov_rol=31 is distinct from inventory ROL values)
    if morph_name == "decrypt_fov" and fallback_ops:
        fov_rol = fallback_ops[0][1]  # ROL value (31)
        rol_matches = []
        for m in pattern_matches:
            if m["ops"][0]["op"] == "rol" and m["ops"][0]["value"] == fov_rol:
                m["confidence"] = "high"
                m["note"] = f"FOV cipher found via ROL={fov_rol} match in tag '{m['source_tag']}'"
                rol_matches.append(m)
        if len(rol_matches) == 1:
            return rol_matches[0]
        elif len(rol_matches) > 1:
            print(f"[FINGERPRINT] {morph_name}: {len(rol_matches)} ROL matches found — using first")

    if pattern_matches:
        return pattern_matches[0]

    return None


def main():
    cfg = load_config()
    sha_path = Path(cfg.get("sha_dumper_output", str(OUTPUT_DIR / "sha-dumper-output.txt")))

    if not sha_path.exists():
        print(f"[!] sha-dumper output not found: {sha_path}")
        return 1

    text = sha_path.read_text(encoding="utf-8", errors="ignore")
    sections, cipher_variants = parse_ini(text)

    # Build master.json-compatible data
    master = {
        "build": "sha-dumper",
        "source": "sha-dumper-only",
        "source_url": "sha-dumper (runtime dump)",
        "offsets": {},
        "structs": {},
        "il2cpp_api": {},
        "gc_handles": {},
        "klass_rvas": {},
        "decrypt_functions": {},
        "materials": {},
    }

    # 1. Parse static pointers -> offsets + klass_rvas
    sp = sections.get("static_pointers", {})
    for sha_name, morph_name in STATIC_PTR_MAP.items():
        if sha_name in sp:
            val = parse_int(sp[sha_name])
            if val:
                master["offsets"][morph_name] = val
                if morph_name in ("BaseNetworkable", "MainCamera", "TOD_Sky"):
                    master["klass_rvas"][morph_name] = val

    # 2. Parse cipher blocks -> decrypt_functions (with duplicate handling)
    # Try to load current decrypt values for comparison
    current_decrypts = load_current_decrypts(cfg)

    found_tags = set()
    for tag, variants in cipher_variants.items():
        morph_name = CIPHER_TAG_MAP.get(tag, tag)
        if morph_name == tag and tag not in CIPHER_TAG_MAP:
            # Unknown tag — skip for now (fingerprint matching handles these)
            continue

        current_vals = current_decrypts.get(morph_name)
        fallback_ops = FALLBACK_DECRYPT_OPS.get(morph_name)
        best = pick_best_variant(tag, variants, current_vals, fallback_ops)
        master["decrypt_functions"][morph_name] = best
        found_tags.add(tag)

    # 2b. Fingerprint matching for missing cipher functions
    for morph_name, expected_pattern in FINGERPRINT_PATTERNS.items():
        if morph_name in master["decrypt_functions"]:
            continue  # Already found by name

        match = fingerprint_match(morph_name, cipher_variants, CIPHER_TAG_MAP.keys())
        if match:
            master["decrypt_functions"][morph_name] = match
            print(f"[FINGERPRINT] {morph_name}: {match.get('note', 'matched')}")
        else:
            print(f"[WARN] {morph_name}: not found by name or fingerprint — using embedded defaults")

    # 2c. Store unknown cipher count for reference
    unknown_count = len([t for t in cipher_variants if t not in CIPHER_TAG_MAP])
    if unknown_count > 0:
        master["unknown_cipher_count"] = unknown_count

    # 3. Parse curated sections -> structs (NEVER emit 0x0 values)
    for sec_name, morph_struct in CURATED_SECTIONS.items():
        if sec_name not in sections:
            continue
        struct_fields = {}
        for key, val in sections[sec_name].items():
            parsed_val = parse_int(val)
            if parsed_val != 0:  # Skip 0x0 — name obfuscation, not real offset
                struct_fields[key] = parsed_val
        if struct_fields:
            master["structs"][morph_struct] = struct_fields
        else:
            print(f"[WARN] {sec_name}: all fields resolved to 0x0 (name obfuscation) — keeping current offsets")

    # 4. Parse weapon section
    if "weapon" in sections:
        for key, val in sections["weapon"].items():
            if key in WEAPON_FIELDS:
                struct_name, field_name = WEAPON_FIELDS[key]
                parsed_val = parse_int(val)
                if parsed_val != 0:
                    if struct_name not in master["structs"]:
                        master["structs"][struct_name] = {}
                    master["structs"][struct_name][field_name] = parsed_val

    # 5. Parse camera section
    if "camera" in sections:
        cam_fields = {}
        for key, val in sections["camera"].items():
            if key == "slot_rva":
                v = parse_int(val)
                if v:
                    master["offsets"]["MainCamera"] = v
                    master["klass_rvas"]["MainCamera"] = v
            elif key == "cam_object":
                v = parse_int(val)
                if v:
                    cam_fields["camera_object"] = v
        if cam_fields:
            master["structs"]["camera"] = cam_fields

    # 6. Parse native camera struct scan (heuristic — NOT auto-patched, reference only)
    # The struct scan uses orthonormality checks but still produces false positives.
    # Stored as base_camera_sha_dumper for reference only.
    if "native_camera" in sections:
        nc_fields = {}
        for sha_name, morph_name in NATIVE_CAM_MAP.items():
            if sha_name in sections["native_camera"]:
                v = parse_int(sections["native_camera"][sha_name])
                if v:
                    nc_fields[morph_name] = v
        if nc_fields:
            master["structs"]["base_camera_sha_dumper"] = nc_fields
            print(f"[PARSE] native_camera (struct scan): stored as base_camera_sha_dumper (reference only)")

    # 6b. Parse camera icall offsets (from disassembling Camera property getter functions)
    # This is the AUTHORITATIVE source for camera native struct offsets — the offset
    # is extracted directly from the engine's own mov instructions (e.g., movss xmm0, [rcx+0x170]).
    # When resolved=1, offsets are trustworthy and stored as "base_camera" for auto-patching.
    if "camera_icall_offsets" in sections:
        icall_fields = {}
        for sha_name, morph_name in NATIVE_CAM_MAP.items():
            if sha_name in sections["camera_icall_offsets"]:
                v = parse_int(sections["camera_icall_offsets"][sha_name])
                if v:
                    icall_fields[morph_name] = v

        icall_resolved = 0
        if "resolved" in sections["camera_icall_offsets"]:
            icall_resolved = int(parse_int(sections["camera_icall_offsets"]["resolved"]) or 0)

        if icall_fields:
            if icall_resolved:
                master["structs"]["base_camera"] = icall_fields
                print(f"[PARSE] camera_icall_offsets: resolved={icall_resolved} — storing as base_camera (AUTO-PATCH ENABLED — offsets from engine disassembly)")
            else:
                master["structs"]["base_camera_sha_dumper"] = icall_fields
                print(f"[PARSE] camera_icall_offsets: resolved={icall_resolved} — storing as base_camera_sha_dumper (not fully resolved)")

    # 7. Parse TOD sky instances
    if "tod_sky_instances" in sections:
        tsi = sections["tod_sky_instances"]
        if "instances_offset" in tsi:
            val = parse_int(tsi["instances_offset"])
            if val:
                if "tod_sky_static" not in master["structs"]:
                    master["structs"]["tod_sky_static"] = {}
                master["structs"]["tod_sky_static"]["instances"] = val

    # 8. Parse base_networkable section -> top-level offsets
    if "base_networkable" in sections:
        bn = sections["base_networkable"]
        if "slot_rva" in bn:
            v = parse_int(bn["slot_rva"])
            if v:
                master["offsets"]["BaseNetworkable"] = v
                master["klass_rvas"]["BaseNetworkable"] = v
        if "wrapper_off" in bn:
            v = parse_int(bn["wrapper_off"])
            if v:
                master["offsets"]["entity_list_wrapper"] = v
        if "parent_off" in bn:
            v = parse_int(bn["parent_off"])
            if v:
                master["offsets"]["entity_list_parent"] = v
        if "entities" in bn:
            v = parse_int(bn["entities"])
            if v:
                master["offsets"]["entity_list"] = v

    # 9. Parse bulk class dumps -> structs
    SKIP_SECTIONS = set(CURATED_SECTIONS.keys()) | {
        "static_pointers", "camera", "native_camera", "tod_sky_instances",
        "unity_engine_constants", "weapon", "base_networkable", "materials",
    }

    for sec_name, fields in sections.items():
        if sec_name in SKIP_SECTIONS:
            continue
        snake_name = camel_to_snake(sec_name)
        struct_fields = {}
        for key, val in fields.items():
            parsed_val = parse_int(val)
            if parsed_val != 0:  # Never emit 0x0
                struct_fields[key.strip()] = parsed_val
        if struct_fields:
            master["structs"][sec_name] = struct_fields
            if snake_name != sec_name:
                master["structs"][snake_name] = struct_fields

    # 10. Parse materials
    if "materials" in sections:
        for key, val in sections["materials"].items():
            v = parse_int(val)
            if v:
                master["materials"][key.strip()] = v

    # 11. Merge with existing master.json if present
    master_json_path = OUTPUT_DIR / "master.json"
    if master_json_path.exists():
        existing = json.loads(master_json_path.read_text(encoding="utf-8"))
        for k in ("offsets", "structs", "klass_rvas", "decrypt_functions", "materials"):
            for sub_k, sub_v in master[k].items():
                if sub_k not in existing[k]:
                    existing[k][sub_k] = sub_v
                elif isinstance(sub_v, dict) and isinstance(existing[k][sub_k], dict):
                    for fk, fv in sub_v.items():
                        if fk not in existing[k][sub_k]:
                            existing[k][sub_k][fk] = fv
        if existing.get("build", "unknown") != "unknown":
            master = existing
            master["source"] = "morphine+sha-dumper"
        else:
            master["build"] = existing.get("build", "sha-dumper")
            master["source"] = "sha-dumper-only"
        print(f"[OK] Merged sha-dumper data into existing master.json")
    else:
        print(f"[OK] Created master.json from sha-dumper (Morphine fallback mode)")

    # Write master.json
    master_json_path.write_text(json.dumps(master, indent=2), encoding="utf-8")

    print(f"    {len(master['offsets'])} top-level offsets")
    print(f"    {len(master['structs'])} structs")
    print(f"    {len(master['klass_rvas'])} klass RVAs")
    print(f"    {len(master['decrypt_functions'])} decrypt functions")
    print(f"    {master.get('unknown_cipher_count', 0)} unknown cipher functions")
    print(f"    {len(master['materials'])} materials")

    # Print confidence summary
    high = sum(1 for d in master["decrypt_functions"].values() if d.get("confidence") == "high")
    medium = sum(1 for d in master["decrypt_functions"].values() if d.get("confidence") == "medium")
    low = sum(1 for d in master["decrypt_functions"].values() if d.get("confidence") == "low")
    none = sum(1 for d in master["decrypt_functions"].values() if "confidence" not in d)
    print(f"    Confidence: {high} high, {medium} medium, {low} low, {none} unmarked")

    print(f"[OK] Wrote {master_json_path}")
    return 0


def load_current_decrypts(cfg):
    """Load current decrypt constants from C:\\rust_decrypts.dat or OffsetManager.hpp."""
    current = {}

    # Try C:\rust_decrypts.dat first
    dat_path = Path(cfg.get("decrypts_dat", r"C:\rust_decrypts.dat"))
    if dat_path.exists():
        try:
            text = dat_path.read_text(encoding="utf-8", errors="ignore")
            # Parse key=value format, group by prefix
            pending_prefix = None
            pending_ops = []
            for line in text.splitlines():
                line = line.strip()
                if line.startswith("#") or not line:
                    if pending_prefix and pending_ops:
                        current[pending_prefix] = [{"op": o[0], "value": o[1]} for o in pending_ops]
                    pending_prefix = None
                    pending_ops = []
                    # Check for prefix in comment
                    if "prefix=" in line:
                        import re
                        m = re.search(r'prefix=(\w+)', line)
                        if m:
                            pending_prefix = m.group(1)
                    continue
                if "=" in line:
                    key, _, val = line.partition("=")
                    key = key.strip()
                    val = val.strip()
                    # Extract op type from key (e.g., nk_rol -> rol, cla_add_2 -> add)
                    parts = key.split("_")
                    if len(parts) >= 2:
                        prefix = parts[0]
                        op_type = "_".join(parts[1:])
                        # Normalize: remove _2, _3 suffixes
                        op_type = re.sub(r'_\d+$', '', op_type)
                        parsed_val = parse_int(val)
                        if pending_prefix is None or pending_prefix == prefix:
                            pending_prefix = prefix
                            pending_ops.append((op_type, parsed_val))
            if pending_prefix and pending_ops:
                current[pending_prefix] = [{"op": o[0], "value": o[1]} for o in pending_ops]
        except Exception as e:
            print(f"[WARN] Could not parse {dat_path}: {e}")

    # Map prefixes to Morphine names
    prefix_map = {
        "nk": "base_networkable_0",
        "nk2": "base_networkable_1",
        "cla": "cl_active_item",
        "fov": "decrypt_fov",
        "inv": "player_inventory",
        "ey": "player_eyes",
    }

    result = {}
    for prefix, morph_name in prefix_map.items():
        if prefix in current:
            result[morph_name] = current[prefix]

    # If .dat not available (locked while Rust running), use fallback values
    if not result:
        print("[INFO] C:\\rust_decrypts.dat not available (locked or missing) — using fallback for variant matching")
        for morph_name, fb_ops in FALLBACK_DECRYPT_OPS.items():
            result[morph_name] = [{"op": o[0], "value": o[1]} for o in fb_ops]

    return result


if __name__ == "__main__":
    sys.exit(main())
