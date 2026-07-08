#!/usr/bin/env python3
"""
cross_validate_offsets.py - Cross-validate offsets.hpp against Frida runtime dump.

Uses offset-based matching to handle obfuscated field names (%hash).
The hash is based on the field NAME, not the offset, so it stays stable
across game updates. By building a mapping {hash -> our_field_name} using
the current Frida dump (matched by offset value), we can detect offset
changes in future updates.

Output:
  - output/frida_field_mapping.json: {class: {our_field: frida_hash}}
  - output/cross_validation_report.txt: disagreements and match counts
  - output/frida_patches.txt: patches for offsets that changed
"""
import json
import re
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "output"
FRIDA_JSON = OUTPUT_DIR / "frida_validation.json"
MAPPING_FILE = OUTPUT_DIR / "frida_field_mapping.json"
REPORT = OUTPUT_DIR / "cross_validation_report.txt"
PATCHES = OUTPUT_DIR / "frida_patches.txt"

# Load config.json for namespace -> our_offsets_namespace mapping
CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def load_json(path):
    if not path.exists():
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def extract_namespace_block(content, namespace):
    """Extract a namespace block from offsets.hpp using brace counting."""
    start_pat = re.compile(rf"namespace\s+{re.escape(namespace)}\b", re.MULTILINE)
    start_m = start_pat.search(content)
    if not start_m:
        return ""
    i = start_m.end()
    while i < len(content) and content[i] != '{':
        i += 1
    if i >= len(content):
        return ""
    brace_count = 1
    i += 1
    while i < len(content) and brace_count > 0:
        if content[i] == '{':
            brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
        i += 1
    return content[start_m.start():i]


def extract_fields(content, namespace):
    """Extract {field_name: int_value} from a namespace block in offsets.hpp."""
    block = extract_namespace_block(content, namespace)
    fields = {}
    for m in re.finditer(r'(?:inline|constexpr)\s+uint64_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;', block):
        val_str = m.group(2)
        fields[m.group(1)] = int(val_str, 0)
    return fields


def normalize_hex(v):
    s = str(v).strip().lower().replace("0x", "")
    try:
        return f"{int(s, 16):x}"
    except ValueError:
        return s


def build_offset_to_frida_fields(frida_offsets):
    """Build {class_name: {offset_int: [field_names]}} from Frida data."""
    result = {}
    for cls, fields in frida_offsets.items():
        result[cls] = {}
        for fname, offset_str in fields.items():
            try:
                offset_int = int(offset_str, 16)
            except (ValueError, TypeError):
                continue
            if offset_int == 0:
                continue
            if offset_int not in result[cls]:
                result[cls][offset_int] = []
            result[cls][offset_int].append(fname)
    return result


def build_mapping_from_current(offsets_hpp, frida_data, cfg):
    """Build {class: {our_field: frida_hash}} by matching current offset values."""
    ns_mappings = cfg.get("namespace_field_mappings", {})
    frida_offsets = frida_data.get("offsets", {})
    frida_by_offset = build_offset_to_frida_fields(frida_offsets)

    mapping = {}
    matched = 0
    unmatched = 0

    for morphine_key, ns_info in ns_mappings.items():
        our_ns = ns_info.get("namespace", "")
        if not our_ns:
            continue
        our_fields = extract_fields(offsets_hpp, our_ns)
        if not our_fields:
            continue

        # Find matching Frida class by trying our_ns and morphine_key
        frida_cls = None
        for candidate in [our_ns, morphine_key]:
            if candidate in frida_offsets:
                frida_cls = candidate
                break
        if not frida_cls:
            continue

        cls_offset_map = frida_by_offset.get(frida_cls, {})
        if our_ns not in mapping:
            mapping[our_ns] = {"_frida_class": frida_cls}

        for field_name, offset_val in our_fields.items():
            if offset_val == 0:
                continue
            candidates = cls_offset_map.get(offset_val, [])
            if len(candidates) == 1:
                mapping[our_ns][field_name] = candidates[0]
                matched += 1
            elif len(candidates) > 1:
                # Multiple Frida fields at same offset — pick first (usually fine)
                mapping[our_ns][field_name] = candidates[0]
                matched += 1
            else:
                unmatched += 1

    return mapping, matched, unmatched


def cross_validate_with_mapping(offsets_hpp, frida_data, mapping, cfg):
    """Use saved mapping to find Frida offsets by hash, compare with our offsets."""
    frida_offsets = frida_data.get("offsets", {})
    patches = []
    agreements = 0
    disagreements = 0
    not_found = 0

    ns_mappings = cfg.get("namespace_field_mappings", {})

    for our_ns, field_map in mapping.items():
        if not isinstance(field_map, dict):
            continue
        frida_cls = field_map.get("_frida_class", our_ns)
        frida_fields = frida_offsets.get(frida_cls, {})
        if not frida_fields:
            continue

        # Reverse: {frida_hash: offset_str}
        frida_lookup = {}
        for fname, offset_str in frida_fields.items():
            frida_lookup[fname] = offset_str

        our_fields = extract_fields(offsets_hpp, our_ns)
        for field_name, our_offset in our_fields.items():
            if our_offset == 0:
                continue
            frida_hash = field_map.get(field_name)
            if not frida_hash:
                not_found += 1
                continue
            frida_offset_str = frida_lookup.get(frida_hash)
            if not frida_offset_str:
                not_found += 1
                continue
            frida_offset = int(frida_offset_str, 16)
            if frida_offset == our_offset:
                agreements += 1
            else:
                disagreements += 1
                patches.append({
                    "namespace": our_ns,
                    "field": field_name,
                    "old_value": our_offset,
                    "new_value": frida_offset,
                    "frida_hash": frida_hash,
                })

    return patches, agreements, disagreements, not_found


def main():
    print("[CROSS] Cross-validating offsets against Frida dump...")
    cfg = load_config()
    offsets_hpp_path = Path(cfg["offsets_hpp"])
    offsets_hpp = offsets_hpp_path.read_text(encoding="utf-8", errors="ignore") if offsets_hpp_path.exists() else ""

    frida_data = load_json(FRIDA_JSON)
    if not frida_data or not frida_data.get("offsets"):
        print("[WARN] No Frida data available (frida_validation.json not found).")
        print("[WARN] Run getnewoffsets.bat with Rust in-game to generate Frida dump.")
        return 0

    frida_classes = len(frida_data.get("offsets", {}))
    print(f"[OK] Frida: {frida_classes} classes loaded")

    # Load saved mapping or build new one
    # The mapping maps our field names to Frida hashes (which are name-based, not offset-based).
    # Hashes stay the same across game updates unless the game renames fields.
    # So the mapping should be REUSED — only build new if none exists.
    saved_mapping = load_json(MAPPING_FILE)
    if saved_mapping and len(saved_mapping) > 2:  # has actual class entries
        print("[OK] Using saved Frida field mapping (hashes are stable across updates)")
        mapping = saved_mapping
    else:
        print("[INFO] Building Frida field mapping from current offsets...")
        print("[INFO] (This requires offsets.hpp and Frida to be in sync — same game version)")
        mapping, matched, unmatched = build_mapping_from_current(offsets_hpp, frida_data, cfg)
        mapping["_build_version"] = frida_data.get("unity_version", "unknown")
        with open(MAPPING_FILE, "w", encoding="utf-8") as f:
            json.dump(mapping, f, indent=2)
        print(f"[OK] Mapping: {matched} fields matched, {unmatched} unmatched")
        print(f"[OK] Saved mapping to {MAPPING_FILE}")

    # Cross-validate
    patches, agreements, disagreements, not_found = cross_validate_with_mapping(
        offsets_hpp, frida_data, mapping, cfg
    )

    # Write patches in offsets_patch.txt format (NS/OLD/NEW) so apply_patches.py can apply them
    patch_lines = []
    if patches:
        patch_lines.append("// ============================================================")
        patch_lines.append("// Frida-validated offset changes (auto-applicable by apply_patches.py)")
        patch_lines.append(f"// {len(patches)} offsets changed since last mapping")
        patch_lines.append("// ============================================================")
        patch_lines.append("")
        for p in patches:
            old_hex = f"0x{p['old_value']:08X}"
            new_hex = f"0x{p['new_value']:08X}"
            patch_lines.append(f"// {p['namespace']}::{p['field']} (hash: {p['frida_hash']}): {old_hex} -> {new_hex}")
            patch_lines.append(f"// NS: {p['namespace']}")
            patch_lines.append(f"// OLD: inline uint64_t  {p['field']} = {old_hex};")
            patch_lines.append(f"// NEW: inline uint64_t  {p['field']} = {new_hex};")
            patch_lines.append("")
    else:
        patch_lines.append("// No Frida-validated offset changes detected.")
        patch_lines.append("// All offsets match the Frida runtime dump.")
    PATCHES.write_text("\n".join(patch_lines), encoding="utf-8")

    # Write report
    report_lines = [
        f"Cross-Validation Report",
        f"========================",
        f"Frida classes:       {frida_classes}",
        f"Agreements:          {agreements}",
        f"Disagreements:       {disagreements}",
        f"Fields not in Frida: {not_found}",
        f"",
        f"Patches generated:    {len(patches)}",
        f"Patch file:           {PATCHES}",
        f"",
    ]
    if patches:
        report_lines.append("Changed offsets:")
        for p in patches:
            old_hex = f"0x{p['old_value']:X}"
            new_hex = f"0x{p['new_value']:X}"
            report_lines.append(f"  {p['namespace']}::{p['field']}: {old_hex} -> {new_hex}")
    else:
        report_lines.append("All offsets match Frida validation. No changes needed.")

    REPORT.write_text("\n".join(report_lines), encoding="utf-8")

    print(f"\n[CROSS] Results:")
    print(f"  Agreements:        {agreements}")
    print(f"  Disagreements:     {disagreements}")
    print(f"  Fields not in Frida: {not_found}")
    print(f"  Patches:            {len(patches)}")
    print(f"  Report:             {REPORT}")
    print(f"  Patches file:       {PATCHES}")

    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
