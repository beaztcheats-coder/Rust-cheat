#!/usr/bin/env python3
"""parse_morphine_dump.py - Parse morphine-dumper_output.h into master.json

Reads the Morphine dumper output header file and extracts:
- All namespace/struct field offsets
- All static pointer RVAs (klass_rvas)
- All decryption function implementations
- Build ID

Outputs:
  output/master.json          - Unified offsets + decrypts for compare_and_patch.py
  output/rust_decrypts.dat    - Decrypt constants for OffsetManager
"""
import json
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
OUTPUT_DIR = SCRIPT_DIR / "output"


def parse_header(path):
    """Parse morphine-dumper_output.h into structured data."""
    if not path.exists():
        print(f"[ERROR] Morphine dump not found: {path}")
        return None

    content = path.read_text(encoding="utf-8", errors="ignore")

    # Extract build ID
    build_match = re.search(r'inline\s+std::string\s+Build\s*=\s*"([^"]+)"', content)
    build_id = build_match.group(1) if build_match else "unknown"
    print(f"[morphine] Build: {build_id}")

    # Parse all namespace/struct blocks
    # Pattern: namespace NAME { ... } or struct NAME { ... }
    ns_pattern = re.compile(
        r'(?:namespace|struct)\s+(\w+)\s*\{([^}]*(?:\{[^}]*\}[^}]*)*)\}',
        re.MULTILINE
    )

    master = {
        "build": build_id,
        "source": "morphine_dumper",
        "offsets": {
            "static_pointers": {},
            "structs": {},
        },
        "klass_rvas": {},
        "decrypt_functions": {},
        "namespaces": {},
    }

    field_count = 0
    ns_count = 0

    for match in ns_pattern.finditer(content):
        ns_name = match.group(1)
        body = match.group(2)

        # Extract fields: constexpr uintptr_t NAME = VALUE;
        # Also: inline static constexpr uintptr_t NAME = VALUE;
        field_pattern = re.compile(
            r'(?:inline\s+)?(?:static\s+)?constexpr\s+(?:std::)?uintptr_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)',
            re.MULTILINE
        )
        # Also uint32_t fields
        field_pattern_u32 = re.compile(
            r'(?:inline\s+)?(?:static\s+)?constexpr\s+(?:std::)?uint32_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)',
            re.MULTILINE
        )

        fields = {}
        for fm in field_pattern.finditer(body):
            fname = fm.group(1)
            fval = int(fm.group(2), 0)
            fields[fname] = fval
            field_count += 1

        for fm in field_pattern_u32.finditer(body):
            fname = fm.group(1)
            fval = int(fm.group(2), 0)
            if fname not in fields:
                fields[fname] = fval
                field_count += 1

        if not fields:
            continue

        ns_count += 1

        # Categorize the namespace
        if ns_name == "klass_rvas":
            # Static pointer RVAs
            for fname, fval in fields.items():
                master["klass_rvas"][fname] = fval
        elif ns_name in ("gc_handles", "il2cpp_api"):
            # Special structs - store as-is
            master["namespaces"][ns_name] = fields
        else:
            # Regular struct fields
            master["offsets"]["structs"][ns_name] = fields

    # Parse top-level static pointers (il2cpphandle, etc.)
    top_level_pattern = re.compile(
        r'inline\s+static\s+constexpr\s+uintptr_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+)',
        re.MULTILINE
    )
    for match in top_level_pattern.finditer(content):
        name = match.group(1)
        val = int(match.group(2), 0)
        if name not in master["klass_rvas"] and name != "Build":
            master["offsets"]["static_pointers"][name] = val

    # Parse decryption functions
    parse_decryption_functions(content, master)

    print(f"[morphine] Parsed {ns_count} namespaces, {field_count} fields")
    print(f"[morphine] klass_rvas: {len(master['klass_rvas'])}")
    print(f"[morphine] structs: {len(master['offsets']['structs'])}")
    print(f"[morphine] decrypt_functions: {len(master['decrypt_functions'])}")

    return master


def parse_decryption_functions(content, master):
    """Parse decryption function implementations from the dump."""
    # Pattern: uintptr_t decryption::NAME(uint64_t ...) { ... }
    # Extract ops from the function body
    func_pattern = re.compile(
        r'(?:uintptr_t|inline\s+uint32_t)\s+decryption::(\w+)\s*\([^)]*\)\s*\{([^}]*(?:\{[^}]*\}[^}]*)*)\}',
        re.MULTILINE
    )

    for match in func_pattern.finditer(content):
        func_name = match.group(1)
        body = match.group(2)

        ops = []
        is_raw_return = "return a1;" in body or "return il2cpp_get_handle" in body

        # Extract operations: edx = edx - 0x... / edx = edx + 0x... / edx = edx ^ 0x... / edx = (edx << 0x..) | (edx >> 0x..)
        # For ROL: (edx << 0xN) | (edx >> 0xM) where N + M = 32
        # For direct functions: edx = ...
        # For loop functions: ecx = ...

        # Try direct decrypt pattern (edx-based)
        add_matches = re.findall(r'edx\s*=\s*edx\s*\+\s*(0x[0-9A-Fa-f]+)', body)
        sub_matches = re.findall(r'edx\s*=\s*edx\s*-\s*(0x[0-9A-Fa-f]+)', body)
        xor_matches = re.findall(r'edx\s*=\s*edx\s*\^\s*(0x[0-9A-Fa-f]+)', body)
        rol_matches = re.findall(r'edx\s*=\s*\(edx\s*<<\s*(0x[0-9A-Fa-f]+)\)\s*\|\s*\(edx\s*>>\s*(0x[0-9A-Fa-f]+)\)', body)

        if not add_matches and not sub_matches and not xor_matches and not rol_matches:
            # Try loop decrypt pattern (ecx-based)
            add_matches = re.findall(r'ecx\s*=\s*ecx\s*\+\s*(0x[0-9A-Fa-f]+)', body)
            sub_matches = re.findall(r'ecx\s*=\s*ecx\s*-\s*(0x[0-9A-Fa-f]+)', body)
            xor_matches = re.findall(r'ecx\s*=\s*ecx\s*\^\s*(0x[0-9A-Fa-f]+)', body)
            rol_matches = re.findall(r'ecx\s*=\s*ecx\s*<<\s*(0x[0-9A-Fa-f]+)', body)

        # Also try val-based pattern (for fov)
        if not add_matches and not sub_matches and not xor_matches and not rol_matches:
            add_matches = re.findall(r'val\s*\+=\s*(0x[0-9A-Fa-f]+)', body)
            sub_matches = re.findall(r'val\s*-=\s*(0x[0-9A-Fa-f]+)', body)
            xor_matches = re.findall(r'val\s*\^=\s*(0x[0-9A-Fa-f]+)', body)
            # ROL for val: val = (val << N) | (val >> M)
            rol_matches = re.findall(r'val\s*=\s*\(val\s*<<\s*(0x[0-9A-Fa-f]+)\)\s*\|\s*\(val\s*>>\s*(0x[0-9A-Fa-f]+)\)', body)

        # Reconstruct operations in order by scanning the body line by line
        ops = extract_ops_from_body(body)

        if ops:
            # Map function names to canonical names
            name_map = {
                "client_entities": "base_networkable_0",
                "entity_list": "base_networkable_1",
                "cl_active_item": "cl_active_item",
                "player_inventory": "player_inventory",
                "player_eyes": "player_eyes",
                "decrypt_fov": "decrypt_fov",
                "encrypt_fov": "encrypt_fov",
            }
            canonical = name_map.get(func_name, func_name)

            master["decrypt_functions"][canonical] = {
                "ops": [{"op": o[0], "value": o[1]} for o in ops],
                "ops_raw": [(o[0], o[1]) for o in ops],
                "input_style": "pointer" if is_raw_return and "il2cpp_get_handle" in body else "raw",
                "return_style": "handle" if "il2cpp_get_handle" in body else "raw",
                "confidence": "morphine",
                "source": "morphine_dumper",
            }
            print(f"  [decrypt] {func_name} → {canonical}: {len(ops)} ops")


def extract_ops_from_body(body):
    """Extract operations in order from a decrypt function body."""
    ops = []
    lines = body.split('\n')

    for line in lines:
        line = line.strip()

        # Direct pattern: edx = edx OP value
        # ROL: edx = (edx << N) | (edx >> M)
        rol_match = re.match(r'(?:edx|ecx|val)\s*=\s*\((?:edx|ecx|val)\s*<<\s*(0x[0-9A-Fa-f]+)\)\s*\|\s*\((?:edx|ecx|val)\s*>>\s*(0x[0-9A-Fa-f]+)\)', line)
        if rol_match:
            shift = int(rol_match.group(1), 0)
            shift2 = int(rol_match.group(2), 0)
            if shift + shift2 == 32:
                ops.append(("rol", shift))
            continue

        # ROL via separate lines: ecx = ecx << N / eax = eax >> M / ecx = ecx | eax
        # Skip these complex patterns — they're handled by the single-line version

        # Simple ops
        add_match = re.match(r'(?:edx|ecx|val)\s*=\s*(?:edx|ecx|val)\s*\+\s*(0x[0-9A-Fa-f]+)', line)
        if add_match:
            ops.append(("add", int(add_match.group(1), 0)))
            continue

        sub_match = re.match(r'(?:edx|ecx|val)\s*=\s*(?:edx|ecx|val)\s*-\s*(0x[0-9A-Fa-f]+)', line)
        if sub_match:
            ops.append(("sub", int(sub_match.group(1), 0)))
            continue

        xor_match = re.match(r'(?:edx|ecx|val)\s*=\s*(?:edx|ecx|val)\s*\^\s*(0x[0-9A-Fa-f]+)', line)
        if xor_match:
            ops.append(("xor", int(xor_match.group(1), 0)))
            continue

        # val += / -= / ^= patterns
        add_match2 = re.match(r'val\s*\+=\s*(0x[0-9A-Fa-f]+)', line)
        if add_match2:
            ops.append(("add", int(add_match2.group(1), 0)))
            continue

        sub_match2 = re.match(r'val\s*-=\s*(0x[0-9A-Fa-f]+)', line)
        if sub_match2:
            ops.append(("sub", int(sub_match2.group(1), 0)))
            continue

        xor_match2 = re.match(r'val\s*\^=\s*(0x[0-9A-Fa-f]+)', line)
        if xor_match2:
            ops.append(("xor", int(xor_match2.group(1), 0)))
            continue

    return ops


def generate_decrypts_dat(master):
    """Generate rust_decrypts.dat from morphine decrypt functions."""
    # Map canonical names to .dat key prefixes
    prefix_map = {
        "base_networkable_0": "nk",
        "base_networkable_1": "nk2",
        "cl_active_item": "cla",
        "player_inventory": "inv",
        "player_eyes": "ey",
        "decrypt_fov": "fov",
    }

    lines = [
        f"# Decrypt constants — generated from Morphine dumper",
        f"# Build: {master.get('build', 'unknown')}",
        f"# Source: morphine_dumper",
        "",
    ]

    for canonical, prefix in prefix_map.items():
        func = master["decrypt_functions"].get(canonical)
        if not func:
            continue

        ops = func.get("ops_raw", func.get("ops", []))
        if not ops:
            continue

        # Generate keys: {prefix}_{op_type} with _2 suffix for duplicates
        op_type_counts = {}
        dat_entries = []

        for op in ops:
            op_type = op["op"] if isinstance(op, dict) else op[0]
            op_val = op["value"] if isinstance(op, dict) else op[1]

            count = op_type_counts.get(op_type, 0) + 1
            op_type_counts[op_type] = count
            suffix = f"_{count}" if count > 1 else ""
            key = f"{prefix}_{op_type}{suffix}"
            dat_entries.append(f"{key}=0x{op_val:X}")

        lines.append(f"# {canonical}")
        lines.extend(dat_entries)
        lines.append("")

    return "\n".join(lines)


def parse_json(path):
    """Parse morphine-dumper_output.json into master.json format.
    
    This is the preferred input — the DLL writes JSON directly, avoiding
    fragile .h file regex parsing. Falls back to .h parsing if JSON missing.
    """
    if not path.exists():
        print(f"[ERROR] Morphine JSON not found: {path}")
        return None

    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    build_id = data.get("build", "unknown")
    print(f"[morphine-json] Build: {build_id}")

    master = {
        "build": build_id,
        "source": "morphine_dumper_json",
        "offsets": {
            "static_pointers": {},
            "structs": {},
        },
        "klass_rvas": {},
        "decrypt_functions": {},
        "encrypt_functions": {},
        "namespaces": {},
    }

    sections = data.get("sections", {})
    field_count = 0

    for section_name, fields in sections.items():
        if not isinstance(fields, dict):
            continue

        if section_name == "klass_rvas":
            for fname, fval in fields.items():
                master["klass_rvas"][fname] = int(str(fval), 0)
                field_count += 1
        elif section_name in ("gc_handles", "il2cpp_api"):
            master["namespaces"][section_name] = {k: int(str(v), 0) for k, v in fields.items()}
            field_count += len(fields)
        elif section_name == "globals":
            for fname, fval in fields.items():
                master["offsets"]["static_pointers"][fname] = int(str(fval), 0)
                field_count += 1
        else:
            parsed = {k: int(str(v), 0) for k, v in fields.items()}
            master["offsets"]["structs"][section_name] = parsed
            field_count += len(parsed)

    # Parse decrypt functions
    name_map = {
        "client_entities": "base_networkable_0",
        "entity_list": "base_networkable_1",
        "cl_active_item": "cl_active_item",
        "player_inventory": "player_inventory",
        "player_eyes": "player_eyes",
        "decrypt_fov": "decrypt_fov",
    }

    for fn_name, fn_data in data.get("decrypt_functions", {}).items():
        canonical = name_map.get(fn_name, fn_name)
        ops = []
        for op in fn_data.get("ops", []):
            ops.append({"op": op["op"], "value": int(str(op["value"]), 0)})

        read_offset_str = fn_data.get("read_offset")
        read_offset = int(str(read_offset_str), 0) if read_offset_str else None

        master["decrypt_functions"][canonical] = {
            "ops": ops,
            "ops_raw": [(o["op"], o["value"]) for o in ops],
            "fn_rva": int(str(fn_data.get("fn_rva", "0")), 0),
            "input_style": fn_data.get("input_style", "raw"),
            "return_style": fn_data.get("return_style", "raw"),
            "read_offset": read_offset,
            "confidence": "morphine",
            "source": "morphine_dumper_json",
        }
        print(f"  [decrypt] {fn_name} → {canonical}: {len(ops)} ops, fn_rva=0x{master['decrypt_functions'][canonical]['fn_rva']:X}")

    # Parse encrypt functions (auto-generated by the dumper)
    for fn_name, fn_data in data.get("encrypt_functions", {}).items():
        canonical = name_map.get(fn_name, fn_name)
        if canonical == "encrypt_fov":
            canonical_enc = "encrypt_fov"
        else:
            canonical_enc = f"encrypt_{canonical}"

        ops = []
        for op in fn_data.get("ops", []):
            ops.append({"op": op["op"], "value": int(str(op["value"]), 0)})

        master["encrypt_functions"][canonical_enc] = {
            "ops": ops,
            "fn_rva": int(str(fn_data.get("fn_rva", "0")), 0),
            "confidence": fn_data.get("confidence", "auto-generated"),
            "source": "morphine_dumper_json",
        }
        print(f"  [encrypt] {canonical_enc}: {len(ops)} ops")

    print(f"[morphine-json] Parsed {field_count} fields, {len(master['klass_rvas'])} klass_rvas")
    print(f"[morphine-json] {len(master['decrypt_functions'])} decrypt functions, {len(master['encrypt_functions'])} encrypt functions")

    return master


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Parse Morphine dumper output")
    parser.add_argument("--input", type=Path, default=Path.home() / "Desktop" / "morphine-dumper_output.h",
                        help="Path to morphine-dumper_output.h (or .json)")
    parser.add_argument("--output-dir", type=Path, default=OUTPUT_DIR,
                        help="Output directory")
    args = parser.parse_args()

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Prefer JSON if available (written by the DLL directly — no regex parsing needed)
    json_path = args.input.with_suffix(".json")
    if json_path.exists():
        print(f"[morphine] Using JSON input: {json_path}")
        master = parse_json(json_path)
    elif args.input.exists():
        print(f"[morphine] JSON not found, falling back to .h: {args.input}")
        master = parse_header(args.input)
    else:
        print(f"[ERROR] No morphine dump found at {args.input} or {json_path}")
        sys.exit(1)

    if not master:
        sys.exit(1)

    # Save master.json
    master_path = args.output_dir / "master.json"
    with open(master_path, "w", encoding="utf-8") as f:
        json.dump(master, f, indent=2)
    print(f"[OK] master.json written to {master_path}")

    # Save rust_decrypts.dat
    dat_content = generate_decrypts_dat(master)
    dat_path = args.output_dir / "rust_decrypts.dat"
    dat_path.write_text(dat_content, encoding="utf-8")
    print(f"[OK] rust_decrypts.dat written to {dat_path}")

    # Summary
    print(f"\n=== Morphine Dump Summary ===")
    print(f"Build: {master['build']}")
    print(f"Structs: {len(master['offsets']['structs'])}")
    print(f"Static pointers: {len(master['offsets']['static_pointers'])}")
    print(f"Klass RVAs: {len(master['klass_rvas'])}")
    print(f"Decrypt functions: {len(master['decrypt_functions'])}")
    print(f"Encrypt functions: {len(master.get('encrypt_functions', {}))}")
    fn_rva_count = sum(1 for f in master['decrypt_functions'].values() if f.get('fn_rva', 0) > 0)
    print(f"Decrypt fn_rvas: {fn_rva_count}/{len(master['decrypt_functions'])}")


if __name__ == "__main__":
    main()
