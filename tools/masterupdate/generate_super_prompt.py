#!/usr/bin/env python3
"""Generate a comprehensive super prompt for opencode to update the cheat.

This produces a single self-contained output/super_prompt.txt that tells opencode:
1. Which automatic patches to apply (offsets, decrypt, OffsetManager, rust_decrypts.dat)
2. Which manual offsets to verify against the Frida dump
3. Which hardcoded offsets inside game-logic functions to check
4. Which algorithms it must NOT rewrite
5. How to build and verify
"""
import json
import re
from pathlib import Path
from datetime import datetime

# Last known good decrypt constants — used to flag mismatches in the super prompt
# Updated: 2026-06-30 (Morphine build 23824285)
FALLBACK_DECRYPT_OPS = {
    "base_networkable_0": [("rol", 2), ("xor", 0x111B9118), ("add", 0x79300E2E)],
    "base_networkable_1": [("rol", 6), ("xor", 0xC5D748E1), ("add", 0x48498B34)],
    "cl_active_item": [("add", 0x9420FF13), ("rol", 16), ("add", 0xEDC489FD), ("rol", 6)],
    "decrypt_fov": [("rol", 31), ("sub", 0x270C775), ("xor", 0x93DAED4D)],
    "player_inventory": [("rol", 25), ("sub", 0x249D878C), ("xor", 0x58D82066), ("add", 0x7CD2A7CE)],
    "player_eyes": [("sub", 0x0C26F5B3), ("xor", 0x6EC84F5D), ("rol", 13)],
}

CONFIG_PATH = Path(__file__).parent / "config.json"
OUTPUT_DIR = Path(__file__).parent / "output"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def load_json(path):
    p = Path(path)
    if not p.exists():
        return {}
    with open(p, "r", encoding="utf-8") as f:
        return json.load(f)


def load_master():
    path = OUTPUT_DIR / "master.json"
    if not path.exists():
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def read_text(path):
    try:
        return Path(path).read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return ""


def find_line(text, pattern):
    """Return 1-indexed line number of first regex match, or None."""
    m = re.search(pattern, text)
    if not m:
        return None
    return text.count("\n", 0, m.start()) + 1


def find_lines(text, labeled_patterns):
    """Return {label: line_number} for each pattern that matches."""
    out = {}
    for label, pat in labeled_patterns.items():
        ln = find_line(text, pat)
        if ln:
            out[label] = ln
    return out


def mapped_namespace_names(cfg):
    """Namespaces that auto_update patches automatically (from config)."""
    names = set()
    for entry in cfg.get("namespace_field_mappings", {}).values():
        ns = entry.get("namespace")
        if ns:
            names.add(ns)
    return names


def all_source_namespaces(offsets_text):
    """All `namespace X {` declarations in offsets.hpp."""
    return set(re.findall(r'\bnamespace\s+(\w+)\s*\{', offsets_text))


def extract_namespace_block(content, namespace):
    """Extract ALL namespace blocks with the given name using brace counting.
    Merges multiple declarations of the same namespace (e.g. TOD_AmbientParameters)."""
    start_pat = re.compile(rf"namespace\s+{re.escape(namespace)}\b", re.MULTILINE)
    result = ""
    for start_m in start_pat.finditer(content):
        start_idx = start_m.start()
        i = start_m.end()
        while i < len(content) and content[i] != '{':
            i += 1
        if i >= len(content):
            continue
        brace_start = i
        brace_count = 1
        i = brace_start + 1
        while i < len(content) and brace_count > 0:
            if content[i] == '{':
                brace_count += 1
            elif content[i] == '}':
                brace_count -= 1
            i += 1
        if brace_count == 0:
            result += content[start_idx:i] + "\n"
    return result


def extract_namespace_fields(content, namespace):
    """Extract {field_name: value_str} from a namespace block."""
    block = extract_namespace_block(content, namespace)
    fields = {}
    for m in re.finditer(r'(?:inline|constexpr)\s+uint64_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;', block):
        fields[m.group(1)] = m.group(2)
    return fields


def scan_hex_offsets(text, start_line, end_line=None):
    """Scan lines [start_line, end_line] for hex/decimal offset literals.
    Returns list of (line_number, offset_str, context)."""
    lines = text.splitlines()
    if end_line is None:
        end_line = len(lines)
    results = []
    for i in range(start_line - 1, min(end_line, len(lines))):
        line = lines[i]
        for m in re.finditer(r'(0x[0-9A-Fa-f]{2,})', line):
            results.append((i + 1, m.group(1), line.strip()[:120]))
    return results


def main():
    cfg = load_config()
    master = load_master()
    project_root = Path(cfg["project_root"])
    out_dir = OUTPUT_DIR

    # --- Load source files ---
    offsets_hpp = read_text(cfg["offsets_hpp"])
    sdk_hpp = read_text(cfg["sdk_hpp"])
    misc_cpp = read_text(project_root / "Hacks" / "Misc" / "misc.cpp")
    aimbot_cpp = read_text(project_root / "Hacks" / "Aimbot" / "aimbot.cpp")
    cache_cpp = read_text(project_root / "Hacks" / "Cache" / "cache.cpp")
    offsetmanager_hpp = read_text(Path(cfg["offsets_hpp"]).parent / "OffsetManager.hpp")

    # --- Load patch files (strip SAVE_BLOCK since SaveDecryptConfig is no-op) ---
    offsets_patch = read_text(out_dir / "offsets_patch.txt")
    sdk_decrypt_patch = read_text(out_dir / "sdk_decrypt_patch.txt")
    offsetmanager_patch = read_text(out_dir / "offsetmanager_patch.txt")
    save_idx = offsetmanager_patch.find("// === SAVE_BLOCK ===")
    if save_idx != -1:
        offsetmanager_patch = offsetmanager_patch[:save_idx].rstrip()
        offsetmanager_patch += "\n// (SAVE_BLOCK omitted -- SaveDecryptConfig is a no-op, do not add W(...) calls)"
    rust_decrypts_dat = read_text(out_dir / "rust_decrypts.dat")
    diff_report = read_text(out_dir / "diff_report.txt")

    # --- Determine unmapped namespaces ---
    mapped = mapped_namespace_names(cfg)
    src_ns = all_source_namespaces(offsets_hpp)
    unmapped_ns = sorted(n for n in src_ns if n not in mapped and n not in ("color",))

    # --- Find landmark line numbers ---
    sdk_landmarks = find_lines(sdk_hpp, {
        "Get_Transformation": r'\bGet_Transformation\b',
        "Get_HeldWeapon": r'\bGet_HeldWeapon\b',
        "Get_HeldItem": r'\bGet_HeldItem\b',
        "Get_Hotbar_list": r'\bGet_Hotbar_list\b',
        "Prediction2": r'\bPrediction2?\b',
        "recoil_values table": r'\brecoil_values\b',
        "bullets table": r'\bbullets\b',
        "decrypt namespace": r'\bnamespace\s+decrypt\s*\{',
    })
    cache_landmarks = find_lines(cache_cpp, {
        "NPC keyword block": r'keywordNpc',
        "animal keyword block": r'keywordAnimal',
        "player prefab check": r'playerPrefabPath',
        "world prefab block": r'AnyEspEnabled',
        "entity classification loop": r'for\s*\(auto\s+i\s*=\s*0;\s*i\s*<\s*entity_size',
    })
    misc_landmarks = find_lines(misc_cpp, {
        "ResolveCameraComponent": r'ResolveCameraComponent',
        "TOD_Sky resolver": r'ResolveTodSkyPtr',
        "FOV changer": r'FovChanger',
        "recoil brute force": r'children',
    })
    aimbot_landmarks = find_lines(aimbot_cpp, {
        "bone / target selection": r'GetBestBone|bonePool',
        "bodyAngles write": r'bodyAngles',
    })
    offsetmgr_landmarks = find_lines(offsetmanager_hpp, {
        "DecryptConfig struct": r'struct\s+DecryptConfig',
        "LOAD_DEC macro": r'#define\s+LOAD_DEC',
        "SaveDecryptConfig": r'SaveDecryptConfig',
    })

    # --- Detect dump path ---
    dump_path = cfg.get("dump_cs_path", "")
    frida_validation_path = cfg.get("frida_validation_path", "")
    has_frida_dump = Path(dump_path).exists() if dump_path else False
    has_frida_validation = Path(frida_validation_path).exists() if frida_validation_path else False

    # --- Fallback verification source: Morphine offsets.json ---
    morphine_json_path = out_dir / "offsets.json"
    has_morphine_json = morphine_json_path.exists()

    # Determine the best available verification source
    if has_frida_dump:
        verification_source = dump_path
        verification_type = "Frida runtime dump (100% accuracy)"
    elif has_morphine_json:
        verification_source = str(morphine_json_path)
        verification_type = "Morphine offsets.json (web dump, ~95% accuracy)"
    else:
        verification_source = None
        verification_type = "NO VERIFICATION SOURCE AVAILABLE"

    # --- Build number ---
    build = master.get("build", "unknown")
    source = master.get("source", "unknown")

    # --- Detect Morphine offline mode ---
    morphine_offline = (source == "sha-dumper-only" or build == "sha-dumper")

    # --- Detect mesh data availability ---
    mesh_tri_path = out_dir / "rust_mesh.tri"
    has_mesh = mesh_tri_path.exists()
    mesh_tri_count = 0
    mesh_tri_size = 0
    if has_mesh:
        mesh_tri_size = mesh_tri_path.stat().st_size
        mesh_tri_count = mesh_tri_size // 36  # sizeof(Tri) = 3 * sizeof(Vector3) = 36

    # --- Build the prompt ---
    lines = []
    a = lines.append

    a("=== COPY EVERYTHING BELOW THIS LINE INTO OPENCODE ===")
    a("")
    a("# RUST CHEAT POST-UPDATE SUPER PROMPT FOR OPENCODE")
    a(f"# Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    a(f"# Morphine/sha-dumper/Frida build: {build}")
    a(f"# Data source: {source}")
    if morphine_offline:
        a("# WARNING: MORPHINE OFFLINE — ALL DATA FROM SHA-DUMPER ONLY")
        a("# Decrypt constant confidence levels:")
        a("#   high = found by name, matches current values (reliable)")
        a("#   medium = found by fingerprint pattern matching (likely correct)")
        a("#   low = found by name but duplicates exist, no match to current (uncertain)")
        a("#   fallback = not found, using last known good values (unchanged)")
        a("")
        # Show confidence summary for decrypt functions
        dfs = master.get("decrypt_functions", {})
        if dfs:
            a("# Decrypt function status:")
            for name, info in sorted(dfs.items()):
                conf = info.get("confidence", "high")
                note = info.get("note", "")
                dup = info.get("duplicate_count", 1)
                ops = info.get("ops", [])
                op_str = " -> ".join(f"{o['op'].upper()} {hex(o['value'])}" for o in ops)
                status_icon = "OK" if conf in ("high", "fallback") else "REVIEW" if conf == "medium" else "UNCERTAIN"

                # Cross-check against fallback values
                fb = FALLBACK_DECRYPT_OPS.get(name)
                mismatch = False
                if fb:
                    actual_seq = tuple((o["op"], o["value"]) for o in ops)
                    fb_seq = tuple(fb)
                    if actual_seq != fb_seq and conf != "high":
                        mismatch = True
                        status_icon = "MISMATCH"

                a(f"#   [{status_icon}] {name}: {conf} ({dup} variant(s)) — {op_str}")
                if mismatch:
                    fb_str = " -> ".join(f"{o[0].upper()} {hex(o[1])}" for o in fb)
                    a(f"#         WARNING: sha-dumper values DIFFER from fallback!")
                    a(f"#         Fallback: {fb_str}")
                    a(f"#         If ESP breaks, use the fallback values instead.")
                if note:
                    a(f"#         Note: {note}")
            a("")

    # ================================================================
    # SECTION 1: CONTEXT
    # ================================================================
    a("# SECTION 1: CONTEXT")
    a("")
    a("You are restoring ALL features of an external Rust cheat after a Rust game update.")
    a("The auto-update pipeline has generated offset patches and decrypt function patches.")
    a("Your job is to:")
    a("  1. Apply the automatic patches (Section 2)")
    a("  2. Verify manual offsets against the Frida dump (Section 3)")
    a("  3. Check hardcoded offsets inside game-logic functions (Section 4)")
    a("  4. Handle prefab drift if needed (Section 5)")
    a("  5. Build and verify (Section 8)")
    a("")
    a(f"Project root: {project_root}")
    a(f"Target DLL flavor: {cfg.get('target_name', 'Rust Prv Ext')}")
    a(f"Build command:")
    a(f'  & "{cfg.get("msbuild_path", "msbuild")}" "{project_root / cfg.get("vcxproj", "Rust Prv Ext.vcxproj")}" /p:Configuration=Release /p:Platform=x64 /m')
    a(f"Target output: {project_root} / x64 / Release / {cfg.get('target_name', 'Rust Prv Ext')}.dll")
    a(f"GameAssembly.dll: {cfg.get('gameassembly_path', '')}")
    a("")
    if has_frida_dump:
        a(f"Authoritative Frida dump: {dump_path}")
    elif has_morphine_json:
        a(f"Verification source: {verification_source} ({verification_type})")
        a("NOTE: Frida dump not found. Using Morphine offsets.json as fallback verification source.")
        a("      For 100% accuracy, run auto_update.bat with Rust running + Frida installed.")
    else:
        a("Verification source: NONE -- locate frida_dump.txt or offsets.json before proceeding")
    if has_frida_validation:
        a(f"Frida validation offsets: {frida_validation_path}")
    a("")
    a("AGENTS.md (project root) is the authoritative project reference. Read it first.")
    a("ALL game memory access MUST go through read<T>(address) / write<T>(address) in")
    a("Driver.hpp. NEVER add direct memory dereferences (*/& on game pointers).")
    a("")

    # ================================================================
    # SECTION 2: AUTOMATIC PATCH BLOCKS
    # ================================================================
    a("================================================================")
    a("SECTION 2: AUTOMATIC PATCH BLOCKS (apply these)")
    a("================================================================")
    a("")

    # 2.1 offsets.hpp patches
    a("## 2.1 offsets.hpp changes")
    a("")
    if offsets_patch.strip():
        a("Apply the following offset changes to offsets.hpp:")
        a("```text")
        a(offsets_patch.strip())
        a("```")
    else:
        a("(no offset changes detected -- all Morphine values match current offsets.hpp)")
    a("")

    # 2.2 sdk.hpp decrypt namespace
    a("## 2.2 sdk.hpp decrypt namespace")
    a("")
    a("Replace the ENTIRE `namespace decrypt { ... }` block in sdk.hpp with the following.")
    a("This block includes Il2cppGetHandle, resolve_possible_handle, and all decrypt functions.")
    a("The decrypt functions use OffsetManager::DecryptCfg.* references (runtime-loaded constants).")
    a("")
    a("```cpp")
    a(sdk_decrypt_patch.strip() if sdk_decrypt_patch.strip() else "// (sdk_decrypt_patch.txt not found)")
    a("```")
    a("")

    # 2.3 rust_decrypts.dat
    a("## 2.3 rust_decrypts.dat")
    a("")
    a("This file is loaded at runtime by OffsetManager::LoadDecryptConfig() from C:\\rust_decrypts.dat.")
    a("Copy this content to C:\\rust_decrypts.dat (close Rust first -- file is locked while game runs):")
    a("")
    a("```text")
    a(rust_decrypts_dat.strip() if rust_decrypts_dat.strip() else "// (rust_decrypts.dat not found)")
    a("```")
    a("")

    # 2.4 OffsetManager.hpp
    a("## 2.4 OffsetManager.hpp changes")
    a("")
    a("Apply these changes to OffsetManager.hpp:")
    a("  1. Replace the DecryptConfig struct body (between { and };)")
    a("  2. Replace the LOAD_DEC block (between #define LOAD_DEC and #undef LOAD_DEC)")
    a("  3. DO NOT modify SaveDecryptConfig -- it is a no-op (disabled for anti-cheat stealth).")
    a("     Do NOT insert W(...) lambda calls. The existing code is correct.")
    a("")
    if offsetmanager_patch.strip():
        a("```text")
        a(offsetmanager_patch.strip())
        a("```")
    else:
        a("(offsetmanager_patch.txt not found)")
    a("")

    # ================================================================
    # SECTION 2.5: DECRYPT FALLBACK CHAIN
    # ================================================================
    a("================================================================")
    a("SECTION 2.5: DECRYPT CONSTANT FALLBACK CHAIN")
    a("================================================================")
    a("")
    a("Decrypt constants are sourced in priority order:")
    a("")
    a("1. MORPHINE (primary) - online offset publisher, provides all 7 decrypt functions")
    a("   If Morphine is available, ALL 20 constants are correct and verified.")
    a("   This is the normal path - Morphine is usually available.")
    a("")
    a("2. SHA-DUMPER (fallback) - injected DLL reads live game memory")
    a("   WARNING: sha-dumper may produce WRONG constants for duplicate cipher sections.")
    a("   Known: cipher_cl_active_item, cipher_eyes, cipher_inventory have duplicate sections.")
    a("   Parser keeps FIRST occurrence. cipher_fov and cipher_bn_1 are NOT emitted at all.")
    a("   If Morphine is unavailable, verify sha-dumper constants against a known-good source.")
    a("")
    a("3. EMBEDDED DEFAULTS (last resort) - hardcoded in OffsetManager.hpp lines 10-42")
    a("   These are from the PREVIOUS build and will be STALE after a game update.")
    a("   The runtime file C:\\rust_decrypts.dat overrides these if present.")
    a("")
    a("FRIDA is NOT a decrypt source - it is used ONLY for offset verification (Section 3).")
    a("If a Morphine decrypt function is missing, the patch emits a WARNING comment and the")
    a("corresponding constants will be absent from rust_decrypts.dat, causing a compile error.")
    if morphine_offline:
        a("")
        a("!!! MORPHINE OFFLINE — DECRYPT VERIFICATION REQUIRED !!!")
        a("Since Morphine is unavailable, ALL decrypt constants come from sha-dumper.")
        a("Before distributing the cheat, manually verify these constants:")
        a("  - nk_rol, nk_xor, nk_add (networkable_key) — used in BN chain")
        a("  - nk2_rol, nk2_add, nk2_xor (networkable_key2) — used in entity list")
        a("  - cla_* (ClActiveItem) — used in held item resolution")
        a("  - inv_* (inventory pointer) — used in inventory resolution")
        a("  - ey_* (PlayerEyes) — used in eye position (may be unused if component walker works)")
        a("  - fov_* (FOV) — used in FOV changer")
        a("Method: inject cheat with debug logging, check BN chain SUCCESS and held item display.")
    a("")

    # ================================================================
    # SECTION 2.6: VISCHECK MESH DATA
    # ================================================================
    a("================================================================")
    a("SECTION 2.6: VISCHECK MESH DATA (BVH Raycast)")
    a("================================================================")
    a("")
    if has_mesh:
        a(f"Mesh data: output/rust_mesh.tri")
        a(f"  Triangles: {mesh_tri_count:,}")
        a(f"  File size: {mesh_tri_size:,} bytes ({mesh_tri_size / 1024 / 1024:.1f} MB)")
        a("")
        a("Packaging: Include rust_mesh.tri in the cheat distribution alongside the DLL.")
        a("The customer's loader (admin) copies it to C:\\rust_mesh.tri before injecting the cheat.")
        a("The cheat's VisCheck loads from: C:\\rust_mesh.tri -> DLL directory -> %LOCALAPPDATA%")
        a("First load builds a BVH (~20-30s), cached to C:\\rust_mesh.bvh for instant subsequent loads.")
        a("")
        a("If the game map changes (new Rust update), regenerate mesh data:")
        a("  1. Join a Rust server (world must be loaded)")
        a("  2. Run auto_update.bat (sha-dumper dumps mesh automatically)")
        a("  3. Mesh file appears at output/rust_mesh.tri")
    else:
        a("Mesh data: NOT GENERATED")
        a("")
        a("VisCheck will fall back to PlayerModel._visible flag (~60% accuracy).")
        a("To generate mesh data for 100% accurate raycast VisCheck:")
        a("  1. Join a Rust server (world must be loaded, not main menu)")
        a("  2. Run auto_update.bat — sha-dumper dumps mesh automatically")
        a("  3. Mesh file appears at output/rust_mesh.tri")
        a("  4. Re-run update_now.bat to package with the cheat")
    a("")

    # ================================================================
    # SECTION 3: MANUAL OFFSETS VERIFICATION TABLE
    # ================================================================
    a("================================================================")
    a("SECTION 3: MANUAL OFFSETS TO VERIFY AGAINST FRIDA DUMP")
    a("================================================================")
    a("")
    a("The following namespaces in offsets.hpp are NOT auto-patched and may be stale.")
    if verification_source:
        a(f"Verify each value against: {verification_source} ({verification_type})")
    else:
        a("NO VERIFICATION SOURCE AVAILABLE -- download Morphine offsets.json or run Frida")
    if morphine_offline:
        a("")
        a("!!! MORPHINE OFFLINE — NO offsets.json CROSS-VERIFICATION AVAILABLE !!!")
        a("All offsets came from sha-dumper (runtime memory dump). These are generally")
        a("accurate, but verify critical offsets against dump.cs if available.")
        a("Priority offsets to verify: PlayerEyes, PlayerInput, ClActiveItem, BaseMovement,")
        a("PlayerFlags, BaseNetworkable static pointer, MainCamera pointer.")
    if has_frida_validation:
        a(f"Or against: {frida_validation_path}")
    a("")
    a("For each field below:")
    a("  - Read the current value in offsets.hpp")
    a("  - Search the Frida dump for the matching class and field name")
    a("  - If the value differs, update it in offsets.hpp")
    a("  - If the field does not exist in the dump, mark it TODO and report it")
    a("")

    manual_namespaces = [
        ("BaseCamera", ["viewMatrix", "projectionMatrix", "culling_mask", "world_position", "field_of_view", "wrapper_class", "entity"]),
        ("BaseNetworkable2", ["parent_entity", "prefab_id"]),
        ("ModelState", ["flags", "Flying", "OnGround", "Sleeping", "Mounted", "Ducked"]),
        ("base_player_flags", ["IsAdmin", "ReceivingSnapshot", "Sleeping", "Spectating", "Wounded", "IsDeveloper", "Connected", "ThirdPersonViewmode"]),
        ("unity_string", ["m_stringLength", "first_char"]),
        ("ItemMagazine", ["contents"]),
        ("HeldEntity", ["ownerItemUID", "viewModel"]),
        ("Model", ["boneTransforms"]),
        ("SkinnedMultiMesh", ["RendererList"]),
        ("BaseMovement2", ["admin_cheat", "target_movement"]),
        ("PlayerWalkMovement", ["GroundAngle", "GroundAngleNew", "GroundTime", "JumpTime", "LandTime", "GravityMultiplier", "TargetMovement"]),
        ("RecoilProperties", ["RecoilYawMin", "RecoilYawMax", "RecoilPitchMin", "RecoilPitchMax", "AimconeCurveScale", "NewRecoilOverride"]),
        ("BaseProjectileExt", ["camera_punches", "viewmodel_instance", "magazine", "aim_sway", "aim_sway_speed", "sight_aim_cone_scale", "hip_aim_cone_scale", "string_hold_duration_max"]),
        ("BaseEntity", ["flags", "model", "positionLerp", "objRef", "posChain0", "posChain1", "posChain2", "posChain3", "posFinal", "bounds"]),
        ("FOV", ["ConVar_Graphics", "fovField", "fovWrite", "cameraFovBypass"]),
        ("PlayerInput", ["bodyAngles", "yaw"]),
        ("PlayerEyes", ["viewOffset", "bodyRotation", "headPosition"]),
        ("EffectNetwork", ["static_fields", "instance", "hitPosition"]),
        ("convar_admin", ["convar_admin", "playerIds"]),
        ("TOD_Sky_Static", ["instances"]),
        ("TOD_Sky", ["Instances", "CycleParameters", "AtmosphereParameters", "DayParameters", "NightParameters", "SunParameters", "MoonParameters", "StarParameters", "Clouds", "AmbientParameters", "timeSinceAmbientUpdate", "timeSinceReflectionUpdate"]),
        ("TOD_AmbientParameters", ["Saturation", "UpdateInterval"]),
        ("TOD_NightParameters", ["AmbientMultiplier"]),
        ("PlayerInventory", ["Belt", "Wear", "Main", "loot", "BeltFallback1", "BeltFallback2"]),
        ("ItemContainer", ["ItemList", "ItemListFallback"]),
        ("Item", ["ItemDefinition", "ItemId", "HeldEntity_1", "Amount", "ItemIdFallback1", "ItemIdFallback2"]),
    ]

    for ns_name, fields in manual_namespaces:
        current_fields = extract_namespace_fields(offsets_hpp, ns_name)
        a(f"### {ns_name}")
        a("")
        if current_fields:
            a(f"| Field | Current Value | Action |")
            a(f"|---|---|---|")
            for field in fields:
                val = current_fields.get(field, "NOT FOUND")
                a(f"| {field} | {val} | Verify against Frida dump, update if changed |")
        else:
            a(f"(namespace not found in offsets.hpp -- may be inline elsewhere)")
        a("")

    # BaseCamera - native struct offsets (NOT auto-detected in Unity 6)
    a("### BaseCamera - Native Struct Offsets (MANUAL - Unity 6 limitation)")
    a("")
    a("**IMPORTANT:** In Unity 6, Camera native struct offsets CANNOT be auto-detected.")
    a("The icall disassembly approach was tried with 4 iterations (getters, setters,")
    a("_Injected variants, resolve_icall) - ALL icalls only access managed cached fields,")
    a("NOT the native Camera struct. The native struct is accessed through internal Unity")
    a("engine functions that are not exposed as icalls.")
    a("")
    a("When sha-dumper reports `resolved=1` in `[camera_icall_offsets]`, offsets were")
    a("auto-detected (rare - only works if Unity changes icall implementation).")
    a("When `resolved=0` (expected for Unity 6), you MUST verify camera offsets manually:")
    a("")
    a("**Step 1: Check `[camera_icall_offsets]` in sha-dumper-output.txt**")
    a("  - If resolved=1: offsets are auto-detected, skip to Step 3")
    a("  - If resolved=0: continue to Step 2")
    a("")
    a("**Step 2: Search UnknownCheats for latest Rust camera offsets**")
    a("  - Go to https://www.unknowncheats.me/forum/rust-a/")
    a("  - Search for recent posts (last 30 days) mentioning camera offsets")
    a("  - Look for posts by: fefe4444, v1mper, temopzso, sha-dumper threads")
    a("  - Search terms: 'camera offset', 'viewMatrix', 'fieldOfView', 'BaseCamera'")
    a("  - Cross-reference with the `[camera_icall_debug]` section in sha-dumper output")
    a("    (shows function bytes for manual disassembly if needed)")
    a("  - If UC has no recent posts, use the UC-confirmed values below as fallback")
    a("")
    a("UC-confirmed fallback offsets (build 23824285):")
    a("| Field | Offset | Source |")
    a("|---|---|---|")
    a("| viewMatrix | 0x2FC | fefe4444 #24841 + runtime diagnostic |")
    a("| projectionMatrix | 0x18C | old UC dump (same source as viewMatrix) |")
    a("| field_of_view | 0x170 | v1mper IL2CPP dump + temopzso #24865 |")
    a("| world_position | 0x444 | fefe4444 #24841 |")
    a("| culling_mask | 0x74 | v1mper IL2CPP dump |")
    a("")
    a("**Step 3: Verify at runtime with CAM_CHAIN diagnostic**")
    a("  (See POST-BUILD CAMERA DIAGNOSTIC section below)")
    a("")
    a("Chain offsets (auto-detected from sha-dumper - confirmed correct):")
    a("| static_fields | 0xB8 |")
    a("| wrapper_class | 0x90 |")
    a("| entity | 0x10 |")
    a("")
    a("**Also:** Camera matrix reading MUST be BEFORE the `playerList.empty()` check in")
    a("PositionRefresh. If gated by playerList, camera matrix is never read when no other")
    a("players are on server -> ALL ESP blocked. See cache.cpp PositionRefresh function.")
    a("")
    a("### Static pointers (global `offsets` namespace)")
    a("")
    a("These are critical RVA pointers that must match the current GameAssembly.dll build:")
    a("")
    static_ptrs = extract_namespace_fields(offsets_hpp, "offsets")
    if static_ptrs:
        a(f"| Pointer | Current RVA | Action |")
        a(f"|---|---|---|")
        for ptr_name in ["basenetworkable_pointer", "camera_pointer", "Il2cppGetHandle", "TOD_Sky_TypeInfo",
                         "Class_TOD_Sky_Static", "EffectNetwork_Pointer", "Class_SingletonComponent_UI_LoadingScreen",
                         "Class_SingletonComponent_MixerSnapshotManager__c",
                         "cam_typeinfo_singleton", "cam_typeinfo_camera_c"]:
            val = static_ptrs.get(ptr_name, "NOT FOUND")
            a(f"| {ptr_name} | {val} | Verify against Frida/sha-dumper output, update if changed |")
    a("")

    # ================================================================
    # SECTION 3.5: FRIDA + IL2CPPINSPECTOR CROSS-REFERENCE
    # ================================================================
    a("================================================================")
    a("SECTION 3.5: FRIDA + IL2CPPINSPECTOR CROSS-REFERENCE")
    a("================================================================")
    a("")
    a("Two independent cross-validation sources:")
    a("  1. Frida (injected runtime dump) — hash-based field matching")
    a("  2. Il2CppInspectorPro (offline metadata dump) — direct field offset extraction")
    a("")
    a("Il2CppInspectorPro reads GameAssembly.dll + global-metadata.dat directly")
    a("(no game injection needed). It gives ALL IL2CPP field offsets, TypeInfo RVAs,")
    a("and method RVAs from the authoritative metadata source.")
    a("")

    # Load Il2CppInspector cross-validation
    il2cpp_cv_path = out_dir / "il2cpp_cross_validation.txt"
    il2cpp_cv = il2cpp_cv_path.read_text(encoding="utf-8", errors="ignore") if il2cpp_cv_path.exists() else ""
    if il2cpp_cv:
        a("Il2CppInspectorPro cross-validation results:")
        for line in il2cpp_cv.splitlines():
            if line.strip():
                a(line)
        a("")
    else:
        a("Il2CppInspectorPro NOT available. Run auto_update.bat to generate.")
        a("")

    a("--- Frida runtime dump ---")
    a("")
    # Load Frida validation data
    frida_val_path = out_dir / "frida_validation.json"
    frida_val = load_json(str(frida_val_path)) if frida_val_path.exists() else {}
    frida_offsets = frida_val.get("offsets", {}) if frida_val else {}
    # Load cross-validation results
    cv_report_path = out_dir / "cross_validation_report.txt"
    cv_report = cv_report_path.read_text(encoding="utf-8", errors="ignore") if cv_report_path.exists() else ""
    frida_patches_path = out_dir / "frida_patches.txt"
    frida_patches = frida_patches_path.read_text(encoding="utf-8", errors="ignore") if frida_patches_path.exists() else ""
    if frida_offsets:
        a(f"Frida classes available: {len(frida_offsets)}")
        a(f"Target classes found: {frida_val.get('target_classes_found', 'unknown')}")
        a("")
        if cv_report:
            for line in cv_report.splitlines():
                a(line)
        a("")
        if frida_patches.strip() and "No Frida-validated" not in frida_patches:
            a("WARNING: Frida detected offset changes! Apply these patches:")
            a("```")
            a(frida_patches.strip())
            a("```")
        elif "No Frida-validated" in frida_patches:
            a("All offsets match Frida validation. No changes needed.")
        else:
            a("Frida patches file not generated — run cross_validate_offsets.py first.")
        a("")
        a("If Frida found changes, update offsets.hpp with the NEW values above.")
        a("The frida_field_mapping.json file tracks hash -> field name for future updates.")
    else:
        a("Frida validation NOT available. Run auto_update.bat with Rust in-game to generate.")
    a("")

    # ================================================================
    # SECTION 4: HARDCODED OFFSET SCANS IN GAME LOGIC
    # ================================================================
    a("================================================================")
    a("SECTION 4: HARDCODED OFFSETS INSIDE GAME-LOGIC FUNCTIONS")
    a("================================================================")
    a("")
    a("These functions contain hardcoded offsets that are NOT in the auto-patched namespaces.")
    a("Read each function at the line numbers below and verify every hardcoded offset against")
    a("the Frida dump. Replace ONLY the offset numbers -- DO NOT rewrite the algorithm.")
    a("")
    a("### 4.1 sdk.hpp game-logic functions")
    a("")
    for label, ln in sdk_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("")
    a("Key hardcoded offsets to verify in sdk.hpp:")
    a("  - Get_Transformation(): +0x10, +0x28, +0x18, +0x90 (SSE transform chain offsets)")
    a("  - Get_HeldWeapon(): children list offsets 0x10, 0x18, 0x20, inventory container offsets")
    a("  - Get_HeldItem(): uses ResolvePlayerInventory() + direct Belt(0x60) + ItemList(0x20)")
    a("  - ResolvePlayerInventory(): inv_raw+0x18 -> decrypt_inventory_pointer -> Il2cppGetHandle (primary),")
    a("    component walker GetPlayerInventory() (fallback). Do NOT change this pattern.")
    a("  - Get_Hotbar_list(): uses ResolvePlayerInventory() + direct Belt(0x60) + ItemList(0x20)")
    a("  - Get_ObjectPosition(): now uses offsets::BaseEntity::posChain0-3, posFinal (verify in Section 3)")
    a("  - decrypt functions (networkable_key, networkable_key2): use hardcoded 0x18 for GCHandle read — verify matches offsets::BaseNetworkable::encrypted_handle")
    a("  - List<T> layout: list+0x10 (array ptr), list+0x18 (size), array+0x20+i*8 (items) — from system_list namespace")
    a("  - recoil_values / bullets tables: item shortname strings (not offsets, but verify item IDs)")
    a("")

    a("### 4.2 Hacks/Misc/misc.cpp functions")
    a("")
    for label, ln in misc_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("")
    a("Key hardcoded offsets to verify in misc.cpp:")
    a("  - ResolveCameraComponent(): camera chain offsets (static_fields=0xB8, wrapper_class, entity=0x10)")
    a("  - FovChanger(): ConVar_Graphics static_fields, fovWrite offset")
    a("  - ResolveTodSkyPtr(): TOD_Sky_Static::instances=0x98, list traversal +0x10, +0x20")
    a("  - Recoil brute force: children=0x28, items=0x10, count=0x18, child+0x20+i*8")
    a("    RecoilProperties offsets (RecoilYawMin=0x18, etc.), BaseProjectile::recoil")
    a("  - DebugCam: world_position, PlayerInput::yaw, ModelState::flags, BaseMovement2::target_movement")
    a("")

    a("### 4.3 Hacks/Aimbot/aimbot.cpp functions")
    a("")
    for label, ln in aimbot_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("")
    a("Key hardcoded offsets to verify in aimbot.cpp:")
    a("  - GetBestBone(): bone index mapping (53=head, 52=neck, 20=spine1, 14=pelvis) — verify against dump.cs bone hierarchy")
    a("  - boneToCache[66] arrays (4 duplicates at lines ~114, ~206, ~269, ~504): bone-to-cache-slot mapping — verify all 4 match")
    a("  - bodyAngles write target: PlayerInput+offset (verify PlayerInput namespace)")
    a("  - Silent aim: PlayerEyes::bodyRotation offset")
    a("  - Height offsets for crouch compensation (1.65f, 1.50f, 1.20f, 0.50f)")
    a("")

    a("### 4.4 Hacks/Cache/cache.cpp functions")
    a("")
    for label, ln in cache_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("")
    a("Key hardcoded offsets to verify in cache.cpp:")
    a("  - BN chain: static_fields=0xB8, wrapper_class, parent_static_fields=0x10, entity=0x18")
    a("  - Entity array: +0x18 (size), +0x10 (array ptr), +0x20+i*8 (entities)")
    a("  - Prefab name read: object+0x10, objectClass+0x20, +0x50 (namePtr)")
    a("  - World prefab position: now uses Get_ObjectPosition() (same as players/NPCs — verify BaseEntity::posChain* in Section 3)")
    a("  - Diagnostic camera scan: uses offsets::camera_pointer + cam_typeinfo_singleton/cam_typeinfo_camera_c + BaseCamera::* constants")
    a("  - Player inventory wear: uses ResolvePlayerInventory() + PlayerInventory::Wear + ItemList(0x20)")
    a("  - Crouch flag: uses offsets::ModelState::Ducked (verify in Section 3)")
    a("")
    a("### 4.5 CRITICAL CODE CONSTRAINTS — DO NOT BREAK THESE")
    a("")
    a("These are runtime patterns that MUST be preserved when fixing offsets after an update.")
    a("Breaking any of these will cause ESP to partially or fully fail.")
    a("")
    a("1. **Camera matrix BEFORE playerList check** (cache.cpp PositionRefresh):")
    a("   The camera chain (7 IOCTLs) MUST be read BEFORE the `playerList.empty()` check.")
    a("   If gated by playerList, camera matrix is never read when no other players are on")
    a("   server -> ALL ESP blocked (NPC, animal, world, everything).")
    a("")
    a("2. **Animal lifestate read as int** (cache.cpp animal classification):")
    a("   `BaseCombatEntity::lifestate` is an int (0=Alive, 1=Dead, 2=Sleeping).")
    a("   Animal classification must read lifestate as `int` and only skip if `== 1`.")
    a("   Do NOT use `IsDead()` (reads bool, treats sleeping bears as dead).")
    a("   Code: `int lifestate = read<int>(...); bool animalDead = (lifestate == 1);`")
    a("")
    a("3. **Animal/NPC position always updated in cache merge** (cache.cpp ~line 1100):")
    a("   The merge from tempEspCache to g_EspCache must ALWAYS update headPos:")
    a("   `it->second.headPos = v.headPos;`")
    a("   Do NOT use `if (it->second.headPos.Empty())` — this freezes animal/NPC positions")
    a("   after the first cache cycle (no fast position thread for animals/NPCs).")
    a("")

    # ================================================================
    # SECTION 5: PREFAB DRIFT DISCOVERY
    # ================================================================
    a("================================================================")
    a("SECTION 5: PREFAB DRIFT DISCOVERY")
    a("================================================================")
    a("")
    a("Prefab name drift is the #1 cause of broken ESP after updates.")
    a("The cache.cpp keyword lists classify entities by prefab name substring matching.")
    a("")
    a("Current NPC keywords (cache.cpp around line %d):" % cache_landmarks.get("NPC keyword block", 361))
    a("  scientistnpc, scarecrow, scientist, tunneldweller, underwaterdweller,")
    a("  murderer, bandit_guard, peacekeeper, heavyscientist, npc/bandit, npc/scientist,")
    a("  npc/murderer, npc/scarecrow, bandit/missionprovider, npc/peacekeeper,")
    a("  npc/heavyscientist, /tunneldweller, /underwaterdweller")
    a("")
    a("Current animal keywords (cache.cpp around line %d):" % cache_landmarks.get("animal keyword block", 387))
    a("  boar, chicken, cow, deer, pig, rabbit, goat, sheep, horse, wolf, bear,")
    a("  moose, elk, buffalo, bison, goose, duck, turkey, raccoon, fox, coyote")
    a("")
    a("Current world prefab keywords (cache.cpp around line %d):" % cache_landmarks.get("world prefab block", 630))
    a("  hemp-collectable, small_stash_deployed, metal-ore, stone-ore, sulfur-ore,")
    a("  player_corpse, autoturret_deployed, guntrap, minicopter.entity,")
    a("  item_drop_backpack, bradleyapc, stone-collectable, metal-collectable,")
    a("  sulfur-collectable, wood-collectable, sam_site, beartrap, landmine,")
    a("  flameturret, ballista, catapult, locker.deployed, sleepingbag, bed_deployed,")
    a("  cupboard.tool, vending, workbench, storage_barrel, box.wooden.large,")
    a("  coffinstorage, ladder.wooden, generator, rechargable.battery,")
    a("  loot_barrel_1, loot_barrel_2, oil_barrel, crate_normal, crate_normal_2,")
    a("  crate_tools, crate_elite, codelockedhackablecrate, crate_medical, crate_food,")
    a("  rowboat, rhib, kayak, tugboat, submarine, scraptransporthelicopter,")
    a("  attackhelicopter, hotairballoon, motorbike, trike, pedalbike, snowmobile,")
    a("  shark, cargo")
    a("")
    a("### Discovery workflow:")
    a("  1. Logging is controlled by g_UpdateLoggingEnabled (set in Logger.hpp).")
    a("     If enabled, cache.cpp emits 'CACHE UNMATCHED' lines for unclassified prefabs.")
    a("     Check the flavor log: C:\\cheat_debug.log (or _bomza)")
    a("  2. Search for 'CACHE UNMATCHED' lines and extract the prefab name.")
    a("  3. Add the new prefab keyword to the appropriate keyword list in cache.cpp.")
    a("  4. If it is a new entity type, also add a toggle in offsets.hpp WORLD/NPC_ESP/ANIMAL_ESP namespace.")
    a("")

    # ================================================================
    # SECTION 6: FEATURE-PLUMBING CHECKLIST
    # ================================================================
    a("================================================================")
    a("SECTION 6: FEATURE-PLUMBING CHECKLIST")
    a("================================================================")
    a("")
    a("If you add a NEW feature toggle in offsets.hpp, you MUST also:")
    a("  1. Add SaveBool/SaveFloat/SaveColor in Config.hpp::Save()")
    a("  2. Add the matching LOAD_* in the Config::Load callback")
    a("  3. Add a menu control in Render/render.hpp")
    a("  4. Add a TR() string in Translation.hpp if user-facing")
    a("  5. Add a hotkey binding in Hotkeys.hpp if applicable")
    a("")

    # ================================================================
    # SECTION 7: ARCHITECTURE GUARDRAILS
    # ================================================================
    a("================================================================")
    a("SECTION 7: ARCHITECTURE GUARDRAILS -- DO NOT VIOLATE")
    a("================================================================")
    a("")
    a("1. NEVER generate complete files. Only output targeted patches.")
    a("")
    a("2. NEVER rewrite recoil, aimbot, TOD, camera, or BN-chain algorithms.")
    a("   These use brute-force and fallback strategies that are proven to work.")
    a("   You MAY update hardcoded offset numbers inside them, but DO NOT change the logic.")
    a("")
    a("3. ALWAYS use read<T>(address) / write<T>(address) for all game memory access.")
    a("   NEVER use direct memory dereferences (*, &, -> on game pointers).")
    a("")
    a("4. NEVER swap nk_ and nk2_ prefixes.")
    a("   nk_ = networkable_key (client_entities), nk2_ = networkable_key2 (entity_list).")
    a("")
    a("5. PlayerInventory IS wired through decrypt in ResolvePlayerInventory().")
    a("   The correct chain is: read(BasePlayer+PlayerInventory) -> +0x18 -> decrypt_inventory_pointer -> Il2cppGetHandle.")
    a("   This is the SAME pattern as networkable_key/networkable_key2 in the BN chain.")
    a("   decrypt_eyes exists but PlayerEyes uses component walker (GetComponentByName) — leave as-is.")
    a("   Do NOT change ResolvePlayerInventory() to direct decrypt or raw pointer — it will break held item.")
    a("")
    a("6. DO NOT change BrightNight (TOD_Sky_Static::instances at offset 0x98).")
    a("   This is verified by sha-dumper. If the offset changes, verify with sha-dumper first.")
    a("")
    a("7. The `resolve_possible_handle` function MUST be preserved in the decrypt namespace.")
    a("   If replacing the decrypt namespace, include this function unchanged.")
    a("")
    a("8. SaveDecryptConfig in OffsetManager.hpp is a NO-OP (disabled for anti-cheat stealth).")
    a("   Do NOT insert W(...) lambda calls. The existing empty body is correct.")
    a("")
    a("9. Decrypt constants are loaded at runtime from C:\\rust_decrypts.dat via OffsetManager.")
    a("   The .dat file keys use the format {prefix}_{op_type} (e.g., nk_rol, cla_sub).")
    a("   Generated decrypt functions use OffsetManager::DecryptCfg.* references.")
    a("   Do NOT hardcode decrypt constants in the C++ code.")
    a("")
    a("10. AGENTS.md is the authoritative reference. Read it before making changes.")
    a("    Morphine/sha-dumper/Frida are the source of truth for offset VALUES,")
    a("    but AGENTS.md describes the architecture and what NOT to change.")
    a("")
    a("11. If the Morphine build number differs from the current build,")
    a("    be extra cautious. Verify critical offsets (PlayerEyes, PlayerInput, ClActiveItem,")
    a("    BaseMovement, PlayerFlags) against the Frida dump before committing changes.")
    a("")
    a("12. C:\\rust_decrypts.dat is LOCKED while Rust is running. Close the game before")
    a("    overwriting it. apply_patches.py will warn 'file is likely locked' if it is.")
    a("")
    a("13. Prefer small, targeted edits over rewriting whole files.")
    a("    Match existing indentation (tabs in offsets.hpp, spaces in sdk.hpp).")
    a("")
    a("14. Preserve ResolvePlayerInventory() pattern in sdk.hpp.")
    a("    It resolves the inventory pointer via: inv_raw+0x18 -> decrypt_inventory_pointer -> Il2cppGetHandle.")
    a("    This is the SAME GCHandle pattern as networkable_key/networkable_key2 in the BN chain.")
    a("    Get_HeldItem(), Get_Hotbar_list(), and cache.cpp wear read all use ResolvePlayerInventory().")
    a("    Do NOT replace it with direct decrypt or raw pointer — it will break held item resolution.")
    a("")

    # ================================================================
    # SECTION 8: BUILD & VERIFICATION
    # ================================================================
    a("================================================================")
    a("SECTION 8: BUILD & VERIFICATION")
    a("================================================================")
    a("")
    a("### Build")
    a(f'  & "{cfg.get("msbuild_path", "msbuild")}" "{project_root / cfg.get("vcxproj", "Rust Prv Ext.vcxproj")}" /p:Configuration=Release /p:Platform=x64 /m')
    a(f"  Target: {project_root} / x64 / Release / {cfg.get('target_name', 'Rust Prv Ext')}.dll")
    a(f"  After each batch of edits, run the build and use compiler errors as feedback.")
    a("")
    a("### POST-BUILD CAMERA DIAGNOSTIC (CRITICAL - do this FIRST)")
    a("  Camera native struct offsets CANNOT be auto-detected in Unity 6 (see BaseCamera section).")
    a("  If you searched UnknownCheats and updated camera offsets in offsets.hpp, you MUST verify them.")
    a("  If you did NOT update camera offsets (resolved=0, using UC-confirmed fallback), still verify.")
    a("  The CAM_CHAIN diagnostic confirms correct camera operation at runtime.")
    a("")
    a("  Steps:")
    a("  1. Build the DEBUG flavor: TargetName='Rust Prv Ext_debug' (logging auto-enabled).")
    a("  2. Inject Rust Prv Ext_debug.dll into the game.")
    a("  3. Open C:\\cheat_debug.log and search for 'CAM_CHAIN'.")
    a("  4. Expected success line:")
    a("     CAM_CHAIN: OK ncam=0x... fov=65.0 v._11=0.XXXX p._11=1.0000")
    a("     - fov should be 40-90 (player FOV)")
    a("     - v._11 should be NON-ZERO (typically 0.5-0.9)")
    a("     - p._11 should be ~1.0")
    a("  5. If you see 'CAM_CHAIN: FAIL' or any of these are zero:")
    a("     - viewMatrix=0 (v._11=0): Camera viewMatrix offset is WRONG.")
    a("       Try 0x2FC (UC source). If still zero, try 0x300, 0x2F8, 0x304.")
    a("     - fov=0 or fov FAILED: fieldOfView offset is WRONG.")
    a("       Try 0x170 (UC source). If still zero, try 0x16C, 0x174.")
    a("     - worldPos=0: worldPosition offset is WRONG.")
    a("       Try 0x444 (UC source). If still zero, try 0x440, 0x448.")
    a("     - ncam=0 (native_cam): Camera chain failed at static_fields/wrapper_class/entity.")
    a("       Check Section 3.5 — chain offsets should match sha-dumper.")
    a("  6. Camera chain MUST be read BEFORE the playerList.empty() check in PositionRefresh")
    a("     (cache.cpp ~line 1158). If camera is inside the playerList gate, ALL ESP breaks")
    a("     when no other players are on the server.")
    a("")
    a("### In-game verification (do not skip)")
    a("  1. Build MUST complete with zero errors.")
    a("  2. Inject the DLL with the project loader.")
    a("  3. If logging is enabled, check the flavor log:")
    a("     - C:\\cheat_debug.log         (BeaZt)")
    a("     - C:\\cheat_debug_bomza.log   (Bomza)")
    a("  4. Verify the BN chain succeeds: look for 'BN chain SUCCESS!' + non-zero entity size.")
    a("  5. Verify ESP renders OTHER PLAYERS, NPCs, and WORLD PREFABS -- not just LocalPlayer.")
    a("     (LocalPlayer-only ESP means the BN chain or decrypt is wrong.)")
    a("  6. If ESP classification is wrong, review the log for 'CACHE UNMATCHED' prefab names")
    a("     and add them to cache.cpp (Section 5).")
    a("  7. Test aimbot: enable Memory aimbot, verify it locks onto targets and writes bodyAngles.")
    a("  8. Test recoil: enable RecoilEnabled, verify weapon recoil is reduced.")
    a("  9. Test FOV changer: enable FovChanger, verify camera FOV changes.")
    a("  10. Test BrightNight: enable, verify ambient light increases at night.")
    if has_mesh:
        a("  11. Test VisCheck: enable in BeaZt menu (Player ESP -> VisCheck).")
        a("      Verify players behind walls show as INVISIBLE (different color).")
        a(f"      BVH should load from C:\\rust_mesh.tri ({mesh_tri_count:,} triangles).")
        a("      First load takes ~20-30s (BVH build). Check menu status text.")
        a("      Subsequent loads should be instant (BVH cache at C:\\rust_mesh.bvh).")
    else:
        a("  11. VisCheck: mesh data NOT available — will use PlayerModel._visible fallback.")
        a("      To enable raycast VisCheck, run auto_update.bat with Rust in-game.")
    a("")

    # ================================================================
    # SECTION 9: EXECUTION ORDER
    # ================================================================
    a("================================================================")
    a("SECTION 9: EXECUTION ORDER")
    a("================================================================")
    a("")
    a("1. Read AGENTS.md (project root) first.")
    a("2. Apply the automatic patches from Section 2 (offsets.hpp, sdk.hpp, OffsetManager.hpp, rust_decrypts.dat).")
    if morphine_offline:
        a("   !!! MORPHINE OFFLINE: Verify decrypt constants manually (see Section 2.5 warning).")
    a("3. Verify the manual offsets in Section 3 against the Frida dump.")
    a("4. Check the hardcoded offsets in Section 4 against the Frida dump.")
    a("5. Handle any prefab drift from Section 5.")
    a("6. Build and verify per Section 8 (start with POST-BUILD CAMERA DIAGNOSTIC).")
    a("7. If mesh data exists (Section 2.6), package rust_mesh.tri with the cheat DLL.")
    a("8. Report: what you changed, what still needs testing, and any offsets you could not verify.")
    a("")
    a("=== END OF PROMPT ===")

    # --- Write output ---
    prompt = "\n".join(lines)
    out_path = out_dir / "super_prompt.txt"
    out_path.write_text(prompt, encoding="utf-8")
    print(f"[OK] Generated {out_path}")
    print(f"     {len(lines)} lines")
    print(f"     Unmapped namespaces: {len(unmapped_ns)}")
    print(f"     Frida dump available: {has_frida_dump}")
    print(f"     Frida validation available: {has_frida_validation}")


if __name__ == "__main__":
    main()
