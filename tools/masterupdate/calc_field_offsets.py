#!/usr/bin/env python3
"""Calculate field offsets from Il2CppInspector C++ struct definitions.
Parses il2cpp-types.h to find struct definitions and compute byte offsets."""
import re
import sys
from pathlib import Path

TYPES_H = Path(__file__).parent / "output" / "il2cppinspector" / "cpp" / "appdata" / "il2cpp-types.h"

# Known type sizes on x64
TYPE_SIZES = {
    "bool": 1, "int8_t": 1, "uint8_t": 1, "char": 1,
    "int16_t": 2, "uint16_t": 2,
    "int32_t": 4, "uint32_t": 4, "float": 4,
    "int64_t": 8, "uint64_t": 8, "double": 8,
    "il2cpp_array_size_t": 8,
}

# Structs we've already sized (cache)
struct_sizes = {}

# Known struct sizes
KNOWN_SIZES = {
    "Vector3": 12, "Vector4": 16, "Quaternion": 16,
    "Color": 16, "Color32": 4,
}

def get_type_size(type_str, lines_cache=None):
    """Get the byte size of a C++ type string."""
    type_str = type_str.strip().rstrip(";").strip()

    # Pointer
    if "*" in type_str:
        return 8

    # Array (e.g., "Type[32]")
    m = re.match(r'(.+?)\[(\d+)\]', type_str)
    if m:
        base_size = get_type_size(m.group(1), lines_cache)
        return base_size * int(m.group(2))

    # Basic types
    for t, s in TYPE_SIZES.items():
        if type_str == t:
            return s

    # Known struct sizes
    for t, s in KNOWN_SIZES.items():
        if type_str == t:
            return s

    # Nested struct (e.g., "struct BaseCombatEntity__Fields")
    if type_str.startswith("struct "):
        struct_name = type_str[7:].strip()
        if struct_name in struct_sizes:
            return struct_sizes[struct_name]
        if struct_name in KNOWN_SIZES:
            return KNOWN_SIZES[struct_name]
        # Try to find and parse this struct
        if lines_cache:
            size = parse_struct_size(struct_name, lines_cache)
            if size:
                struct_sizes[struct_name] = size
                return size

    # Default: assume 8 (pointer-sized)
    return 8

def get_type_alignment(size):
    """Get alignment for a type of given size."""
    if size <= 1: return 1
    if size <= 2: return 2
    if size <= 4: return 4
    return 8

def parse_struct_size(name, lines):
    """Find and parse a struct to calculate its total size."""
    pattern = re.compile(rf'^struct {re.escape(name)} \{{')
    for i, line in enumerate(lines):
        if pattern.match(line.strip()):
            return parse_struct_fields(lines, i, name)
    return None

def parse_struct_fields(lines, start_idx, struct_name):
    """Parse struct fields starting at start_idx, return total size."""
    offset = 0
    depth = 1
    i = start_idx + 1

    while i < len(lines) and depth > 0:
        line = lines[i].strip()
        if "{" in line:
            depth += 1
        if "}" in line:
            depth -= 1
            if depth == 0:
                break
            i += 1
            continue

        # Skip preprocessor directives
        if line.startswith("#"):
            i += 1
            continue

        # Parse field: "type name;" or "type *name;" or "type name[N];"
        # Remove trailing semicolon
        field = line.rstrip(";").strip()
        if not field or field == "}":
            i += 1
            continue

        # Split type and name (last token is the name)
        parts = field.rsplit(None, 1)
        if len(parts) < 2:
            i += 1
            continue

        type_str, name = parts[0], parts[1]

        # Calculate field size
        size = get_type_size(type_str, lines)
        align = get_type_alignment(size)

        # Align offset
        offset = (offset + align - 1) & ~(align - 1)

        # Store field offset
        if struct_name == "BasePlayer__Fields" or struct_name == "BaseCombatEntity__Fields":
            clean_name = name.replace("*", "").strip()
            print(f"    +0x{offset:03X} ({offset:3d})  {type_str:40s}  {clean_name}")

        offset += size
        i += 1

    # Final alignment
    align = get_type_alignment(8)  # Struct alignment is typically 8
    offset = (offset + align - 1) & ~(align - 1)

    return offset

def main():
    print(f"[INFO] Loading {TYPES_H}...")
    lines = TYPES_H.read_text(encoding="utf-8", errors="ignore").splitlines()
    print(f"[INFO] {len(lines)} lines loaded")

    # Parse BaseCombatEntity__Fields first (BasePlayer inherits from it)
    print("\n=== BaseCombatEntity__Fields ===")
    bce_size = parse_struct_size("BaseCombatEntity__Fields", lines)
    if bce_size:
        struct_sizes["BaseCombatEntity__Fields"] = bce_size
        print(f"  Total size: {bce_size} bytes (0x{bce_size:X})")

    # Parse BasePlayer__Fields
    print("\n=== BasePlayer__Fields ===")
    bp_size = parse_struct_size("BasePlayer__Fields", lines)
    if bp_size:
        struct_sizes["BasePlayer__Fields"] = bp_size
        print(f"  Total size: {bp_size} bytes (0x{bp_size:X})")

    # Compare with current offsets.hpp values
    print("\n=== COMPARISON WITH offsets.hpp ===")
    current_bp = {
        "PlayerEyes": 0x2E0,
        "PlayerInventory": 0x2B8,
        "PlayerInput": 0x560,
        "PlayerModel": 0x678,
        "PlayerFlags": 0x670,
        "ClActiveItem": 0x520,
        "BaseMovement": 0x388,
        "DisplayName": 0x488,
        "CurrentTeam": 0x4F0,
    }

    # The BasePlayer__Fields struct starts after the BaseCombatEntity__Fields (base class)
    # But in IL2CPP, the field offsets are from the START of the object (BasePlayer),
    # not from the start of BasePlayer__Fields.
    # Object header: klass (8) + monitor (8) = 16 bytes = 0x10
    # Then BaseCombatEntity__Fields starts at 0x10
    # Then BasePlayer-specific fields start at 0x10 + bce_size

    if bce_size:
        bp_field_start = 0x10 + bce_size  # Object header + base class fields
        print(f"  BasePlayer field start: 0x{bp_field_start:X} (header=0x10 + BCE={bce_size}=0x{bce_size:X})")
        print(f"  currentTeam should be near 0x{current_bp['CurrentTeam']:X}")

    return 0

if __name__ == "__main__":
    sys.exit(main())
