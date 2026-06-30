#!/usr/bin/env python3
"""Parse Morphine offsets.hpp and materials.hpp into master.json."""
import json
import re
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def parse_int(s: str) -> int:
    s = s.strip().rstrip("u").rstrip("U").rstrip("ULL").rstrip("LL")
    if s.startswith("0x"):
        return int(s, 16)
    return int(s)


def extract_static_constexpr(text: str) -> dict:
    """Extract all static constexpr uintptr_t Name = value; lines."""
    result = {}
    pattern = re.compile(r"static\s+constexpr\s+\w+\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)[uUlL]*;")
    for name, value in pattern.findall(text):
        result[name] = parse_int(value)
    return result


def extract_struct(text: str, struct_name: str) -> dict:
    """Extract a struct of static constexpr fields."""
    pattern = re.compile(rf"struct\s+{struct_name}\s*\{{(.*?)\}};", re.DOTALL)
    m = pattern.search(text)
    if not m:
        return {}
    return extract_static_constexpr(m.group(1))


def extract_structs(text: str) -> dict:
    """Extract all structs."""
    structs = {}
    pattern = re.compile(r"struct\s+(\w+)\s*\{(.*?)\};", re.DOTALL)
    for name, body in pattern.findall(text):
        fields = extract_static_constexpr(body)
        if fields:
            structs[name] = fields
    return structs


def extract_build_number(text: str) -> str:
    m = re.search(r'Build\s*=\s*"([^"]+)"', text)
    return m.group(1) if m else "unknown"


def extract_decrypt_functions(text: str) -> dict:
    """Extract decryption:: functions and their bodies."""
    funcs = {}
    # Match functions in namespace decryption
    pattern = re.compile(
        r"(?:inline\s+)?(?:uintptr_t|uint64_t|uint32_t)\s+decryption::(\w+)\s*\([^)]*\)\s*\{(.*?)\n\}",
        re.DOTALL,
    )
    for name, body in pattern.findall(text):
        funcs[name] = body.strip()
    return funcs


def normalize_line(line: str) -> str:
    """Remove casts and std:: prefixes to simplify parsing."""
    line = re.sub(r"\(std::uint32_t\*\)", "", line)
    line = re.sub(r"\(std::uint8_t\*\)", "", line)
    line = re.sub(r"\bstd::\w+\b", "", line)
    line = line.replace("*", "")
    line = line.replace("&", "")
    # Expand compound assignments: val += X → val = val + X
    line = re.sub(r"(\w+)\s*([+\-^|])=\s*", r"\1=\1\2", line)
    line = re.sub(r"\s+", "", line)
    return line


def extract_operations(body: str) -> list:
    """Extract decrypt operation sequence from a C++ function body."""
    ops = []
    lines = [normalize_line(line) for line in body.splitlines()]
    lines = [line for line in lines if line and not line.startswith("//")]

    # Main accumulator variables used in Morphine decrypt functions
    ACCUMULATORS = {"ecx", "edx", "x", "v", "t", "y", "val", "rax", "eax"}

    i = 0
    while i < len(lines):
        line = lines[i]

        # Skip obvious non-operation lines (loop control, pointer math, copies)
        if any(k in line for k in ["do{", "}while", "--", "++", "=rdx", "=0x2", "r8d", "r9d"]) and not any(op in line for op in ["<<", ">>", "+", "-", "^", "|"]):
            i += 1
            continue

        # Multi-line ROL: dst1 = src1 << n; dst2 = src2 >> (32-n); dst3 = dst1 | dst2;
        m1 = re.match(r"(\w+)=(\w+)<<(0x[0-9A-Fa-f]+|\d+)", line)
        if m1 and i + 2 < len(lines):
            dst1, src1, n1 = m1.groups()
            m2 = re.match(r"(\w+)=(\w+)>>(0x[0-9A-Fa-f]+|\d+)", lines[i + 1])
            m3 = re.match(r"(\w+)=(\w+)\|(\w+)", lines[i + 2])
            if m2 and m3:
                dst2, src2, n2 = m2.groups()
                dst3, src3a, src3b = m3.groups()
                n1v = parse_int(n1)
                n2v = parse_int(n2)
                if n1v + n2v == 32 and dst1 == src3a and dst2 == src3b:
                    ops.append({"op": "rol", "value": n1v})
                    i += 3
                    continue

        # Multi-line ROR: dst1 = src1 >> n; dst2 = src2 << (32-n); dst3 = dst1 | dst2;
        m1 = re.match(r"(\w+)=(\w+)>>(0x[0-9A-Fa-f]+|\d+)", line)
        if m1 and i + 2 < len(lines):
            dst1, src1, n1 = m1.groups()
            m2 = re.match(r"(\w+)=(\w+)<<(0x[0-9A-Fa-f]+|\d+)", lines[i + 1])
            m3 = re.match(r"(\w+)=(\w+)\|(\w+)", lines[i + 2])
            if m2 and m3:
                dst2, src2, n2 = m2.groups()
                dst3, src3a, src3b = m3.groups()
                n1v = parse_int(n1)
                n2v = parse_int(n2)
                if n1v + n2v == 32 and dst1 == src3a and dst2 == src3b:
                    ops.append({"op": "ror", "value": n1v})
                    i += 3
                    continue

        # Single-line ROL/ROR: ((x << n) | (x >> (32-n)))
        m = re.search(r"\((\w+)<<(0x[0-9A-Fa-f]+|\d+)\)\|\(\1>>(0x[0-9A-Fa-f]+|\d+)\)", line)
        if m:
            n1 = parse_int(m.group(2))
            n2 = parse_int(m.group(3))
            if n1 + n2 == 32:
                ops.append({"op": "rol", "value": n1})
                i += 1
                continue

        m = re.search(r"\((\w+)>>(0x[0-9A-Fa-f]+|\d+)\)\|\(\1<<(0x[0-9A-Fa-f]+|\d+)\)", line)
        if m:
            n1 = parse_int(m.group(2))
            n2 = parse_int(m.group(3))
            if n1 + n2 == 32:
                ops.append({"op": "ror", "value": n1})
                i += 1
                continue

        # ADD / SUB / XOR on accumulator only
        m = re.match(r"(\w+)=(\w+)([+-^])(0x[0-9A-Fa-f]+|\d+)", line)
        if m:
            dst, src, op_sym, val = m.groups()
            if dst in ACCUMULATORS and src == dst:
                op_map = {"+": "add", "-": "sub", "^": "xor"}
                ops.append({"op": op_map[op_sym], "value": parse_int(val)})
                i += 1
                continue

        i += 1

    return ops


def analyze_decrypt_function(name: str, body: str) -> dict:
    """Analyze a decrypt function: input style, return style, operations."""
    result = {
        "morphine_name": name,
        "input_style": "raw",  # or "pointer" if it reads a1 + offset
        "return_style": "raw",  # or "handle"
        "read_offset": None,
        "ops": [],
    }

    # Check if it reads from input pointer
    m = re.search(r"read<[^>]+>\(\s*a1\s*\+\s*(0x[0-9A-Fa-f]+|\d+)\s*\)", body)
    if m:
        result["input_style"] = "pointer"
        result["read_offset"] = parse_int(m.group(1))

    # Check return style
    last_return = None
    for line in reversed(body.splitlines()):
        line = line.strip()
        if line.startswith("return"):
            last_return = line
            break

    if last_return and ("il2cpp_get_handle" in last_return or "Il2cppGetHandle" in last_return):
        result["return_style"] = "handle"

    result["ops"] = extract_operations(body)
    return result


def extract_namespace_fields_from_morphine(text: str) -> dict:
    """Extract all `namespace X { constexpr ... }` blocks from Morphine output.
    Returns {namespace_name: {field_name: int_value, ...}, ...}."""
    result = {}
    pattern = re.compile(r"namespace\s+(\w+)\s*\{", re.MULTILINE)
    for m in pattern.finditer(text):
        name = m.group(1)
        i = m.end()
        brace_count = 1
        while i < len(text) and brace_count > 0:
            if text[i] == '{':
                brace_count += 1
            elif text[i] == '}':
                brace_count -= 1
            i += 1
        if brace_count == 0:
            block = text[m.end():i - 1]
            fields = {}
            for fm in re.finditer(r'constexpr\s+(?:std::)?\w+\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*;', block):
                fields[fm.group(1)] = parse_int(fm.group(2))
            if fields:
                result[name] = fields
    return result


def parse_materials(text: str) -> dict:
    result = {}
    pattern = re.compile(r"static\s+constexpr\s+int\s+(\w+)\s*=\s*(-?\d+);")
    for name, value in pattern.findall(text):
        result[name] = int(value)
    return result


def main():
    cfg = load_config()
    out_dir = Path(__file__).parent / "output"

    offsets_path = out_dir / "offsets.hpp"
    materials_path = out_dir / "materials.hpp"

    if not offsets_path.exists():
        print(f"[ERROR] {offsets_path} not found. Morphine fetch failed.")
        print("        Pipeline will use sha-dumper fallback if available.")
        import sys
        sys.exit(1)

    offsets_text = offsets_path.read_text(encoding="utf-8", errors="ignore")

    data = {
        "build": extract_build_number(offsets_text),
        "source_url": cfg["morphine_offsets_url"],
        "offsets": extract_static_constexpr(offsets_text),
        "structs": extract_structs(offsets_text),
        "namespaces": extract_namespace_fields_from_morphine(offsets_text),
        "il2cpp_api": extract_struct(offsets_text, "il2cpp_api"),
        "gc_handles": extract_struct(offsets_text, "gc_handles"),
        "klass_rvas": extract_struct(offsets_text, "klass_rvas"),
        "decrypt_functions": {},
        "materials": {},
    }

    # Decrypt functions
    funcs = extract_decrypt_functions(offsets_text)
    for name, body in funcs.items():
        data["decrypt_functions"][name] = analyze_decrypt_function(name, body)

    # Materials
    if materials_path.exists():
        materials_text = materials_path.read_text(encoding="utf-8", errors="ignore")
        data["materials"] = parse_materials(materials_text)

    # Save
    master_json = out_dir / "master.json"
    with open(master_json, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)

    print(f"[OK] Parsed build {data['build']}:")
    print(f"    {len(data['offsets'])} top-level offsets")
    print(f"    {len(data['structs'])} structs")
    print(f"    {len(data['namespaces'])} namespaces")
    print(f"    {len(data['il2cpp_api'])} il2cpp API entries")
    print(f"    {len(data['gc_handles'])} gc handle entries")
    print(f"    {len(data['decrypt_functions'])} decrypt functions")
    print(f"    {len(data['materials'])} materials")
    print(f"[OK] Wrote {master_json}")


if __name__ == "__main__":
    main()
