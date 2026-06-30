#!/usr/bin/env python3
"""Extract key offsets from Il2CppInspectorPro JSON metadata."""
import json
import sys
from pathlib import Path

JSON_PATH = Path(__file__).parent / "output" / "il2cppinspector" / "metadata.json"

# Current offsets.hpp values for comparison
CURRENT_PTRS = {
    "basenetworkable_pointer": 0xE334210,
    "camera_pointer": 0xE37ACA0,
    "TOD_Sky_TypeInfo": 0xE3593D0,
    "Il2cppGetHandle": 0x0E7A9F20,
}

# Classes to search for field offsets
TARGET_CLASSES = [
    "BasePlayer", "BaseCombatEntity", "BaseNetworkable", "BaseEntity",
    "BaseCamera", "PlayerEyes", "PlayerInput", "PlayerModel",
    "PlayerInventory", "BaseProjectile", "RecoilProperties",
    "BaseMovement", "Item", "ItemDefinition", "ItemContainer",
    "Model", "TOD_Sky", "SkinnedMultiMesh", "HeldEntity",
    "MainCamera", "ConVar_Graphics", "EffectNetwork",
    "PlayerWalkMovement", "ViewModel", "WorldItem",
]

def main():
    print(f"[INFO] Loading {JSON_PATH} ({JSON_PATH.stat().st_size / 1024 / 1024:.0f} MB)...")
    with open(JSON_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    am = data.get("addressMap", {})
    print(f"[INFO] addressMap keys: {list(am.keys())}")

    # === 1. TypeInfo Pointers ===
    tip = am.get("typeInfoPointers", [])
    print(f"\n=== TYPEINFO POINTERS ({len(tip)} entries) ===")

    # Check format
    if tip:
        print(f"Format: {json.dumps(tip[0], indent=2)[:200]}")

    # Search for our target classes
    found_ptrs = {}
    for entry in tip:
        if isinstance(entry, dict):
            addr = entry.get("virtualAddress", entry.get("address", ""))
            name = entry.get("name", entry.get("typeName", ""))
        elif isinstance(entry, list) and len(entry) >= 2:
            addr, name = entry[0], entry[1]
        else:
            continue

        for target in CURRENT_PTRS:
            cheat_name = target
            if target == "basenetworkable_pointer" and "BaseNetworkable" in str(name) and "_" not in str(name).replace("BaseNetworkable_", ""):
                found_ptrs[cheat_name] = (addr, name)
            elif target == "camera_pointer" and str(name) == "MainCamera":
                found_ptrs[cheat_name] = (addr, name)
            elif target == "TOD_Sky_TypeInfo" and str(name) == "TOD_Sky":
                found_ptrs[cheat_name] = (addr, name)

    # Also search by name substring
    for entry in tip:
        if isinstance(entry, dict):
            addr = entry.get("virtualAddress", entry.get("address", ""))
            name = str(entry.get("name", entry.get("typeName", "")))
        elif isinstance(entry, list) and len(entry) >= 2:
            addr, name = str(entry[0]), str(entry[1])
        else:
            continue

        if "BaseNetworkable" in name and "BaseNetworkable_" not in name and "basenetworkable_pointer" not in found_ptrs:
            found_ptrs["basenetworkable_pointer"] = (addr, name)
        if name == "MainCamera" and "camera_pointer" not in found_ptrs:
            found_ptrs["camera_pointer"] = (addr, name)
        if name == "TOD_Sky" and "TOD_Sky_TypeInfo" not in found_ptrs:
            found_ptrs["TOD_Sky_TypeInfo"] = (addr, name)
        if "Il2cppGetHandle" in name or ("gchandle" in name.lower() and "array" in name.lower()):
            found_ptrs["Il2cppGetHandle"] = (addr, name)

    print("\n--- TypeInfo Pointer Comparison ---")
    for target, current_val in CURRENT_PTRS.items():
        if target in found_ptrs:
            addr, name = found_ptrs[target]
            # Parse hex address
            try:
                il2cpp_val = int(addr, 16) if isinstance(addr, str) else int(addr)
            except (ValueError, TypeError):
                il2cpp_val = 0
            match = "MATCH" if il2cpp_val == current_val else "DIFFERS"
            print(f"  {target}: current=0x{current_val:X}  il2cpp={addr} ({name})  [{match}]")
        else:
            print(f"  {target}: current=0x{current_val:X}  il2cpp=NOT FOUND")

    # === 2. Type Metadata (struct definitions with field offsets) ===
    tm = am.get("typeMetadata", [])
    print(f"\n=== TYPE METADATA ({len(tm) if isinstance(tm, list) else 'N/A'} entries) ===")
    if isinstance(tm, list) and tm:
        print(f"First entry: {json.dumps(tm[0], indent=2)[:400]}")

    # === 3. Method Definitions (for Il2cppGetHandle RVA) ===
    md = am.get("methodDefinitions", [])
    print(f"\n=== METHOD DEFINITIONS ({len(md)} entries) ===")
    if md:
        print(f"Format: {json.dumps(md[0], indent=2)[:300]}")

    # Search for Il2cppGetHandle
    for entry in md:
        if isinstance(entry, dict):
            name = str(entry.get("name", ""))
            addr = entry.get("methodCodeAddress", entry.get("virtualAddress", ""))
            if "GetHandle" in name or "gchandle" in name.lower():
                print(f"  Found: {name} at {addr}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
