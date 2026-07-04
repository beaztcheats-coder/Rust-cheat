#!/usr/bin/env python3
"""
generate_verification_report.py

Generates a comprehensive verification report showing ALL offsets, decrypts,
encrypts, and status info for the cheat — in one readable file.

Run after compare_and_patch.py. Output: output/verification_report.txt
"""
import json
import re
import sys
from pathlib import Path
from datetime import datetime

SCRIPT_DIR = Path(__file__).parent
OUTPUT_DIR = SCRIPT_DIR / "output"
MASTER_JSON = OUTPUT_DIR / "master.json"
DECRYPT_ALGORITHMS_JSON = OUTPUT_DIR / "decrypt_algorithms.json"
OFFSETS_PATCH = OUTPUT_DIR / "offsets_patch.txt"
RUST_DECRYPTS_DAT = OUTPUT_DIR / "rust_decrypts.dat"
SHA_DUMPER_OUTPUT = OUTPUT_DIR / "sha-dumper-output.txt"
OFFSETS_HPP = Path(__file__).parent.parent.parent / "offsets.hpp"
REPORT_FILE = OUTPUT_DIR / "verification_report.txt"

# Decrypt function display names and their dat-key prefixes
DECRYPT_NAMES = {
    "base_networkable_0": ("networkable_key", "nk"),
    "base_networkable_1": ("networkable_key2", "nk2"),
    "cl_active_item": ("decrypt_ClActiveItem", "cla"),
    "player_inventory": ("decrypt_inventory_pointer", "inv"),
    "player_eyes": ("decrypt_eyes", "ey"),
    "decrypt_fov": ("decrypt_fov", "fov"),
}


def load_json(path):
    if not path.exists():
        return {}
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except:
        return {}


def load_text(path):
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="ignore")


def parse_offsets_hpp():
    """Parse offsets.hpp to get all namespace/field/value pairs."""
    content = load_text(OFFSETS_HPP)
    result = {}  # {namespace: [(field, value, comment)]}
    current_ns = None
    for line in content.split("\n"):
        line = line.strip()
        ns_match = re.match(r'namespace\s+(\w+)', line)
        if ns_match:
            current_ns = ns_match.group(1)
            if current_ns not in result:
                result[current_ns] = []
            continue
        if current_ns and current_ns != "color":
            field_match = re.match(r'(?:inline|constexpr)\s+\w+\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)', line)
            if field_match:
                name = field_match.group(1)
                val_str = field_match.group(2)
                val = int(val_str, 16) if val_str.startswith("0x") else int(val_str)
                comment = ""
                cmt = line.split("//", 1)
                if len(cmt) > 1:
                    comment = cmt[1].strip()
                result[current_ns].append((name, val, comment))
    return result


def parse_changed_offsets():
    """Parse offsets_patch.txt to get changed offsets."""
    content = load_text(OFFSETS_PATCH)
    changes = []
    for line in content.split("\n"):
        m = re.match(r'//\s*(\w+)\s+\(namespace:\s*(\w+)\):\s*(0x[0-9A-Fa-f]+)\s*->\s*(0x[0-9A-Fa-f]+)', line)
        if m:
            changes.append({
                "field": m.group(1),
                "namespace": m.group(2),
                "old": m.group(3),
                "new": m.group(4),
            })
    return changes


def parse_rust_decrypts():
    """Parse rust_decrypts.dat for final decrypt constants."""
    content = load_text(RUST_DECRYPTS_DAT)
    result = {}
    for line in content.split("\n"):
        line = line.strip()
        if line.startswith("#") or "=" not in line:
            continue
        key, _, val_str = line.partition("=")
        key = key.strip()
        val_str = val_str.strip()
        try:
            val = int(val_str, 16) if val_str.startswith("0x") else int(val_str)
            result[key] = val
        except:
            pass
    return result


def parse_sha_dumper_statics():
    """Parse sha-dumper output for static pointers and BN chain."""
    content = load_text(SHA_DUMPER_OUTPUT)
    statics = {}
    bn_chain = {}
    camera = {}
    mesh_info = {}

    # [static_pointers]
    m = re.search(r'\[static_pointers\]([\s\S]*?)(?=\n\[|\Z)', content)
    if m:
        for line in m.group(1).strip().split("\n"):
            kv = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', line.strip())
            if kv:
                statics[kv.group(1)] = int(kv.group(2), 16)

    # [base_networkable]
    m = re.search(r'\[base_networkable\]([\s\S]*?)(?=\n\[|\Z)', content)
    if m:
        for line in m.group(1).strip().split("\n"):
            kv = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', line.strip())
            if kv:
                bn_chain[kv.group(1)] = int(kv.group(2), 16)

    # [native_camera]
    m = re.search(r'\[native_camera\]([\s\S]*?)(?=\n\[|\Z)', content)
    if m:
        for line in m.group(1).strip().split("\n"):
            kv = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', line.strip())
            if kv:
                camera[kv.group(1)] = int(kv.group(2), 16)

    # Mesh
    mesh_path = OUTPUT_DIR / "rust_mesh.tri"
    if mesh_path.exists():
        mesh_info["triangles"] = mesh_path.stat().st_size // 36
        mesh_info["size_mb"] = mesh_path.stat().st_size / (1024 * 1024)
        mesh_info["status"] = "OK"
    else:
        mesh_info["status"] = "MISSING"

    return statics, bn_chain, camera, mesh_info


def format_decrypt_ops(ops_raw):
    """Format ops_raw [[op, val], ...] as a human-readable chain."""
    parts = []
    for op_type, val in ops_raw:
        if op_type == "rol" or op_type == "ror":
            parts.append(f"{op_type.upper()} {val}")
        else:
            parts.append(f"{op_type.upper()} 0x{val:08X}")
    return " -> ".join(parts)


def format_encrypt_ops(ops_raw):
    """Format the reverse (encrypt) of a decrypt chain.

    Rules:
    - Reverse the order
    - ADD <-> SUB (inverse)
    - XOR stays XOR (self-inverse)
    - ROL -> ROR (inverse)
    """
    inverse = {"add": "sub", "sub": "add", "xor": "xor", "rol": "ror", "ror": "rol"}
    parts = []
    for op_type, val in reversed(ops_raw):
        inv_type = inverse.get(op_type, op_type)
        if inv_type in ("rol", "ror"):
            if inv_type == "ror":
                parts.append(f"ROR {val}")
            else:
                parts.append(f"ROL {val}")
        else:
            parts.append(f"{inv_type.upper()} 0x{val:08X}")
    return " -> ".join(parts)


def load_frida_validated_fields():
    """Load Frida-validated field names from hash mapping + TYPE_MATCH.
    Returns a set of (namespace, field_name) tuples that are Frida-validated."""
    validated = set()
    
    # From hash mapping
    hash_mapping_path = OUTPUT_DIR / "field_hash_mapping.json"
    if hash_mapping_path.exists():
        hm = load_json(hash_mapping_path)
        if isinstance(hm, dict):
            for ns, fields in hm.items():
                if isinstance(fields, dict):
                    for fhash, info in fields.items():
                        if isinstance(info, dict):
                            name = info.get("name", "")
                            if name and not name.startswith("bodyAngles_old"):
                                validated.add((ns, name))
    
    # From TYPE_MATCH in apply_frida_offsets.py (hardcoded list)
    type_matched_fields = {
        "PlayerEyes", "PlayerInput", "PlayerModel", "BaseMovement",
        "PlayerInventory", "PlayerRigidbody", "PlayerFlags",
        "Lifestate", "model", "bounds", "bodyAngles",
        "SkinnedMultiMesh", "loot", "boneTransforms",
        "recoil", "primaryMagazine", "viewModel", "NewRecoilOverride",
        "CycleParameters", "AtmosphereParameters", "DayParameters",
        "NightParameters", "SunParameters", "MoonParameters",
        "StarParameters", "Clouds", "AmbientParameters",
    }
    # These fields are matched by unique type in their respective namespaces
    type_match_ns = {
        "PlayerEyes": "BasePlayer",
        "PlayerInput": "BasePlayer",
        "PlayerModel": "BasePlayer",
        "BaseMovement": "BasePlayer",
        "PlayerInventory": "BasePlayer",
        "PlayerRigidbody": "BasePlayer",
        "PlayerFlags": "BasePlayer",
        "Lifestate": "BaseCombatEntity",
        "model": "BaseEntity",
        "bounds": "BaseEntity",
        "bodyAngles": "PlayerInput",
        "SkinnedMultiMesh": "PlayerModel",
        "loot": "PlayerInventory",
        "boneTransforms": "Model",
        "recoil": "BaseProjectile",
        "primaryMagazine": "BaseProjectile",
        "viewModel": "HeldEntity",
        "NewRecoilOverride": "RecoilProperties",
        "CycleParameters": "TOD_Sky",
        "AtmosphereParameters": "TOD_Sky",
        "DayParameters": "TOD_Sky",
        "NightParameters": "TOD_Sky",
        "SunParameters": "TOD_Sky",
        "MoonParameters": "TOD_Sky",
        "StarParameters": "TOD_Sky",
        "Clouds": "TOD_Sky",
        "AmbientParameters": "TOD_Sky",
    }
    for field, ns in type_match_ns.items():
        validated.add((ns, field))
    
    return validated


def determine_freshness(name, ns, comment, current_build, frida_validated):
    """Determine the freshness/source of an offset.
    Returns a short tag for the FRESHNESS column."""
    # Check if Frida-validated (cross-validated)
    if (ns, name) in frida_validated:
        return "CROSS-VAL"
    
    # Check comment for source indicators
    cmt_lower = comment.lower()
    
    # Current build verified
    if current_build and current_build in comment:
        return "FRESH"
    
    # Specific sources
    if "morphine" in cmt_lower:
        return "MORPHINE"
    if "uc" in cmt_lower or "v1mper" in cmt_lower or "temopzso" in cmt_lower:
        return "UC-FB"
    if "fefe4444" in cmt_lower:
        return "SHA-DUMP"
    if "old" in cmt_lower:
        return "OLD"
    if "verify" in cmt_lower:
        return "VERIFY"
    if "sha-dumper" in cmt_lower or "validict" in cmt_lower:
        return "FRESH"
    
    # Check for any build number (might be stale)
    build_match = re.search(r'build\s+(\d+)', cmt_lower)
    if build_match:
        build_num = build_match.group(1)
        if current_build and build_num != current_build:
            return "STALE"
        return "FRESH"
    
    return "UNKNOWN"


def main():
    master = load_json(MASTER_JSON)
    capstone = load_json(DECRYPT_ALGORITHMS_JSON)
    offsets_hpp = parse_offsets_hpp()
    changed = parse_changed_offsets()
    decrypts_dat = parse_rust_decrypts()
    statics, bn_chain, camera, mesh_info = parse_sha_dumper_statics()
    frida_validated = load_frida_validated_fields()

    build = master.get("build", "unknown")
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    lines = []
    L = lines.append

    L("=" * 72)
    L("  RUST CHEAT - FULL VERIFICATION REPORT")
    L(f"  Build: {build} | Generated: {now}")
    L("=" * 72)
    L("")

    # === STATIC POINTERS ===
    L("=== STATIC POINTERS ===")
    ptr_fields = [
        ("basenetworkable_pointer", "BaseNetworkable"),
        ("camera_pointer", "MainCamera"),
        ("TOD_Sky_TypeInfo", "TOD_Sky"),
        ("Il2cppGetHandle", "GCHandle Array"),
        ("EffectNetwork_Pointer", "EffectNetwork"),
        ("Class_SingletonComponent_UI_LoadingScreen", "UI_LoadingScreen"),
        ("Class_SingletonComponent_MixerSnapshotManager__c", "MixerSnapshotMgr"),
    ]
    ns_offsets = offsets_hpp.get("offsets", [])
    ns_dict = {name: val for name, val, _ in ns_offsets}

    sha_names = {
        "BaseNetworkable": "basenetworkable",
        "MainCamera": "main_camera",
        "TOD_Sky": "tod_sky",
    }

    # Load sig scan results for cross-validation
    sig_results_path = OUTPUT_DIR / "sig_scan_results.json"
    sig_results = load_json(sig_results_path) if sig_results_path.exists() else {}

    resolved_count = 0
    for field_name, sha_name in ptr_fields:
        hpp_val = ns_dict.get(field_name, 0)
        sha_val = statics.get(sha_names.get(sha_name, sha_name.lower()), 0)
        sig_val = sig_results.get(field_name, {}).get("rva", 0)

        if hpp_val:
            resolved_count += 1
            sources = []
            if sha_val and sha_val == hpp_val:
                sources.append("sha-dumper")
            if sig_val and sig_val == hpp_val:
                sources.append("sig-scan")
            if not sources:
                sources.append("offsets.hpp")
            src = "+".join(sources)
            L(f"  {field_name:50s} 0x{hpp_val:08X}  [{src}]")
        else:
            L(f"  {field_name:50s} MISSING              [WARNING]")
    L(f"  Resolved: {resolved_count}/{len(ptr_fields)}")
    L("")

    # === DECRYPT ALGORITHMS ===
    capstone_algos = {a["name"]: a for a in capstone.get("algorithms", [])}
    master_decrypts = master.get("decrypt_functions", {})

    exact_count = 0
    total_count = 0
    L("=== DECRYPT ALGORITHMS ===")
    for canonical, (display_name, prefix) in DECRYPT_NAMES.items():
        total_count += 1
        algo = capstone_algos.get(canonical)
        master_fn = master_decrypts.get(canonical, {})

        if algo:
            ops_raw = algo.get("ops_raw", [])
            source = algo.get("source", "unknown")
            rva = algo.get("rva", "?")
            warning = algo.get("warning")
        elif master_fn:
            ops_raw = master_fn.get("ops_raw", [])
            source = master_fn.get("source", "morphine")
            rva = master_fn.get("read_offset", "?")
            warning = None
        else:
            L(f"  {display_name} ({prefix}):")
            L(f"    MISSING - will use FALLBACK_DECRYPT_OPS")
            L("")
            continue

        if not ops_raw:
            # Try to reconstruct from ops (string format)
            ops_list = master_fn.get("ops", [])
            ops_raw = []
            for op in ops_list:
                val = op.get("value", 0)
                if isinstance(val, str):
                    val = int(val, 16) if val.startswith("0x") else int(val)
                ops_raw.append((op.get("op", "?"), val))

        confidence = "HIGH"
        if "exact" in source:
            exact_count += 1
        elif "structural" in source:
            confidence = "MEDIUM"
        elif "fallback" in source or "morphine" in source:
            confidence = "MEDIUM"
        else:
            confidence = "MEDIUM"

        L(f"  {display_name} ({prefix}):")
        L(f"    Decrypt:  {format_decrypt_ops(ops_raw)}")
        L(f"    Encrypt:  {format_encrypt_ops(ops_raw)}")
        L(f"    Source: {source}    Confidence: {confidence}    RVA: {rva}")
        if warning:
            L(f"    WARNING: {warning}")
        L("")

    L(f"  Exact matches: {exact_count}/{total_count}")
    L("")

    # === DECRYPT CONSTANTS (from rust_decrypts.dat) ===
    L("=== DECRYPT CONSTANTS (rust_decrypts.dat) ===")
    if decrypts_dat:
        current_prefix = None
        for key in sorted(decrypts_dat.keys()):
            val = decrypts_dat[key]
            prefix_part = key.split("_")[0]
            if prefix_part != current_prefix:
                current_prefix = prefix_part
                display = next((d for c, d in DECRYPT_NAMES.values() if d == prefix_part), prefix_part)
                L(f"  [{display}]")
            L(f"    {key:20s} = 0x{val:08X}")
    else:
        L("  NOT GENERATED - run compare_and_patch.py first")
    L("")

    # === BN CHAIN ===
    L("=== BN CHAIN OFFSETS ===")
    bn_fields = [
        ("static_fields", "static_fields"),
        ("wrapper_class", "wrapper_off"),
        ("parent_static_fields", "parent_off"),
        ("entity", "entities"),
        ("encrypted_handle", "hv_offset"),
    ]
    for display_name, sha_key in bn_fields:
        hpp_val = 0
        for name, val, _ in offsets_hpp.get("BaseNetworkable", []):
            if name == display_name:
                hpp_val = val
                break
        sha_val = bn_chain.get(sha_key, 0)
        src = "sha-dumper" if sha_val and sha_val == hpp_val else "offsets.hpp"
        L(f"  {display_name:25s} 0x{hpp_val:04X}  [{src}]")
    L("")

    # === CAMERA OFFSETS ===
    L("=== CAMERA OFFSETS ===")
    cam_fields = [
        ("viewMatrix", "view_matrix"),
        ("projectionMatrix", "projection_matrix"),
        ("world_position", "world_position"),
        ("field_of_view", "field_of_view"),
        ("culling_mask", "culling_mask"),
    ]
    validated = camera.get("validated", 0)
    for display_name, sha_key in cam_fields:
        hpp_val = 0
        for name, val, _ in offsets_hpp.get("BaseCamera", []):
            if name == display_name:
                hpp_val = val
                break
        sha_val = camera.get(sha_key, 0)
        if sha_val and sha_val == hpp_val and validated:
            src = "sha-dumper (validated)"
        elif sha_val and validated:
            src = f"sha-dumper (differs: 0x{sha_val:X})"
        else:
            src = "uc_fallback"
        L(f"  {display_name:25s} 0x{hpp_val:04X}  [{src}]")
    L(f"  Structural validation: {'PASSED' if validated else 'FAILED (using fallback)'}")
    L("")

    # === FIELD OFFSETS (all namespaces) ===
    L("=== FIELD OFFSETS (all cheat-critical) ===")
    L("  Legend: [CROSS-VAL] Frida-verified  [FRESH] current build  [MORPHINE] old morphine")
    L("         [UC-FB] UC dump fallback  [OLD] unspecified old  [STALE] different build")
    L("")
    critical_ns = [
        "BasePlayer", "BaseCombatEntity", "BaseEntity", "BaseNetworkable",
        "BaseCamera", "PlayerEyes", "PlayerInput", "PlayerModel",
        "PlayerInventory", "ItemContainer", "Item", "ItemDefinition",
        "BaseProjectile", "HeldEntity", "Model", "ModelState",
        "TOD_Sky", "TOD_Sky_Static", "TOD_NightParameters", "TOD_AmbientParameters",
        "EffectNetwork", "convar_admin", "FOV", "BaseMovement2",
        "PlayerWalkMovement", "SkinnedMultiMesh", "RecoilProperties",
        "base_player_flags", "unity_string", "SingletonComponent",
        "MixerSnapshotManager", "UI_LoadingScreen",
    ]
    changed_fields = {c["field"] for c in changed}
    changed_ns = {c["namespace"] for c in changed}

    # Build a build number string for freshness checking
    build_str = str(build) if build != "unknown" else ""
    fresh_count = 0
    crossval_count = 0
    fallback_count = 0
    total_fields = 0

    for ns in critical_ns:
        fields = offsets_hpp.get(ns, [])
        if not fields:
            continue
        L(f"  {ns}:")
        for name, val, comment in fields:
            total_fields += 1
            changed_marker = ""
            for c in changed:
                if c["field"] == name and c["namespace"] == ns:
                    changed_marker = f" CHANGED({c['old']}->{c['new']})"
                    break
            freshness = determine_freshness(name, ns, comment, build_str, frida_validated)
            if freshness == "CROSS-VAL":
                crossval_count += 1
            elif freshness == "FRESH":
                fresh_count += 1
            else:
                fallback_count += 1
            cmt = f" // {comment}" if comment else ""
            L(f"    {name:35s} 0x{val:04X}  [{freshness:9s}]{changed_marker}{cmt}")
        L("")
    L(f"  Freshness: {crossval_count} CROSS-VAL | {fresh_count} FRESH | {fallback_count} FALLBACK/other (total {total_fields})")
    L("")

    # === GCHANDLE ===
    L("=== GCHANDLE ===")
    gc_rva = ns_dict.get("Il2cppGetHandle", 0)
    gc_api = master.get("il2cpp_api", {}).get("gchandle_get_target", 0)
    if isinstance(gc_api, str):
        gc_api = int(gc_api, 16) if gc_api.startswith("0x") else int(gc_api)
    L(f"  gchandle_get_target RVA: 0x{gc_api:08X}" if gc_api else "  gchandle_get_target RVA: not found")
    L(f"  GC handle array RVA:   0x{gc_rva:08X}" if gc_rva else "  GC handle array RVA:   not found")
    handle_dump = OUTPUT_DIR / "handle_dump.json"
    if handle_dump.exists():
        L(f"  Handle analysis:       OK (handle_dump.json exists)")
    else:
        L(f"  Handle analysis:       not run")
    L("")

    # === MESH / MATERIALS ===
    L("=== MESH / MATERIALS ===")
    if mesh_info.get("status") == "OK":
        L(f"  Mesh: {mesh_info['triangles']:,} triangles ({mesh_info['size_mb']:.1f} MB)   OK")
    else:
        L(f"  Mesh: MISSING")
    mat_count = len(master.get("materials", {}))
    if mat_count:
        L(f"  Materials: {mat_count} entries   OK")
    else:
        L(f"  Materials: not dumped")
    L("")

    # === CHANGED OFFSETS ===
    L("=== CHANGED OFFSETS THIS UPDATE ===")
    if changed:
        for c in changed:
            L(f"  {c['namespace']}::{c['field']}: {c['old']} -> {c['new']}")
    else:
        L("  (no changes)")
    L("")

    # === FRIDA STATUS ===
    L("=== FRIDA CROSS-VALIDATION ===")
    frida_json = OUTPUT_DIR / "frida_validation.json"
    if frida_json.exists():
        fj = load_json(frida_json)
        classes = fj.get("classes_dumped", 0)
        with_offsets = fj.get("classes_with_offsets", 0)
        static_rvas = fj.get("static_rvas_resolved", 0)
        L(f"  Classes dumped: {classes}")
        L(f"  Classes with offsets: {with_offsets}")
        L(f"  Static RVAs resolved: {static_rvas} (note: values may be heap pointers, use sha-dumper)")
    else:
        L(f"  Frida dump not run (optional)")
    hash_mapping = OUTPUT_DIR / "field_hash_mapping.json"
    if hash_mapping.exists():
        hm = load_json(hash_mapping)
        mapped = sum(len(v) for v in hm.values()) if isinstance(hm, dict) else 0
        L(f"  Hash mapping: {mapped} fields mapped")
    L("")

    # === SUMMARY ===
    L("=" * 72)
    L("  SUMMARY")
    L("=" * 72)
    L(f"  Static Pointers:  {resolved_count}/{len(ptr_fields)} resolved")
    L(f"  Decrypts:         {exact_count}/{total_count} exact match")
    L(f"  Field Offsets:    {total_fields} fields across {len(critical_ns)} namespaces")
    L(f"  Freshness:        {crossval_count} CROSS-VAL | {fresh_count} FRESH | {fallback_count} FALLBACK")
    L(f"  Changed Offsets:  {len(changed)}")
    L(f"  Camera:           {len(cam_fields)} offsets {'(validated)' if validated else '(fallback)'}")
    L(f"  Mesh:             {'OK' if mesh_info.get('status') == 'OK' else 'MISSING'}")
    L(f"  Materials:        {'OK' if mat_count else 'not dumped'}")
    L("")

    # Warnings
    warnings = []
    for canonical, (display_name, prefix) in DECRYPT_NAMES.items():
        algo = capstone_algos.get(canonical)
        if algo and "exact" not in algo.get("source", ""):
            warnings.append(f"  - {display_name} decrypt is not exact match (source: {algo.get('source')})")
    if not validated:
        warnings.append("  - Camera offsets using UC fallback (structural validation failed)")
    if mesh_info.get("status") != "OK":
        warnings.append("  - Mesh data missing (VisCheck will not work)")
    if fallback_count > total_fields * 0.3:
        warnings.append(f"  - {fallback_count}/{total_fields} field offsets are FALLBACK/unverified (>30%) — run Frida for cross-validation")

    if warnings:
        L("  WARNINGS:")
        for w in warnings:
            L(w)
        L("")

    L("  STATUS: READY TO APPLY" if not warnings else "  STATUS: READY TO APPLY (with warnings)")
    L("  Run: update_now.bat to apply patches and build all 4 DLLs")
    L("")
    L("  In-game verification checklist:")
    L("    [ ] ESP shows players/animals/items (validates nk + nk2 + field offsets)")
    L("    [ ] Held item name shows in ESP (validates cla + inv)")
    L("    [ ] Aimbot targets correctly (validates ey + PlayerInput)")
    L("    [ ] FOV changer works (validates fov)")
    L("    [ ] Recoil compensation works (validates BaseProjectile offsets)")
    L("")
    L("=" * 72)

    report = "\n".join(lines)
    REPORT_FILE.write_text(report, encoding="utf-8")
    print(f"[OK] Verification report written to {REPORT_FILE}")
    print(f"     {len(lines)} lines, {len(report)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
