#!/usr/bin/env python3
"""Compare Morphine data against current project files and generate patches."""
import json
import os
import re
import shutil
import copy
from datetime import datetime
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"

# Fallback operation sequences for decrypt functions.
# Used when sha-dumper can't find a function (name obfuscation) or confidence is low.
# Updated: 2026-07-03 (validict build 24037537)
FALLBACK_DECRYPT_OPS = {
    "base_networkable_0": [("rol", 0x16), ("sub", 0x512FB7E6), ("xor", 0x3C25B628), ("add", 0x606330A1)],
    "base_networkable_1": [("rol", 0x12), ("xor", 0xE54E9BFF), ("rol", 0x8), ("xor", 0xCECB4770)],
    "cl_active_item": [("rol", 0x1E), ("add", 0x69D5BDEE), ("rol", 0x10), ("xor", 0x60869282)],
    "decrypt_fov": [("xor", 0x8041A4D4), ("add", 0x2270CDAC), ("rol", 0x1D), ("sub", 0x3BA7A498)],
    "player_inventory": [("rol", 0x8), ("add", 0x18E53C82), ("rol", 0x1)],
    "player_eyes": [("sub", 0x6FB58358), ("xor", 0x6DC93C8F), ("rol", 0x15), ("add", 0x4E3D6061)],
}


def merge_fallback_funcs(funcs: dict) -> dict:
    """Merge fallback ops into the funcs dict.

    - MISSING functions: add fallback with 'fallback' confidence.
    - ANY existing source data (Morphine/sha-dumper): ALWAYS KEEP IT.
      Never override current-build data with stale fallback values.
      The fallback is from the PREVIOUS build and is wrong after a game update.
    Returns a new dict — does not modify the original."""
    result = copy.deepcopy(funcs)
    for morph_name, fallback_ops in FALLBACK_DECRYPT_OPS.items():
        if morph_name not in result:
            # Missing — add fallback
            fb_ops_list = [{"op": o[0], "value": o[1]} for o in fallback_ops]
            result[morph_name] = {
                "ops": fb_ops_list,
                "input_style": "pointer" if morph_name.startswith("base_networkable") else "raw",
                "return_style": "handle" if morph_name.startswith("base_networkable") else "raw",
                "read_offset": None,
                "confidence": "fallback",
                "note": "Using last known good values (not found in sha-dumper)",
            }
        # else: source data exists — KEEP IT. Do NOT override with fallback.
    return result


def generate_decrypt_keys(prefix: str, ops: list) -> list:
    """Generate (key_name, op_type, op_value) tuples from a decrypt function's ops.
    Key format: {prefix}_{op_type} with _2, _3 suffix for duplicate op types.
    This is the SINGLE SOURCE OF TRUTH for key naming — used by append_ops,
    write_decrypts_dat, and generate_offsetmanager_patch to ensure consistency."""
    result = []
    op_type_counts = {}
    for op in ops:
        op_type = op["op"]
        count = op_type_counts.get(op_type, 0) + 1
        op_type_counts[op_type] = count
        suffix = f"_{count}" if count > 1 else ""
        key = f"{prefix}_{op_type}{suffix}"
        val = op["value"]
        if isinstance(val, str):
            val = int(val, 16) if val.startswith("0x") or val.startswith("0X") else int(val)
        result.append((key, op_type, val))
    return result


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def load_master():
    path = Path(__file__).parent / "output" / "master.json"
    with open(path, "r", encoding="utf-8") as f:
        master = json.load(f)
    # Normalize decrypt function op values to ints (capstone merge may leave hex strings)
    for fn_name, fn_info in master.get("decrypt_functions", {}).items():
        for op in fn_info.get("ops", []):
            val = op.get("value")
            if isinstance(val, str):
                try:
                    op["value"] = int(val, 16) if val.startswith("0x") or val.startswith("0X") else int(val)
                except ValueError:
                    pass
    return master


def create_backups(cfg: dict):
    """Create timestamped backups of all files we might modify."""
    ts = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    backup_dir = Path(__file__).parent / "backups" / ts
    backup_dir.mkdir(parents=True, exist_ok=True)

    files_to_backup = [
        cfg["offsets_hpp"],
        cfg["sdk_hpp"],
        str(Path(cfg["offsets_hpp"]).parent / "OffsetManager.hpp"),
    ]
    if Path(cfg["materials_hpp"]).exists():
        files_to_backup.append(cfg["materials_hpp"])

    for src in files_to_backup:
        src_path = Path(src)
        if src_path.exists():
            shutil.copy2(src_path, backup_dir / src_path.name)

    print(f"[OK] Backups saved to {backup_dir}")

    # Prune old backups — keep only the 5 most recent
    backups_root = Path(__file__).parent / "backups"
    if backups_root.exists():
        all_backups = sorted([d for d in backups_root.iterdir() if d.is_dir()], reverse=True)
        for old in all_backups[5:]:
            shutil.rmtree(old, ignore_errors=True)

    return str(backup_dir)


def extract_namespace_block(content: str, namespace: str) -> str:
    """Extract a namespace block from offsets.hpp using brace counting."""
    # Use regex to find exact namespace (avoid matching ItemContainer when looking for Item)
    start_pat = re.compile(rf"namespace\s+{re.escape(namespace)}\b", re.MULTILINE)
    start_m = start_pat.search(content)
    if not start_m:
        return ""

    start_idx = start_m.start()

    i = start_m.end()
    while i < len(content) and content[i] != '{':
        i += 1
    if i >= len(content):
        return ""

    brace_start = i
    brace_count = 1
    i = brace_start + 1
    while i < len(content) and brace_count > 0:
        if content[i] == '{':
            brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
        i += 1

    if brace_count != 0:
        return ""

    return content[start_idx:i]


def find_existing_offset(content: str, name: str, namespace: str = None) -> tuple:
    """Find an existing inline/constexpr uint64_t Name = value; line. Returns (old_value, line).

    Searches the specified namespace first. If not found, falls back to searching
    ALL namespaces — handles cases where Morphine puts a field in a different
    namespace than the cheat (e.g. BaseProjectile vs BaseProjectileExt)."""
    pattern = re.compile(rf"((?:inline|constexpr)\s+uint64_t\s+{re.escape(name)}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)")

    if namespace:
        block = extract_namespace_block(content, namespace)
        if block:
            m = pattern.search(block)
            if m:
                return int(m.group(2), 0), m.group(0)

    # Fallback: search entire file (all namespaces)
    m = pattern.search(content)
    if m:
        return int(m.group(2), 0), m.group(0)
    return None, None


def generate_offset_patches(cfg: dict, master: dict, content: str) -> tuple:
    """Generate patches for offsets.hpp using precise name and namespace mappings."""
    patches = []
    changes = []
    missing = []

    name_mappings = cfg.get("name_mappings", {})
    namespace_mappings = cfg.get("namespace_field_mappings", {})

    # 1. Top-level offsets (full key mappings only) — global offsets namespace
    for morphine_name, value in master.get("offsets", {}).items():
        project_name = name_mappings.get(morphine_name)
        if project_name is None:
            continue  # Skip unmapped top-level constants (il2cpp API, flags, etc.)

        if value == 0:
            continue  # Never patch to 0x0 — name obfuscation, not a real offset

        old_value, old_line = find_existing_offset(content, project_name, namespace="offsets")
        if old_value is None:
            missing.append((project_name, value))
            continue

        if old_value != value:
            changes.append((project_name, old_value, value))
            patches.append({
                "name": project_name,
                "namespace": "offsets",
                "old_value": old_value,
                "new_value": value,
                "old_line": old_line,
                "new_line": re.sub(
                    rf"((?:inline|constexpr)\s+uint64_t\s+{re.escape(project_name)}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)",
                    rf"\g<1>0x{value:08X}\g<3>",
                    old_line,
                    flags=re.IGNORECASE,
                ),
            })

    # 2. Class TypeInfo RVAs (klass_rvas) — static pointers in global offsets namespace
    for morphine_name, value in master.get("klass_rvas", {}).items():
        project_name = name_mappings.get(morphine_name, morphine_name)  # Fallback: use name as-is (sig_scanner uses project_names)
        if value == 0:
            continue  # Never patch to 0x0

        old_value, old_line = find_existing_offset(content, project_name, namespace="offsets")
        if old_value is None:
            missing.append((project_name, value))
            continue

        if old_value != value:
            changes.append((project_name, old_value, value))
            patches.append({
                "name": project_name,
                "namespace": "offsets",
                "old_value": old_value,
                "new_value": value,
                "old_line": old_line,
                "new_line": re.sub(
                    rf"((?:inline|constexpr)\s+uint64_t\s+{re.escape(project_name)}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)",
                    rf"\g<1>0x{value:08X}\g<3>",
                    old_line,
                    flags=re.IGNORECASE,
                ),
            })

    # 3. Struct fields (namespace + field mappings)
    for struct_name, fields in master.get("structs", {}).items():
        mapping = None
        # Normalize: compare ignoring case AND underscores (handles CamelCase vs snake_case)
        norm_struct = struct_name.lower().replace("_", "")
        for key, val in namespace_mappings.items():
            if key.lower().replace("_", "") == norm_struct:
                mapping = val
                break
        if not mapping:
            continue  # Skip unmapped structs

        project_ns = mapping.get("namespace")
        field_map = mapping.get("fields", {})

        for morphine_field, value in fields.items():
            if value == 0:
                continue  # Never patch to 0x0 — name obfuscation

            project_field = field_map.get(morphine_field)
            if project_field is None:
                # Try direct Il2Cpp field name match (sha-dumper bulk dumps use raw names)
                # Check if any field_map value matches the raw field name
                for map_morph, map_proj in field_map.items():
                    if map_proj.lower() == morphine_field.lower():
                        project_field = map_proj
                        break
            if project_field is None:
                continue  # Skip unmapped fields

            project_name = project_field
            old_value, old_line = find_existing_offset(content, project_name, namespace=project_ns)
            if old_value is None:
                missing.append((project_name, value))
                continue

            if old_value != value:
                changes.append((project_name, old_value, value))
                patches.append({
                    "name": project_name,
                    "namespace": project_ns,
                    "old_value": old_value,
                    "new_value": value,
                    "old_line": old_line,
                    "new_line": re.sub(
                        rf"((?:inline|constexpr)\s+uint64_t\s+{re.escape(project_name)}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)",
                        rf"\g<1>0x{value:08X}\g<3>",
                        old_line,
                        flags=re.IGNORECASE,
                    ),
                })

    # 4. Namespace fields (namespace + field mappings) — same logic as structs but from namespaces dict
    for ns_name, fields in master.get("namespaces", {}).items():
        mapping = None
        norm_ns = ns_name.lower().replace("_", "")
        for key, val in namespace_mappings.items():
            if key.lower().replace("_", "") == norm_ns:
                mapping = val
                break
        if not mapping:
            continue

        project_ns = mapping.get("namespace")
        field_map = mapping.get("fields", {})

        # Verify the namespace exists in our offsets.hpp before patching
        our_block = extract_namespace_block(content, project_ns)
        if not our_block:
            continue  # Namespace not in our code — skip to avoid false matches

        for morphine_field, value in fields.items():
            if value == 0:
                continue  # Never patch to 0x0 — name obfuscation

            project_field = field_map.get(morphine_field)
            if project_field is None:
                for map_morph, map_proj in field_map.items():
                    if map_proj.lower() == morphine_field.lower():
                        project_field = map_proj
                        break
            if project_field is None:
                continue

            project_name = project_field
            old_value, old_line = find_existing_offset(content, project_name, namespace=project_ns)
            if old_value is None:
                missing.append((project_name, value))
                continue

            if old_value != value:
                changes.append((project_name, old_value, value))
                patches.append({
                    "name": project_name,
                    "namespace": project_ns,
                    "old_value": old_value,
                    "new_value": value,
                    "old_line": old_line,
                    "new_line": re.sub(
                        rf"((?:inline|constexpr)\s+uint64_t\s+{re.escape(project_name)}\s*=\s*)(0x[0-9A-Fa-f]+|\d+)(;)",
                        rf"\g<1>0x{value:08X}\g<3>",
                        old_line,
                        flags=re.IGNORECASE,
                    ),
                })

    return patches, changes, missing


def generate_sdk_patches(cfg: dict, master: dict, content: str) -> tuple:
    """Generate patches for sdk.hpp (e.g., gc_handle_array_rva if referenced there)."""
    patches = []
    changes = []

    # Map specific morphine values to sdk.hpp variables
    # Currently mostly empty because handle vars live in offsets.hpp
    # Add any sdk-specific mappings here if needed

    return patches, changes


def generate_decrypt_patch(master: dict, cfg: dict) -> str:
    """Generate a complete, ready-to-paste namespace decrypt { ... } block."""
    mapping = cfg.get("decrypt_mappings", {})
    funcs = merge_fallback_funcs(master.get("decrypt_functions", {}))

    lines = [
        "// ============================================================",
        "// Auto-generated decrypt namespace patch from Morphine",
        f"// Build: {master.get('build', 'unknown')}",
        "// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions",
        "// ============================================================",
        "",
        "namespace decrypt {",
        "",
        "    // ============================================================",
        "    // Runtime pointer validation",
        "    // ============================================================",
        "",
        "    inline bool IsValidPointer(uint64_t p)",
        "    {",
        "        if (!p) return false;",
        "        if (p & 0x7) return false;                 // basic 8-byte alignment",
        "        if (p < 0x10000) return false;             // reject null/small values",
        "        return true;",
        "    }",
        "",
        "    inline bool IsInModuleRange(uint64_t p)",
        "    {",
        "        // Fast range check against loaded module bases.",
        "        // Falls back to true if bases are not initialized yet.",
        "        if (!GameAssembly && !UnityPlayer) return true;",
        "        uint64_t ga_end = GameAssembly ? GameAssembly + 0x40000000 : 0; // ~1GB generous",
        "        uint64_t up_end = UnityPlayer ? UnityPlayer + 0x20000000 : 0;",
        "        if (GameAssembly && p >= GameAssembly && p <= ga_end) return true;",
        "        if (UnityPlayer && p >= UnityPlayer && p <= up_end) return true;",
        "        return false;",
        "    }",
        "",
        "    // ============================================================",
        "    // Il2Cpp GCHandle resolver",
        "    // Derived from il2cpp_gchandle_get_target disassembly.",
        "    // ============================================================",
        "",
        "    inline uint64_t Il2cppGetHandle(uint64_t handle_ptr)",
        "    {",
        "        if (!handle_ptr) return 0;",
        "",
        "        uint64_t page_base = handle_ptr & 0xFFFFFFFFFFFFE000ULL;",
        "        if (!page_base) return 0;",
        "",
        "        uint8_t type = read<uint8_t>(page_base + 0x20);",
        "        if (type >= 4) return 0;",
        "",
        "        int64_t slot = (int64_t)(handle_ptr - page_base - 0x28) >> 3;",
        "        if (slot < 0) return 0;",
        "",
        "        uint32_t size = read<uint32_t>(page_base + 0x1C);",
        "        if ((uint32_t)slot >= size) return 0;",
        "",
        "        uint64_t bitmap_ptr = read<uint64_t>(page_base + 0x10);",
        "        if (!bitmap_ptr) return 0;",
        "",
        "        uint32_t bitmask = read<uint32_t>(bitmap_ptr + 4 * ((uint32_t)slot >> 5));",
        "        if (!((bitmask >> (slot & 0x1F)) & 1)) return 0;",
        "",
        "        uint64_t entry = read<uint64_t>(page_base + 8 * ((uint32_t)slot + 5));",
        "",
        "        if (type > 1)",
        "            return entry;",
        "",
        "        // type 0/1: real helper returns 0 for empty slots instead of ~0",
        "        if (!entry) return 0;",
        "        return ~entry;",
        "    }",
        "",
        "    // ============================================================",
        "    // Type-aware read helpers",
        "    // Use these for fields classified by the updater.",
        "    // ============================================================",
        "",
        "    inline uint64_t ReadObjectPtr(uint64_t address)",
        "    {",
        "        uint64_t handle = read<uint64_t>(address);",
        "        if (!IsValidPointer(handle)) return 0;",
        "        return Il2cppGetHandle(handle);",
        "    }",
        "",
        "    inline uint64_t ReadRawPtr(uint64_t address)",
        "    {",
        "        uint64_t p = read<uint64_t>(address);",
        "        return IsValidPointer(p) ? p : 0;",
        "    }",
        "",
        "    inline uint64_t ReadTaggedPtr(uint64_t address, uint64_t (*decrypt_fn)(uint64_t))",
        "    {",
        "        uint64_t encoded = read<uint64_t>(address);",
        "        if (!encoded) return 0;",
        "        uint64_t decoded = decrypt_fn(encoded);",
        "        return IsValidPointer(decoded) ? decoded : 0;",
        "    }",
        "",
        "    inline uint64_t ReadObjectTagged(uint64_t address, uint64_t (*decrypt_fn)(uint64_t))",
        "    {",
        "        uint64_t encoded = read<uint64_t>(address);",
        "        if (!encoded) return 0;",
        "        uint64_t decoded = decrypt_fn(encoded);",
        "        if (!decoded) return 0;",
        "        return Il2cppGetHandle(decoded);",
        "    }",
        "",
        "    // ============================================================",
        "    // Handle resolver — preserved across updates (not Morphine-generated)",
        "    // ============================================================",
        "",
        "    inline uint64_t resolve_possible_handle(uint64_t value)",
        "    {",
        "        if (!value) return 0;",
        "        if (value & 1)",
        "            return Il2cppGetHandle(value);",
        "        // If not 8-byte aligned, it might be a disguised GCHandle — try Il2cppGetHandle",
        "        if (value & 0x7) {",
        "            uint64_t resolved = Il2cppGetHandle(value);",
        "            if (resolved && (resolved & 0x7) == 0 && resolved > 0x10000)",
        "                return resolved;",
        "        }",
        "        // Aligned value: check if pointer-to-pointer",
        "        if ((value & 0x7) == 0 && value < 0x7FFFFFFFFFFF) {",
        "            uint64_t deref = read<uint64_t>(value);",
        "            if (deref && deref < 0x7FFFFFFFFFFF && (deref & 0x7) == 0 && deref != value)",
        "                return deref;",
        "        }",
        "        return value;",
        "    }",
        "",
    ]

    key_map = cfg.get("decrypt_dat_key_map", {})
    seen_project_names = set()

    for morphine_name, project_name in mapping.items():
        if morphine_name not in funcs:
            lines.append(f"    // WARNING: {morphine_name} not found in any data source")
            lines.append("")
            continue

        if project_name in seen_project_names:
            continue
        seen_project_names.add(project_name)

        info = funcs[morphine_name]
        confidence = info.get("confidence", "high")
        if confidence == "fallback":
            lines.append(f"    // PRESERVED: {morphine_name} — {info.get('note', 'using fallback values')}")
        ops = info.get("ops", [])
        cfg_prefix = key_map.get(project_name)

        if project_name == "decrypt_fov":
            # FOV operates on a single uint32_t, not a uint64_t loop
            keys = generate_decrypt_keys(cfg_prefix, ops) if cfg_prefix else None
            lines.append(f"    inline uint32_t {project_name}(uint32_t val) {{")
            for i, op in enumerate(ops):
                if keys:
                    ref = f"OffsetManager::DecryptCfg.{keys[i][0]}"
                else:
                    ref = f"0x{op['value']:08X}"
                if op["op"] == "rol":
                    lines.append(f"        val = (val << {ref}) | (val >> (32 - {ref}));")
                elif op["op"] == "ror":
                    lines.append(f"        val = (val >> {ref}) | (val << (32 - {ref}));")
                elif op["op"] == "add":
                    lines.append(f"        val += {ref};")
                elif op["op"] == "sub":
                    lines.append(f"        val -= {ref};")
                elif op["op"] == "xor":
                    lines.append(f"        val ^= {ref};")
            lines.append("        return val;")
            lines.append("    }")
            lines.append("")
            continue

        if info.get("input_style") == "pointer":
            lines.extend(generate_pointer_decrypt(project_name, ops, info.get("return_style", "handle"), cfg_prefix))
        else:
            lines.extend(generate_raw_decrypt(project_name, ops, info.get("return_style", "handle"), cfg_prefix))
        lines.append("")

    # Preserve encrypt_fov (reverse of decrypt_fov) — not in Morphine mappings but required by misc.cpp
    lines.append("    // encrypt_fov — reverse of decrypt_fov, preserved across updates")
    lines.append("    inline uint32_t encrypt_fov(uint32_t val) {")
    fov_ops = []
    for morphine_name, project_name in mapping.items():
        if project_name == "decrypt_fov" and morphine_name in funcs:
            fov_ops = funcs[morphine_name].get("ops", [])
            break
    if fov_ops:
        fov_prefix = key_map.get("decrypt_fov")
        keys = generate_decrypt_keys(fov_prefix, fov_ops) if fov_prefix else None
        reversed_ops = list(reversed(fov_ops))
        reversed_keys = list(reversed(keys)) if keys else None
        for i, op in enumerate(reversed_ops):
            if reversed_keys:
                ref = f"OffsetManager::DecryptCfg.{reversed_keys[i][0]}"
            else:
                ref = f"0x{op['value']:08X}"
            if op["op"] == "rol":
                lines.append(f"        val = (val >> {ref}) | (val << (32 - {ref}));")
            elif op["op"] == "ror":
                lines.append(f"        val = (val << {ref}) | (val >> (32 - {ref}));")
            elif op["op"] == "add":
                lines.append(f"        val -= {ref};")
            elif op["op"] == "sub":
                lines.append(f"        val += {ref};")
            elif op["op"] == "xor":
                lines.append(f"        val ^= {ref};")
    else:
        lines.append("        // Fallback: use DecryptCfg fields directly")
        lines.append("        val += OffsetManager::DecryptCfg.fov_sub;")
        lines.append("        val = (val >> OffsetManager::DecryptCfg.fov_rol) | (val << (32 - OffsetManager::DecryptCfg.fov_rol));")
        lines.append("        val -= OffsetManager::DecryptCfg.fov_add;")
        lines.append("        val ^= OffsetManager::DecryptCfg.fov_xor;")
    lines.append("        return val;")
    lines.append("    }")
    lines.append("")

    lines.append("} // namespace decrypt")
    lines.append("")
    return "\n".join(lines)


def generate_pointer_decrypt(name: str, ops: list, return_style: str, cfg_key_prefix: str = None) -> list:
    lines = [
        f"    inline uintptr_t {name}(uint64_t a1)",
        "    {",
        "        uint64_t value = read<uint64_t>(a1 + 0x18);",
        "        if (!value) return 0;",
        "        uint32_t* data = (uint32_t*)&value;",
        "        uint32_t count = 2;",
        "        do {",
        "            uint32_t x = *data;",
    ]
    lines.extend(append_ops(ops, cfg_key_prefix))
    lines.extend([
        "            data++;",
        "            --count;",
        "        } while (count);",
    ])
    if return_style == "handle":
        lines.extend([
            "        uintptr_t resolved = Il2cppGetHandle(value);",
            "        if (resolved) return resolved;",
            "        return (value & 0x7) == 0 ? value : 0;",
        ])
    else:
        lines.extend([
            "        return value;",
        ])
    lines.extend([
        "    }",
    ])
    return lines


def generate_raw_decrypt(name: str, ops: list, return_style: str, cfg_key_prefix: str = None) -> list:
    lines = [
        f"    inline uint64_t {name}(uint64_t raw_value)",
        "    {",
        "        if (!raw_value) return 0;",
        "        uint64_t value = raw_value;",
        "        uint32_t* data = (uint32_t*)&value;",
        "        uint32_t count = 2;",
        "        do {",
        "            uint32_t x = *data;",
    ]
    lines.extend(append_ops(ops, cfg_key_prefix))
    lines.extend([
        "            data++;",
        "            --count;",
        "        } while (count);",
    ])
    if return_style == "handle":
        lines.extend([
            "        return Il2cppGetHandle(value);",
        ])
    else:
        lines.extend([
            "        return value;",
        ])
    lines.append("    }")
    return lines


def append_ops(ops: list, cfg_key_prefix: str = None) -> list:
    lines = []
    var = "x"
    keys = generate_decrypt_keys(cfg_key_prefix, ops) if cfg_key_prefix else None
    for i, op in enumerate(ops):
        out = f"v{i+1}"
        if keys:
            cfg_key = f"OffsetManager::DecryptCfg.{keys[i][0]}"
        if op["op"] == "rol":
            if cfg_key_prefix:
                lines.append(f"            uint32_t {out} = ({var} << {cfg_key}) | ({var} >> (32 - {cfg_key})); // ROL")
            else:
                lines.append(f"            uint32_t {out} = ({var} << {op['value']}) | ({var} >> (32 - {op['value']})); // ROL {op['value']}")
        elif op["op"] == "ror":
            if cfg_key_prefix:
                lines.append(f"            uint32_t {out} = ({var} >> {cfg_key}) | ({var} << (32 - {cfg_key})); // ROR")
            else:
                lines.append(f"            uint32_t {out} = ({var} >> {op['value']}) | ({var} << (32 - {op['value']})); // ROR {op['value']}")
        elif op["op"] == "add":
            if cfg_key_prefix:
                lines.append(f"            uint32_t {out} = {var} + {cfg_key}; // ADD")
            else:
                lines.append(f"            uint32_t {out} = {var} + 0x{op['value']:08X}u; // ADD 0x{op['value']:08X}")
        elif op["op"] == "sub":
            if cfg_key_prefix:
                lines.append(f"            uint32_t {out} = {var} - {cfg_key}; // SUB")
            else:
                lines.append(f"            uint32_t {out} = {var} - 0x{op['value']:08X}u; // SUB 0x{op['value']:08X}")
        elif op["op"] == "xor":
            if cfg_key_prefix:
                lines.append(f"            uint32_t {out} = {var} ^ {cfg_key}; // XOR")
            else:
                lines.append(f"            uint32_t {out} = {var} ^ 0x{op['value']:08X}u; // XOR 0x{op['value']:08X}")
        var = out
    lines.append(f"            *data = {var};")
    return lines


def write_decrypts_dat(cfg: dict, master: dict):
    """Generate rust_decrypts.dat from decrypt constants in OffsetManager key format."""
    lines = [
        f"# Morphine decrypt constants — loaded by OffsetManager::LoadDecryptConfig()",
        f"# Build: {master.get('build', 'unknown')}",
        f"# Key format: {{prefix}}_{{op_type}} where prefix matches OffsetManager struct fields",
        f"# Duplicate op types get _2, _3 suffix (first occurrence has no suffix)",
        "",
    ]
    mapping = cfg.get("decrypt_mappings", {})
    key_map = cfg.get("decrypt_dat_key_map", {})
    funcs = merge_fallback_funcs(master.get("decrypt_functions", {}))
    seen_project_names = set()

    for morphine_name, project_name in mapping.items():
        if morphine_name not in funcs:
            continue
        if project_name in seen_project_names:
            continue
        seen_project_names.add(project_name)
        prefix = key_map.get(project_name)
        if not prefix:
            continue

        ops = funcs[morphine_name].get("ops", [])
        lines.append(f"# {project_name} ({morphine_name}) prefix={prefix} sequence: {'-'.join(op['op'] for op in ops)}")

        for key_name, op_type, op_val in generate_decrypt_keys(prefix, ops):
            lines.append(f"{key_name}=0x{op_val:08X}")
        lines.append("")

    out_path = Path(__file__).parent / "output" / "rust_decrypts.dat"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"[OK] Wrote {out_path}")


def generate_offsetmanager_patch(cfg: dict, master: dict) -> str:
    """Generate OffsetManager.hpp patch: DecryptConfig struct, LOAD_DEC, and SaveDecryptConfig."""
    mapping = cfg.get("decrypt_mappings", {})
    key_map = cfg.get("decrypt_dat_key_map", {})
    funcs = merge_fallback_funcs(master.get("decrypt_functions", {}))

    struct_lines = []
    load_lines = []
    save_lines = []
    seen_project_names = set()

    for morphine_name, project_name in mapping.items():
        if morphine_name not in funcs:
            continue
        if project_name in seen_project_names:
            continue
        seen_project_names.add(project_name)

        prefix = key_map.get(project_name)
        if not prefix:
            continue

        ops = funcs[morphine_name].get("ops", [])
        keys = generate_decrypt_keys(prefix, ops)

        op_seq = "-".join(op["op"] for op in ops)
        struct_lines.append(f"        // {project_name} ({morphine_name}): {op_seq}")
        load_comment = f"// {project_name}"
        save_comment = f"// {project_name}"

        for key_name, op_type, op_val in keys:
            if op_type in ("rol", "ror"):
                struct_lines.append(f"        uint32_t {key_name} = {op_val};")
            else:
                struct_lines.append(f"        uint32_t {key_name} = 0x{op_val:08X};")
            load_lines.append(f'                        LOAD_DEC("{key_name}", DecryptCfg.{key_name});')
            save_lines.append(f'        W("{key_name}", DecryptCfg.{key_name});')
        struct_lines.append("")
        load_lines.append(f"                        {load_comment}")
        save_lines.append(f"        {save_comment}")

    # Remove trailing comments
    if load_lines and load_lines[-1].strip().startswith("//"):
        load_lines.pop()
    if save_lines and save_lines[-1].strip().startswith("//"):
        save_lines.pop()

    patch = "\n".join([
        "// ============================================================",
        "// Auto-generated OffsetManager.hpp patch from Morphine",
        f"// Build: {master.get('build', 'unknown')}",
        "// ============================================================",
        "",
        "// === DECRYPTCONFIG_STRUCT ===",
        "// Replace struct DecryptConfig body (between { and };)",
        "",
    ])
    patch += "\n".join(struct_lines)
    patch += "\n"
    patch += "\n// === LOAD_DEC_BLOCK ===\n"
    patch += "// Replace between #define LOAD_DEC and #undef LOAD_DEC\n\n"
    patch += "\n".join(load_lines)
    patch += "\n\n// === SAVE_BLOCK ===\n"
    patch += "// Replace between auto W = lambda and CloseHandle in SaveDecryptConfig\n\n"
    patch += "\n".join(save_lines)
    patch += "\n"

    return patch


def main():
    cfg = load_config()
    master = load_master()
    out_dir = Path(__file__).parent / "output"

    # Backups
    backup_dir = create_backups(cfg)

    # Load current files
    offsets_path = Path(cfg["offsets_hpp"])
    sdk_path = Path(cfg["sdk_hpp"])

    offsets_content = offsets_path.read_text(encoding="utf-8", errors="ignore") if offsets_path.exists() else ""
    sdk_content = sdk_path.read_text(encoding="utf-8", errors="ignore") if sdk_path.exists() else ""

    # Generate patches
    offset_patches, offset_changes, missing_offsets = generate_offset_patches(cfg, master, offsets_content)

    # Deduplicate by project field name
    seen_patches = set()
    deduped_patches = []
    deduped_changes = []
    for patch, change in zip(offset_patches, offset_changes):
        if patch["name"] not in seen_patches:
            seen_patches.add(patch["name"])
            deduped_patches.append(patch)
            deduped_changes.append(change)
    offset_patches = deduped_patches
    offset_changes = deduped_changes

    seen_missing = set()
    deduped_missing = []
    for name, value in missing_offsets:
        if name not in seen_missing:
            seen_missing.add(name)
            deduped_missing.append((name, value))
    missing_offsets = deduped_missing

    sdk_patches, sdk_changes = generate_sdk_patches(cfg, master, sdk_content)

    # Write offsets_patch.txt
    offsets_patch_lines = [
        "// ============================================================",
        "// Auto-generated offsets.hpp patch from Morphine",
        f"// Build: {master.get('build', 'unknown')}",
        "// ============================================================",
        "",
    ]
    for patch in offset_patches:
        offsets_patch_lines.append(f"// {patch['name']} (namespace: {patch['namespace']}): 0x{patch['old_value']:08X} -> 0x{patch['new_value']:08X}")
        offsets_patch_lines.append(f"// NS: {patch['namespace']}")
        offsets_patch_lines.append(f"// OLD: {patch['old_line']}")
        offsets_patch_lines.append(f"// NEW: {patch['new_line']}")
        offsets_patch_lines.append("")

    if missing_offsets:
        offsets_patch_lines.append("// MISSING IN offsets.hpp (would need to be added manually):")
        for name, value in missing_offsets:
            offsets_patch_lines.append(f"//   inline uint64_t {name} = 0x{value:08X};")

    (out_dir / "offsets_patch.txt").write_text("\n".join(offsets_patch_lines), encoding="utf-8")

    # Write sdk_patch.txt
    sdk_patch_lines = [
        "// ============================================================",
        "// Auto-generated sdk.hpp patch from Morphine",
        f"// Build: {master.get('build', 'unknown')}",
        "// ============================================================",
        "",
    ]
    if sdk_patches:
        for patch in sdk_patches:
            sdk_patch_lines.append(f"// {patch['name']}: 0x{patch['old_value']:08X} -> 0x{patch['new_value']:08X}")
            sdk_patch_lines.append(f"// OLD: {patch['old_line']}")
            sdk_patch_lines.append(f"// NEW: {patch['new_line']}")
            sdk_patch_lines.append("")
    else:
        sdk_patch_lines.append("// No direct sdk.hpp offset updates required.")
        sdk_patch_lines.append("// Use sdk_decrypt_patch.txt for decrypt function updates.")

    (out_dir / "sdk_patch.txt").write_text("\n".join(sdk_patch_lines), encoding="utf-8")

    # Write decrypt patch
    decrypt_patch = generate_decrypt_patch(master, cfg)
    (out_dir / "sdk_decrypt_patch.txt").write_text(decrypt_patch, encoding="utf-8")

    # Write rust_decrypts.dat
    write_decrypts_dat(cfg, master)

    # Write OffsetManager patch
    offsetmanager_patch = generate_offsetmanager_patch(cfg, master)
    (out_dir / "offsetmanager_patch.txt").write_text(offsetmanager_patch, encoding="utf-8")
    print(f"[OK] Wrote offsetmanager_patch.txt")

    # Materials diff
    materials_changes = []
    materials_path = Path(cfg["materials_hpp"])
    if cfg.get("apply_materials", False) and master.get("materials"):
        current_materials = set()
        if materials_path.exists():
            text = materials_path.read_text(encoding="utf-8", errors="ignore")
            current_materials = set(re.findall(r"static\s+constexpr\s+int\s+(\w+)", text))
        new_materials = set(master["materials"].keys())
        materials_changes = list(new_materials - current_materials)

    # Write summary
    summary_lines = [
        f"Morphine build: {master.get('build', 'unknown')}",
        f"Offsets changed: {len(offset_changes)}",
        f"Decrypt constants changed: {sum(len(funcs.get('ops', [])) for funcs in master.get('decrypt_functions', {}).values())}",
        f"Materials changed: {len(materials_changes)}",
        f"Build status: NOT YET BUILT",
        f"Backups: {backup_dir}",
        f"Patch files:",
        f"  - output/offsets_patch.txt",
        f"  - output/sdk_patch.txt",
        f"  - output/sdk_decrypt_patch.txt",
        f"  - output/offsetmanager_patch.txt",
        f"  - output/rust_decrypts.dat",
    ]
    (out_dir / "summary.txt").write_text("\n".join(summary_lines), encoding="utf-8")

    # Decrypt mapping for diff report
    decrypt_mapping = cfg.get("decrypt_mappings", {})

    # Write diff_report.txt
    diff_lines = [
        f"Morphine Build: {master.get('build', 'unknown')}",
        f"Backup Directory: {backup_dir}",
        "",
        "=== OFFSET CHANGES ===",
    ]
    for name, old, new in offset_changes:
        diff_lines.append(f"{name}: 0x{old:08X} -> 0x{new:08X}")
    if not offset_changes:
        diff_lines.append("(none)")

    diff_lines.extend(["", "=== MISSING IN offsets.hpp ==="])
    for name, val in missing_offsets:
        diff_lines.append(f"{name}: not found, Morphine value 0x{val:08X}")
    if not missing_offsets:
        diff_lines.append("(none)")

    diff_lines.extend(["", "=== DECRYPT FUNCTIONS ==="])
    for morphine_name, info in master.get("decrypt_functions", {}).items():
        project_name = decrypt_mapping.get(morphine_name, morphine_name)
        seq = " -> ".join(f"{op['op'].upper()} 0x{op['value']:08X}" for op in info.get("ops", []))
        diff_lines.append(f"{project_name} ({morphine_name}): {seq}")
        diff_lines.append(f"    input: {info.get('input_style')}, return: {info.get('return_style')}")

    diff_lines.extend(["", "=== MATERIAL CHANGES ==="])
    for name in materials_changes[:20]:
        diff_lines.append(f"+ {name}")
    if len(materials_changes) > 20:
        diff_lines.append(f"... and {len(materials_changes) - 20} more")
    if not materials_changes:
        diff_lines.append("(none)")

    (out_dir / "diff_report.txt").write_text("\n".join(diff_lines), encoding="utf-8")

    print(f"[OK] Generated patches:")
    print(f"    {len(offset_changes)} offset changes")
    print(f"    {len(missing_offsets)} missing offsets")
    print(f"    {len(master.get('decrypt_functions', {}))} decrypt functions")
    print(f"    Backups: {backup_dir}")
    print(f"[OK] Review output/ before applying.")


if __name__ == "__main__":
    main()
