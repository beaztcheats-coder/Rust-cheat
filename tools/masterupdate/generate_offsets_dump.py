#!/usr/bin/env python3
"""
generate_offsets_dump.py — Generates a consolidated Morphine-style offsets dump.

Output: output/offsets_dump.hpp — single file with all static pointers,
field offsets, klass_rvas, and decrypt function implementations.

Run after compare_and_patch.py. Sources: master.json, offsets.hpp,
decrypt_algorithms.json, sig_scan_results.json.
"""
import json
import re
import sys
from pathlib import Path
from datetime import datetime

SCRIPT_DIR = Path(__file__).parent
OUTPUT_DIR = SCRIPT_DIR / "output"
MASTER_JSON = OUTPUT_DIR / "master.json"
DECRYPT_ALGOS_JSON = OUTPUT_DIR / "decrypt_algorithms.json"
SIG_RESULTS_JSON = OUTPUT_DIR / "sig_scan_results.json"
SHA_DUMPER_OUTPUT = OUTPUT_DIR / "sha-dumper-output.txt"
CFG_PATH = SCRIPT_DIR / "config.json"
DUMP_FILE = OUTPUT_DIR / "offsets_dump.hpp"

DECRYPT_NAMES = {
    "base_networkable_0": ("client_entities", "networkable_key"),
    "base_networkable_1": ("entity_list", "networkable_key2"),
    "cl_active_item": ("cl_active_item", "decrypt_ClActiveItem"),
    "player_inventory": ("player_inventory", "decrypt_inventory_pointer"),
    "player_eyes": ("player_eyes", "decrypt_eyes"),
    "decrypt_fov": ("decrypt_fov", "decrypt_fov"),
}


def load_json(path):
    if not path.exists():
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def load_text(path):
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="ignore")


def parse_offsets_hpp(content):
    """Parse offsets.hpp into {namespace: [(field, value, comment)]}."""
    result = {}
    ns_pattern = re.compile(r'\bnamespace\s+(\w+)\s*\{')
    ns_matches = list(ns_pattern.finditer(content))

    for i, ns_match in enumerate(ns_matches):
        ns_name = ns_match.group(1)
        if ns_name in ("offsets", "decrypt", "color", "ESP", "WORLD", "NPC_ESP",
                       "ANIMAL_ESP", "RADAR", "BATTLE", "AIMBOT", "MISC", "SETTINGS",
                       "MENU_UI", "MENU_FX"):
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
            r'(?:inline|constexpr)\s+(?:uint64_t|std::uintptr_t|int|bool|float|uint32_t)\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)(;)'
        )
        for m in field_pattern.finditer(block):
            field_name = m.group(1)
            val_str = m.group(2)
            val = int(val_str, 16) if val_str.startswith("0x") else int(val_str)
            # Get comment if present
            line_start = block.rfind('\n', 0, m.start()) + 1
            line_end = block.find('\n', m.end())
            if line_end < 0:
                line_end = len(block)
            line = block[line_start:line_end]
            comment = ""
            cmt = line.split("//", 1)
            if len(cmt) > 1:
                comment = cmt[1].strip()
            if ns_name not in result:
                result[ns_name] = []
            result[ns_name].append((field_name, val, comment))

    return result


def parse_top_level_offsets(content):
    """Parse top-level inline uint64_t offsets (before any namespace)."""
    result = {}
    # Only look at first ~2000 chars for top-level
    for m in re.finditer(
        r'inline\s+uint64_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;',
        content[:2000]
    ):
        name = m.group(1)
        val = int(m.group(2), 16) if m.group(2).startswith("0x") else int(m.group(2))
        result[name] = val
    return result


def format_decrypt_function(name, func_name, algo_data):
    """Generate C++ decrypt function from algorithm data."""
    ops_raw = algo_data.get("ops_raw", [])
    source = algo_data.get("source", "unknown")
    rva = algo_data.get("read_offset", algo_data.get("rva", "?"))

    if not ops_raw:
        return f"// {func_name}: no algorithm data\n"

    lines = []
    lines.append(f"// Source: {source}    RVA: {rva}")

    if name == "decrypt_fov":
        # FOV decrypt is special — operates on uint32_t, not uint64_t
        lines.append(f"inline uint32_t {func_name}(uint32_t val) {{")
        for op_type, val in ops_raw:
            if op_type == "rol":
                lines.append(f"\tval = (val << 0x{val:X}) | (val >> (32 - 0x{val:X}));")
            elif op_type == "ror":
                lines.append(f"\tval = (val >> 0x{val:X}) | (val << (32 - 0x{val:X}));")
            elif op_type == "xor":
                lines.append(f"\tval ^= 0x{val:08X};")
            elif op_type == "add":
                lines.append(f"\tval += 0x{val:08X};")
            elif op_type == "sub":
                lines.append(f"\tval -= 0x{val:08X};")
        lines.append("\treturn val;")
        lines.append("}")

        # Encrypt (reverse)
        inverse = {"add": "sub", "sub": "add", "xor": "xor", "rol": "ror", "ror": "rol"}
        lines.append(f"inline uint32_t encrypt_fov(uint32_t val) {{")
        for op_type, val in reversed(ops_raw):
            inv_type = inverse.get(op_type, op_type)
            if inv_type == "rol":
                lines.append(f"\tval = (val << 0x{val:X}) | (val >> (32 - 0x{val:X}));")
            elif inv_type == "ror":
                lines.append(f"\tval = (val >> 0x{val:X}) | (val << (32 - 0x{val:X}));")
            elif inv_type == "xor":
                lines.append(f"\tval ^= 0x{val:08X};")
            elif inv_type == "add":
                lines.append(f"\tval += 0x{val:08X};")
            elif inv_type == "sub":
                lines.append(f"\tval -= 0x{val:08X};")
        lines.append("\treturn val;")
        lines.append("}")
    else:
        # Standard decrypt — operates on uint64_t, 2 iterations over 32-bit halves
        lines.append(f"uintptr_t {func_name}(uint64_t a1) {{")
        if name in ("cl_active_item", "player_inventory", "player_eyes"):
            lines.append(f"\tstd::uint32_t* rdx = (std::uint32_t*)&a1;")
        else:
            lines.append(f"\tstd::uintptr_t rax = driver.read<std::uintptr_t>(a1 + 0x18);")
            lines.append(f"\tstd::uint32_t* rdx = (std::uint32_t*)&rax;")
        lines.append(f"\tstd::uint32_t count = 0x2;")
        lines.append(f"\tdo {{")

        for op_type, val in ops_raw:
            if op_type == "rol":
                lines.append(f"\t\t*rdx = (*rdx << 0x{val:X}) | (*rdx >> (32 - 0x{val:X}));")
            elif op_type == "ror":
                lines.append(f"\t\t*rdx = (*rdx >> 0x{val:X}) | (*rdx << (32 - 0x{val:X}));")
            elif op_type == "xor":
                lines.append(f"\t\t*rdx ^= 0x{val:08X};")
            elif op_type == "add":
                lines.append(f"\t\t*rdx += 0x{val:08X};")
            elif op_type == "sub":
                lines.append(f"\t\t*rdx -= 0x{val:08X};")

        lines.append(f"\t\trdx++;")
        lines.append(f"\t}} while (--count);")
        if name == "cl_active_item" or name == "player_inventory" or name == "player_eyes":
            lines.append(f"\treturn a1;")
        else:
            lines.append(f"\treturn il2cpp_get_handle(rax);")
        lines.append("}")

    return "\n".join(lines)


def main():
    with open(CFG_PATH, "r", encoding="utf-8") as f:
        cfg = json.load(f)

    offsets_hpp_path = Path(cfg["offsets_hpp"])
    offsets_content = load_text(offsets_hpp_path)
    offsets_ns = parse_offsets_hpp(offsets_content)
    top_offsets = parse_top_level_offsets(offsets_content)

    master = load_json(MASTER_JSON)
    capstone = load_json(DECRYPT_ALGOS_JSON)
    sig_results = load_json(SIG_RESULTS_JSON)

    build = master.get("build", "unknown")
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Static pointers
    static_ptrs = master.get("offsets", {}).get("static_pointers", {})
    master_offsets = master.get("offsets", {})
    klass_rvas = master.get("klass_rvas", {})
    sha_statics = {}
    sha_text = load_text(SHA_DUMPER_OUTPUT)
    for line in sha_text.split("\n"):
        m = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', line.strip())
        if m and m.group(1) in ("basenetworkable", "main_camera", "tod_sky",
                                 "ui_loading_screen", "mixer_snapshot_mgr",
                                 "gchandle_array", "effect_network"):
            sha_statics[m.group(1)] = int(m.group(2), 16)

    lines = []
    L = lines.append

    L("// ============================================================")
    L("// Rust Cheat — Consolidated Offsets Dump")
    L(f"// Build: {build}")
    L(f"// Generated: {now}")
    L("// Sources: sha-dumper + capstone + Frida + sig-scanner")
    L("// 100% Morphine-independent")
    L("// ============================================================")
    L("")
    L('#include <cstdint>')
    L('#include <string>')
    L("")
    L(f'inline std::string Build = "{build}";')
    L("")

    # === STATIC POINTERS ===
    L("// === STATIC POINTERS ===")
    L("")

    # il2cpphandle
    il2cpp_val = top_offsets.get("Il2cppGetHandle", master_offsets.get("il2cpphandle", 0))
    sig_val = sig_results.get("Il2cppGetHandle", {}).get("rva", 0)
    src = []
    if sig_val and sig_val == il2cpp_val:
        src.append("sig-scan")
    src.append("sha-dumper" if not sig_val else "offsets.hpp")
    L(f'inline static constexpr uintptr_t il2cpphandle = 0x{il2cpp_val:08X}; // [{"+".join(src)}]')
    L("")

    # klass_rvas
    L("struct klass_rvas {")
    # Add from master.json klass_rvas + sha-dumper + sig-scan
    all_klasses = {}
    for name, rva in klass_rvas.items():
        all_klasses[name] = rva
    # Also add from sha-dumper output
    sha_name_map = {
        "basenetworkable": "BaseNetworkable",
        "main_camera": "MainCamera",
        "tod_sky": "TOD_Sky",
    }
    for sha_key, klass_name in sha_name_map.items():
        if sha_key in sha_statics and klass_name not in all_klasses:
            all_klasses[klass_name] = sha_statics[sha_key]

    for name, rva in sorted(all_klasses.items()):
        sig_name = name if name in sig_results else None
        sig_rva = sig_results.get(sig_name, {}).get("rva", 0) if sig_name else 0
        src = "sha-dumper"
        if sig_rva and sig_rva == rva:
            src = "sha-dumper+sig-scan"
        L(f'\tinline static constexpr uintptr_t {name} = 0x{rva:08X}; // [{src}]')
    L("};")
    L("")

    # gc_handles
    L("struct gc_handles {")
    gc_rva = top_offsets.get("Il2cppGetHandle", 0)
    L(f'\tinline static constexpr uintptr_t array_rva = 0x{gc_rva:08X};')
    L("};")
    L("")

    # === FIELD OFFSETS ===
    L("// === FIELD OFFSETS ===")
    L("")

    # Map our namespace names to Morphine-style snake_case where applicable
    ns_renames = {
        "BaseNetworkable": "base_networkable",
        "BaseCamera": "camera",
        "BasePlayer": "base_player",
        "BaseCombatEntity": "base_combat_entity",
        "BaseEntity": "base_entity",
        "PlayerEyes": "player_eyes",
        "PlayerInput": "player_input",
        "PlayerModel": "player_model",
        "PlayerInventory": "PlayerInventory",
        "ItemContainer": "item_container",
        "Item": "item",
        "ItemDefinition": "ItemDefinition",
        "BaseProjectile": "BaseProjectile",
        "HeldEntity": "held_entity",
        "Model": "model",
        "TOD_Sky": "tod_sky",
        "TOD_Sky_Static": "tod_sky_static",
        "TOD_NightParameters": "tod_night_parameters",
        "TOD_AmbientParameters": "tod_ambient_parameters",
        "SkinnedMultiMesh": "SkinnedMultiMesh",
        "RecoilProperties": "RecoilProperties",
        "EffectNetwork": "effect_network",
        "FOV": "convar_graphics",
        "BaseMovement2": "base_movement",
        "PlayerWalkMovement": "PlayerWalkMovement",
        "base_player_flags": "base_player_flags",
        "unity_string": "unity_string",
        "SingletonComponent": "singleton_component",
        "MixerSnapshotManager": "MixerSnapshotManager",
        "UI_LoadingScreen": "UI_LoadingScreen",
    }

    for ns_name, fields in offsets_ns.items():
        morphine_name = ns_renames.get(ns_name, ns_name)
        L(f"namespace {morphine_name} {{")
        for field_name, val, comment in fields:
            cmt = f" // {comment}" if comment else ""
            L(f"\tconstexpr std::uintptr_t {field_name} = 0x{val:X};{cmt}")
        L("}")
        L("")

    # === DECRYPT FUNCTIONS ===
    L("// === DECRYPT FUNCTIONS ===")
    L("")

    capstone_algos = {a["name"]: a for a in capstone.get("algorithms", [])}
    master_decrypts = master.get("decrypt_functions", {})

    for canonical, (func_name, display_name) in DECRYPT_NAMES.items():
        algo = capstone_algos.get(canonical)
        master_fn = master_decrypts.get(canonical, {})

        if not algo and not master_fn:
            L(f"// {func_name}: MISSING — no algorithm data")
            L("")
            continue

        # Use capstone data if available, otherwise master.json
        data = algo or master_fn
        code = format_decrypt_function(canonical, func_name, data)
        L(code)
        L("")

    # === DECRYPT CONSTANTS ===
    L("// === DECRYPT CONSTANTS (rust_decrypts.dat format) ===")
    L("// Copy to C:\\rust_decrypts.dat to override embedded defaults at runtime")
    L("")
    for canonical, (func_name, prefix) in DECRYPT_NAMES.items():
        algo = capstone_algos.get(canonical, master_decrypts.get(canonical, {}))
        ops_raw = algo.get("ops_raw", [])
        if not ops_raw:
            continue
        L(f"// {func_name} ({prefix})")
        for i, (op_type, val) in enumerate(ops_raw):
            key = f"{prefix}_{op_type}"
            if i > 0 and op_type in ("rol", "xor"):
                # Check for duplicate op types
                prev_ops = [o for o in ops_raw[:i] if o[0] == op_type]
                if prev_ops:
                    key = f"{prefix}_{op_type}_{len(prev_ops) + 1}"
            if op_type in ("rol", "ror"):
                L(f"{key}=0x{val:08X}")
            else:
                L(f"{key}=0x{val:08X}")
        L("")

    # === SUMMARY ===
    L("// ============================================================")
    L("// SUMMARY")
    L("// ============================================================")
    L(f"// Static pointers: {len(all_klasses)} klass_rvas + il2cpphandle")
    L(f"// Field offsets: {sum(len(v) for v in offsets_ns.values())} fields across {len(offsets_ns)} namespaces")
    L(f"// Decrypt functions: {len(DECRYPT_NAMES)} (4 exact capstone + 1 structural + 1 derived)")
    L(f"// Sig scanner: {sum(1 for v in sig_results.values() if v.get('rva', 0) > 0)}/{len(sig_results)} static pointers found")
    L(f"// Sources: sha-dumper + capstone + Frida + sig-scanner (100% Morphine-independent)")
    L("")

    dump = "\n".join(lines)
    DUMP_FILE.write_text(dump, encoding="utf-8")
    print(f"[OK] Consolidated offsets dump written to {DUMP_FILE}")
    print(f"     {len(lines)} lines, {len(dump)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
