#!/usr/bin/env python3
"""Generate a complete opencode master prompt for post-update feature restoration.

This runs after getnewoffsets.bat. It reads the current source + pipeline outputs and
produces output/opencode_update_prompt.txt -- a single ready-to-paste prompt that tells
opencode exactly which files to READ (with full paths), what the pipeline already fixed
automatically, what still needs manual work, and how to verify.

It also absorbs output/ask_opencode.txt (handle resolver prompt from analyze_handle.py)
if that file exists, so the AI has the handle analysis inline.
"""
import json
import re
from pathlib import Path
from datetime import datetime

CONFIG_PATH = Path(__file__).parent / "config.json"
OUTPUT_DIR = Path(__file__).parent / "output"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
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
    """Namespaces that getnewoffsets patches automatically (from config)."""
    names = set()
    for entry in cfg.get("namespace_field_mappings", {}).values():
        ns = entry.get("namespace")
        if ns:
            names.add(ns)
    return names


def all_source_namespaces(offsets_text):
    """All `namespace X {` declarations in offsets.hpp."""
    return set(re.findall(r'\bnamespace\s+(\w+)\s*\{', offsets_text))


def main():
    cfg = load_config()
    project_root = Path(cfg["project_root"])
    out_dir = OUTPUT_DIR

    # --- Load pipeline outputs ---
    master_json = {}
    mp = out_dir / "master.json"
    if mp.exists():
        try:
            master_json = json.loads(read_text(mp))
        except Exception:
            master_json = {}

    build = master_json.get("build", "unknown")
    source = master_json.get("source", "unknown")

    # Load capstone decrypt algorithms (authoritative — not diff_report which may have sha-dumper fingerprints)
    decrypt_algos = {}
    da_path = out_dir / "decrypt_algorithms.json"
    if da_path.exists():
        try:
            da = json.loads(read_text(da_path))
            decrypt_algos = {a["name"]: a for a in da.get("algorithms", [])}
        except Exception:
            pass

    # Load sig scanner results
    sig_results = {}
    sig_path = out_dir / "sig_scan_results.json"
    if sig_path.exists():
        try:
            sig_results = json.loads(read_text(sig_path))
        except Exception:
            pass

    # Parse verification report for FRESHNESS summary
    ver_report = read_text(out_dir / "verification_report.txt")
    freshness_line = ""
    decrypt_line = ""
    static_line = ""
    warnings_section = []
    in_warnings = False
    for line in ver_report.splitlines():
        if "Freshness:" in line and "CROSS-VAL" in line:
            freshness_line = line.strip()
        if "Decrypts:" in line and "exact match" in line:
            decrypt_line = line.strip()
        if "Static Pointers:" in line:
            static_line = line.strip()
        if "WARNINGS:" in line:
            in_warnings = True
            continue
        if in_warnings:
            if line.strip().startswith("STATUS:") or (line.strip() and not line.strip().startswith("-")):
                in_warnings = False
            elif line.strip():
                warnings_section.append(line.strip())

    diff_report = read_text(out_dir / "diff_report.txt")
    summary_text = read_text(out_dir / "summary.txt")

    # --- Detect mesh data ---
    mesh_tri_path = out_dir / "rust_mesh.tri"
    has_mesh = mesh_tri_path.exists()
    mesh_tri_count = 0
    mesh_tri_size_mb = 0
    if has_mesh:
        mesh_tri_size_mb = mesh_tri_path.stat().st_size / 1024 / 1024
        mesh_tri_count = mesh_tri_path.stat().st_size // 36

    # --- Load source files ---
    offsets_hpp = read_text(cfg["offsets_hpp"])
    sdk_hpp = read_text(cfg["sdk_hpp"])
    offsetmanager_hpp = read_text(Path(cfg["offsets_hpp"]).parent / "OffsetManager.hpp")
    cache_cpp = read_text(project_root / "Hacks" / "Cache" / "cache.cpp")
    misc_cpp = read_text(project_root / "Hacks" / "Misc" / "misc.cpp")
    aimbot_cpp = read_text(project_root / "Hacks" / "Aimbot" / "aimbot.cpp")
    visuals_cpp = read_text(project_root / "Hacks" / "Visuals" / "visuals.cpp")
    cheat_hpp = read_text(project_root / "Hacks" / "Cheat" / "Cheat.hpp")
    render_hpp = read_text(project_root / "Render" / "render.hpp")
    config_hpp = read_text(project_root / "Config.hpp")
    agents_md = read_text(project_root / "AGENTS.md")

    # --- Determine unmapped (manual) namespaces in offsets.hpp ---
    mapped = mapped_namespace_names(cfg)
    src_ns = all_source_namespaces(offsets_hpp)
    unmapped_ns = sorted(n for n in src_ns if n not in mapped and n not in ("color",))

    # --- Find landmark line numbers in source ---
    sdk_landmarks = find_lines(sdk_hpp, {
        "Get_Transformation": r'\bGet_Transformation\b',
        "Get_HeldWeapon": r'\bGet_HeldWeapon\b',
        "Get_HeldItem": r'\bGet_HeldItem\b',
        "Get_Hotbar_list": r'\bGet_Hotbar_list\b',
        "Prediction": r'\bPrediction2?\b',
        "recoil table (recoil_values)": r'recoil_values',
        "bullet table (bullets)": r'\bbullets\b',
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
        "children-list brute force": r'children',
        "TOD_Sky resolver": r'TOD_Sky',
        "FOV changer": r'FovChanger|fovWrite',
        "recoil brute scan": r'recoil',
    })
    aimbot_landmarks = find_lines(aimbot_cpp, {
        "bone / target selection": r'BoneList|bone|targetBone',
        "bodyAngles write": r'bodyAngles',
    })
    offsetmgr_landmarks = find_lines(offsetmanager_hpp, {
        "DecryptConfig struct": r'struct\s+DecryptConfig',
        "LOAD_DEC macro": r'#define\s+LOAD_DEC',
        "SaveDecryptConfig": r'SaveDecryptConfig',
    })

    # --- Absorb ask_opencode.txt if present ---
    ask_path = out_dir / "ask_opencode.txt"
    ask_content = read_text(ask_path) if ask_path.exists() else ""

    # --- Detect which pipeline output files exist ---
    output_files = [
        "diff_report.txt", "summary.txt", "offsets_patch.txt", "sdk_patch.txt",
        "sdk_decrypt_patch.txt", "offsetmanager_patch.txt", "rust_decrypts.dat",
        "master.json", "decrypt_algorithms.json", "offsets_dump.hpp",
        "verification_report.txt", "sig_scan_results.json", "sig_patterns.json",
        "frida_validation.json", "frida_decrypt_algorithms.json",
        "cross_validation_report.txt", "field_hash_mapping.json",
        "rust_mesh.tri", "rust_mesh_dump.log",
        "handle_dump.json", "handle_disasm.txt",
    ]
    present_outputs = [f for f in output_files if (out_dir / f).exists()]

    # --- Build decrypt summary from capstone (authoritative) ---
    DECRYPT_DISPLAY = {
        "base_networkable_0": ("networkable_key (nk)", "client_entities"),
        "base_networkable_1": ("networkable_key2 (nk2)", "entity_list"),
        "cl_active_item": ("decrypt_ClActiveItem (cla)", "cl_active_item"),
        "player_inventory": ("decrypt_inventory_pointer (inv)", "player_inventory"),
        "player_eyes": ("decrypt_eyes (ey)", "player_eyes"),
        "decrypt_fov": ("decrypt_fov (fov)", "decrypt_fov"),
    }

    # --- Build paths for the prompt ---
    vcxproj = cfg.get("vcxproj", "Rust Prv Ext.vcxproj")
    target_name = cfg.get("target_name", "Rust Prv Ext")
    msbuild = cfg.get("msbuild_path", "msbuild")
    gameassembly = cfg.get("gameassembly_path", "")

    lines = []
    a = lines.append

    a("# RUST CHEAT POST-UPDATE MASTER PROMPT FOR OPENCODE")
    a(f"# Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    a(f"# Build: {build}")
    a("# Pipeline: 100% Morphine-independent (sha-dumper + capstone + Frida + sig-scanner)")
    a("")
    a("You are restoring all features of an external Rust cheat after a Rust game update.")
    a("The auto-update pipeline has already run and fixed the OFFSET CONSTANTS and DECRYPT")
    a("FUNCTIONS. Your job is to verify the automatic patches, then fix everything the")
    a("pipeline CANNOT touch so that ALL features work again.")
    a("")
    a("Project root: %s" % project_root)
    a("Target DLL flavor: %s" % target_name)
    a("Build command:")
    a('  & "%s" "%s" /p:Configuration=Release /p:Platform=x64 /m' % (msbuild, project_root / vcxproj))
    a(f"Target output: {project_root} / x64 / Release / {target_name}.dll")
    if gameassembly:
        a(f"GameAssembly.dll: {gameassembly}")
    a("")
    a("AGENTS.md (project root) is the authoritative project reference. Read it first.")
    a("ALL game memory access MUST go through read<T>(address) / write<T>(address) in")
    a("Driver.hpp. NEVER add direct memory dereferences (*/& on game pointers).")
    a("")

    # --- Section: verification status (NEW) ---
    a("================================================================")
    a("SECTION 0 -- PIPELINE VERIFICATION STATUS")
    a("================================================================")
    a("This section summarizes what the pipeline verified. Green = OK, Red = investigate.")
    a("")
    if freshness_line:
        a(f"  FRESHNESS:  {freshness_line}")
    else:
        a("  FRESHNESS:  (verification report not found)")
    if decrypt_line:
        a(f"  DECRYPTS:   {decrypt_line}  (4 exact capstone + 1 structural + 1 derived = 6 total)")
    else:
        a("  DECRYPTS:   (not found in report)")
    if static_line:
        a(f"  STATICS:    {static_line}")
    else:
        a("  STATICS:    (not found in report)")
    a(f"  SIG SCAN:   {sum(1 for v in sig_results.values() if v.get('rva', 0) > 0)}/{len(sig_results)} static pointers found by signature scan")
    if warnings_section:
        a("  WARNINGS:")
        for w in warnings_section:
            a(f"    {w}")
    else:
        a("  WARNINGS:   (none)")
    a("")
    a("If any warnings above mention 'decrypt' or 'camera' or 'FALLBACK > 30%', investigate")
    a("before proceeding. Otherwise, the pipeline output is trusted and verified.")
    a("")

    # --- Section: what is automatic ---
    a("================================================================")
    a("SECTION 1 -- WHAT THE PIPELINE ALREADY FIXED AUTOMATICALLY")
    a("================================================================")
    a("These were patched by apply_patches.py (via update_now.bat) and do NOT need manual work:")
    a("- offsets.hpp: namespaces listed in config.json::namespace_field_mappings")
    a("  (BaseNetworkable, BasePlayer, BaseCombatEntity, BaseProjectile, TOD_Sky, etc.)")
    a("- sdk.hpp: the entire `namespace decrypt { ... }` block")
    a("  (Il2cppGetHandle, networkable_key/2, decrypt_ClActiveItem/eyes/fov, etc.)")
    a("- OffsetManager.hpp: DecryptConfig struct + LOAD_DEC block")
    a("- C:\\rust_decrypts.dat: runtime decrypt key/value constants")
    a("")
    a("All data sources (no Morphine needed):")
    a("  - Static pointers: sha-dumper + sig-scanner (cross-validated)")
    a("  - Field offsets: sha-dumper + Frida type-matching + hash mapping")
    a("  - Decrypt algorithms: capstone (4 exact + 1 structural + 1 derived)")
    a("  - Camera offsets: sha-dumper structural validation (or UC fallback)")
    a("  - Mesh + materials: sha-dumper")
    a("")
    a("Decrypt algorithms (from capstone — authoritative, NOT from sha-dumper HDE64):")
    for canonical, (display, func) in DECRYPT_DISPLAY.items():
        algo = decrypt_algos.get(canonical)
        if algo:
            ops = algo.get("ops_raw", [])
            src = algo.get("source", "unknown")
            rva = algo.get("read_offset", algo.get("rva", "?"))
            ops_str = " ".join(f"{o[0].upper()} 0x{o[1]:08X}" if o[0] not in ("rol","ror") else f"{o[0].upper()} {o[1]}" for o in ops)
            a(f"  - {display}: {ops_str}")
            a(f"    Source: {src}  RVA: {rva}")
        else:
            a(f"  - {display}: NOT FOUND in capstone output — investigate!")
    a("")

    # --- Section: files to READ ---
    a("================================================================")
    a("SECTION 2 -- FILES YOU MUST READ (in this order)")
    a("================================================================")
    a("Read each file below with its FULL path. Do NOT guess contents -- open them.")
    a("")
    a("Project reference:")
    a(f"  1. {project_root / 'AGENTS.md'}")
    a("")
    a("Pipeline verification (read these FIRST to check pipeline health):")
    a(f"  2. {out_dir / 'verification_report.txt'}  (FRESHNESS report — check SUMMARY + WARNINGS)")
    a(f"  3. {out_dir / 'offsets_dump.hpp'}  (consolidated Morphine-style dump — all offsets + decrypts)")
    a(f"  4. {out_dir / 'sig_scan_results.json'}  (signature-scanned static pointers)")
    a(f"  5. {out_dir / 'decrypt_algorithms.json'}  (capstone decrypt analysis — 6/6 algorithms)")
    a("")
    a("Pipeline patches (what changed this update):")
    a(f"  6. {out_dir / 'diff_report.txt'}")
    a(f"  7. {out_dir / 'summary.txt'}")
    a(f"  8. {out_dir / 'offsets_patch.txt'}")
    a(f"  9. {out_dir / 'sdk_decrypt_patch.txt'}")
    a(f" 10. {out_dir / 'offsetmanager_patch.txt'}")
    a(f" 11. {out_dir / 'rust_decrypts.dat'}")
    a(f" 12. {out_dir / 'master.json'}  (structured offset data)")
    a("")
    a("Source files you will patch manually:")
    a(f" 13. {cfg['offsets_hpp']}")
    a(f" 14. {cfg['sdk_hpp']}")
    a(f" 15. {Path(cfg['offsets_hpp']).parent / 'OffsetManager.hpp'}")
    a(f" 16. {project_root / 'Hacks' / 'Cache' / 'cache.cpp'}")
    a(f" 17. {project_root / 'Hacks' / 'Misc' / 'misc.cpp'}")
    a(f" 18. {project_root / 'Hacks' / 'Aimbot' / 'aimbot.cpp'}")
    a(f" 19. {project_root / 'Hacks' / 'Visuals' / 'visuals.cpp'}")
    a(f" 20. {project_root / 'Hacks' / 'Cheat' / 'Cheat.hpp'}")
    a(f" 21. {project_root / 'Render' / 'render.hpp'}")
    a(f" 22. {project_root / 'Config.hpp'}")
    a("")

    # --- Section: what is NOT automatic ---
    a("================================================================")
    a("SECTION 3 -- WHAT IS NOT AUTOMATIC (YOU MUST FIX THESE)")
    a("================================================================")
    a("")
    a("--- 3.1 offsets.hpp: custom offsets NOT in namespace_field_mappings ---")
    a(f"The following namespaces in offsets.hpp are NOT auto-patched and may be stale:")
    if unmapped_ns:
        for n in unmapped_ns:
            a(f"  - {n}")
    else:
        a("  (none detected)")
    a("Specific hardcoded offsets to verify against offsets_dump.hpp:")
    a("  - BaseCamera::viewMatrix, projectionMatrix, culling_mask, world_position, field_of_view")
    a("  - BaseNetworkable::children  (used by recoil brute-force + held weapon discovery)")
    a("  - ModelState::flags + the base_player_flags enum bits")
    a("  - BasePlayer::Mounted, CurrentTeam, ModelState, WeaponMoveSpeedScale,")
    a("    ClothingBlocksAiming, PlayerRigidbody, Frozen, CurrentGesture")
    a("  - Feature toggle namespaces: World, NPC_ESP, ANIMAL_ESP, RADAR, AIMBOT, MISC, BATTLE")
    a("    (toggles don't usually change, but the offsets they reference might)")
    a("")

    a("--- 3.2 sdk.hpp: game logic (NOT patched) ---")
    a("These functions contain hardcoded offsets/chase logic that the pipeline ignores:")
    for label, ln in sdk_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("Verify each against offsets_dump.hpp. Pay special attention to:")
    a("  - Get_Transformation(): SSE transform chain offsets")
    a("  - Get_HeldWeapon(): fallback offsets (0x28, 0x38, 0x10, 0x20, etc.)")
    a("  - Get_HeldItem(): inventory container chase offsets")
    a("  - recoil_values / bullets tables: item-id -> recoil mapping")
    a("")

    a("--- 3.3 Hacks/Cache/cache.cpp: entity classification (NOT patched) ---")
    a("Prefab name drift is the #1 cause of broken ESP after updates. Verify:")
    for label, ln in cache_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("After injecting, check the flavor log for 'CACHE UNMATCHED' lines and add any new")
    a("prefab names to the keyword lists. Common drift: new animals, scientist variants,")
    a("new deployables, renamed crates.")
    a("")

    a("--- 3.4 Hacks/Misc/misc.cpp: recoil/FOV/TOD (NOT patched) ---")
    for label, ln in misc_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("  - children-list brute-force weapon discovery depends on BaseNetworkable::children")
    a("  - FOV changer writes to the native camera (verify camera offsets if FOV breaks)")
    a("  - BrightNight / TOD_Sky ambient resolver may need new TOD field offsets")
    a("")

    a("--- 3.5 Hacks/Aimbot/aimbot.cpp (NOT patched) ---")
    for label, ln in aimbot_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("  - bodyAngles write target is PlayerInput+offset -- verify PlayerInput offset")
    a("  - bone index mapping if the skeleton layout changed")
    a("")

    a("--- 3.6 OffsetManager.hpp (partially patched) ---")
    for label, ln in offsetmgr_landmarks.items():
        a(f"  - {label}  (around line {ln})")
    a("  - The struct + LOAD_DEC block are auto-patched. SaveDecryptConfig may be a no-op;")
    a("    do NOT insert a W(...) lambda if the existing code is a no-op.")
    a("  - Keep nk_ constants for networkable_key and nk2_ for networkable_key2 (DO NOT swap)")
    a("  - DecryptConfig default values must match rust_decrypts.dat exactly")
    a("")

    a("--- 3.7 Render/render.hpp + Config.hpp (NOT patched) ---")
    a("  - If you add a new feature toggle in offsets.hpp, you MUST also:")
    a("    * add SaveBool/SaveFloat/SaveColor in Config.hpp::Save()")
    a("    * add the matching LOAD_* in the Config::Load callback")
    a("    * add a menu control in Render/render.hpp")
    a("    * add a TR() string in Translation.hpp if user-facing")
    a("")

    # --- Section: handle resolver (absorbed) ---
    a("================================================================")
    a("SECTION 4 -- Il2cppGetHandle / GCHandle RESOLVER (absorbed)")
    a("================================================================")
    if ask_content:
        a("The following was produced by analyze_handle.py and is ABSORBED here so you do not")
        a("need to open a separate file. If the GCHandle resolver changed, update")
        a("sdk.hpp `Il2cppGetHandle` and the `il2cpphandle` pointer in offsets.hpp.")
        a("")
        a("--- BEGIN ask_opencode.txt ---")
        a(ask_content.strip())
        a("--- END ask_opencode.txt ---")
    else:
        a("analyze_handle.py did not run this cycle (output/ask_opencode.txt missing).")
        a("If the GCHandle resolver (il2cpp_gchandle_get_target) changed this update:")
        a("  1. Run: python analyze_handle.py  (needs GameAssembly.dll + capstone)")
        a("  2. Read output/handle_dump.json + output/handle_disasm.txt")
        a("  3. Update sdk.hpp `Il2cppGetHandle` and offsets.hpp `il2cpphandle` RVA")
        a("  4. The Il2cppGetHandle RVA is also in config.json as `il2cpphandle` and in")
        a("     offsets.hpp as `Il2cppGetHandle`. They MUST match.")
        a("If you cannot run capstone, derive the resolver from output/master.json field")
        a("`gchandle_get_target` / `gc_handle_array` and the disassembly notes there.")
    a("")

    # --- Section: patching rules ---
    a("================================================================")
    a("SECTION 5 -- PATCHING RULES")
    a("================================================================")
    a("- Prefer small, targeted edits over rewriting whole files.")
    a("- For offsets.hpp use the existing style: `inline uint64_t Name = 0xVALUE;` inside")
    a("  the correct namespace. Match indentation (tabs).")
    a("- For sdk.hpp decrypt namespace, PRESERVE resolve_possible_handle and Il2cppGetHandle.")
    a("  Match existing OffsetManager::DecryptCfg.* references.")
    a("- Never swap nk_ and nk2_ prefixes (nk = networkable_key, nk2 = networkable_key2).")
    a("- If rust_decrypts.dat key/value lines changed, update OffsetManager.hpp DecryptConfig")
    a("  defaults to match, so the embedded defaults agree with the .dat file.")
    a("- After EVERY edit batch, run the build command and use compile errors as feedback.")
    a("- Backups live at: %s" % (Path(__file__).parent / "backups"))
    a("  If the build breaks irreversibly, restore offsets.hpp / sdk.hpp / OffsetManager.hpp")
    a("  from the most recent SANE backup (not necessarily the newest -- inspect first).")
    a("")

    # --- Section: verification ---
    a("================================================================")
    a("SECTION 6 -- VERIFICATION (do not skip)")
    a("================================================================")
    a("1. Build MUST complete with zero errors (all 4 DLL flavors).")
    a("2. Close Rust, delete C:\\rust_decrypts.dat (forces use of correct embedded defaults).")
    a("3. Inject the DLL with the project loader.")
    a("4. Check the flavor-specific log:")
    a("   - C:\\cheat_debug.log         (BeaZt)")
    a("   - C:\\cheat_debug_bomza.log   (Bomza)")
    a("   - C:\\cheat_debug_bc.log      (Better Cheats)")
    a("5. Verify the BN chain succeeds: look for 'BN chain SUCCESS!' + non-zero entity size.")
    a("")
    a("IN-GAME FEATURE TEST CHECKLIST:")
    a("  Test                          Validates                    If it fails")
    a("  ----                          ---------                    -----------")
    a("  ESP shows players on server   nk+nk2 decrypts + BN chain   Re-run sha-dumper, check basenetworkable_pointer")
    a("  ESP shows animals/NPCs        Entity classification        Check BaseCombatEntity::Lifestate in dump")
    a("  ESP shows world items         World prefab ESP             Check cache.cpp prefab lists")
    a("  Held item name shows in ESP   cla+inv decrypts             Check decrypt_algorithms.json for new values")
    a("  Aimbot targets correctly      ey decrypt + PlayerInput      Check player_eyes RVA in dump")
    a("  FOV changer works             fov decrypt (no ROL!)        Check decrypt_fov — should have fov_xor/add/sub, NOT fov_rol")
    a("  NoRecoil works                BaseProjectile+RecoilProps   Check BaseProjectile namespace in dump")
    a("  BrightNight works             TOD_Sky offsets              Check TOD_Sky namespace in dump")
    a("")
    a("6. If ESP classification is wrong, review the log for 'CACHE UNMATCHED' prefab names")
    a("   and add them to cache.cpp.")
    a("7. If only LocalPlayer shows in ESP, the BN chain or decrypt is wrong — check log for errors.")
    if has_mesh:
        a(f"8. Test VisCheck: enable in BeaZt menu (Player ESP -> VisCheck toggle).")
        a(f"   Mesh data: output/rust_mesh.tri ({mesh_tri_count:,} tris, {mesh_tri_size_mb:.1f} MB)")
        a(f"   BVH builds on first load (~20-30s), cached to C:\\rust_mesh.bvh.")
        a(f"   Verify: players behind walls show INVISIBLE color, visible players show VISIBLE color.")
        a(f"   Package: include rust_mesh.tri alongside DLL in distribution.")
    else:
        a(f"8. VisCheck: NO mesh data available — uses PlayerModel._visible fallback (~60% accuracy).")
        a(f"   To enable raycast VisCheck, run getnewoffsets.bat with Rust in-game on a server.")
    a("")

    # --- Section: safety ---
    a("================================================================")
    a("SECTION 7 -- SAFETY REMINDERS")
    a("================================================================")
    a("- C:\\rust_decrypts.dat is LOCKED while Rust is running. Close the game before")
    a("  overwriting it. apply_patches.py will warn 'file is likely locked' if it is.")
    a("- update_now.bat auto-restores from the NEWEST backup on build failure. The newest")
    a("  backup can be stale -- inspect it before trusting the restore.")
    a("- Never trust UnknownCheats offsets blindly; verify against offsets_dump.hpp or sha-dumper output.")
    a("- Never use direct memory dereferences; all reads/writes use read<T>() / write<T>().")
    a("- Do NOT commit changes unless the user explicitly asks.")
    a("")

    # --- Section: current state summary ---
    a("================================================================")
    a("SECTION 8 -- CURRENT PIPELINE STATE")
    a("================================================================")
    a(f"Build number: {build}")
    a("Pipeline output files present in %s:" % out_dir)
    for f in present_outputs:
        a(f"  - {f}")
    a("")
    if summary_text.strip():
        a("summary.txt contents:")
        a(summary_text.strip())
        a("")

    # --- Final instruction ---
    a("================================================================")
    a("INSTRUCTIONS")
    a("================================================================")
    a("1. Read Section 0 (verification status) — if any warnings, investigate first.")
    a("2. Read every file listed in Section 2 (in order).")
    a("3. Compare the manual offsets in Section 3 against offsets_dump.hpp.")
    a("4. Apply ONLY the minimal patches needed. Do not refactor.")
    a("5. Build after each batch of edits. Use compiler errors as your next input.")
    a("6. When the build is green, run the in-game checklist (Section 6).")
    a("7. Report: what you changed, what still needs testing, and any offsets you could")
    a("   not verify.")
    a("")

    prompt = "\n".join(lines)
    out_path = out_dir / "opencode_update_prompt.txt"
    out_path.write_text(prompt, encoding="utf-8")
    print(f"[OK] Wrote opencode master prompt: {out_path}")
    print(f"     Build: {build}  |  {len(lines)} lines  |  "
          f"decrypts: {len(decrypt_algos)}/6  |  "
          f"sig-scan: {sum(1 for v in sig_results.values() if v.get('rva', 0) > 0)}/{len(sig_results)}")


if __name__ == "__main__":
    main()
