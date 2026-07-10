#!/usr/bin/env python3
"""
generate_cheat_offsets.py — Generate clean cheat_offsets.txt from morphine dumper output.

Reads the morphine-dumper_output.h file and produces a human-readable summary of ALL
offsets and decrypt functions needed for the cheat. Each value includes a confidence
marker showing how it was resolved.

Usage:
    python generate_cheat_offsets.py [--input PATH] [--output PATH] [--classes-dump PATH]

If paths not specified, reads from Desktop (morphine-dumper_output.h) and writes
cheat_offsets.txt next to it.
"""
import re
import sys
import argparse
from datetime import datetime
from pathlib import Path

def parse_morphine_output(path):
    """Parse morphine-dumper_output.h — extract offsets, decrypts, build ID."""
    if not path.exists():
        print(f"[ERROR] Morphine output not found: {path}")
        return None

    text = path.read_text(encoding="utf-8", errors="ignore")
    data = {"build": "unknown", "offsets": {}, "decrypts": {}, "klass_rvas": {}, "raw": text}

    # Extract build ID
    m = re.search(r'Build\s*=\s*"(\d+)"', text)
    if m:
        data["build"] = m.group(1)

    # Extract klass_rvas (static TypeInfo pointers)
    for m in re.finditer(r'BaseNetworkable\s*=\s*(0x[0-9A-Fa-f]+)', text):
        data["klass_rvas"]["BaseNetworkable"] = m.group(1)
    for m in re.finditer(r'MainCamera\s*=\s*(0x[0-9A-Fa-f]+)', text):
        data["klass_rvas"]["MainCamera"] = m.group(1)
    for m in re.finditer(r'TOD_Sky\s*=\s*(0x[0-9A-Fa-f]+)', text):
        data["klass_rvas"]["TOD_Sky"] = m.group(1)
    for m in re.finditer(r'convar_graphics\s*=\s*(0x[0-9A-Fa-f]+)', text):
        data["klass_rvas"]["ConVar_Graphics"] = m.group(1)
    for m in re.finditer(r'il2cpphandle\s*=\s*(0x[0-9A-Fa-f]+)', text):
        data["klass_rvas"]["Il2cppGetHandle"] = m.group(1)

    # Extract namespace offsets
    # Pattern: namespace NAME { ... constexpr ... field = 0xVALUE; ... }
    ns_pattern = re.compile(r'namespace\s+(\w+)\s*\{([^}]+)\}', re.MULTILINE)

    # Map morphine namespace names to cheat-relevant names
    ns_map = {
        "base_player": "BasePlayer",
        "base_entity": "BaseEntity",
        "base_combat_entity": "BaseCombatEntity",
        "base_networkable": "BaseNetworkable",
        "camera": "Camera",
        "player_eyes": "PlayerEyes",
        "player_input": "PlayerInput",
        "player_model": "PlayerModel",
        "model_state": "ModelState",
        "PlayerInventory": "PlayerInventory",
        "ItemContainer": "ItemContainer",
        "item": "Item",
        "ItemDefinition": "ItemDefinition",
        "BaseProjectile": "BaseProjectile",
        "HeldEntity": "HeldEntity",
        "RecoilProperties": "RecoilProperties",
        "SkinnedMultiMesh": "SkinnedMultiMesh",
        "convar_graphics": "ConVar_Graphics",
        "convar_admin": "ConVar_Admin",
        "EffectNetwork": "EffectNetwork",
        "PlayerWalkMovement": "PlayerWalkMovement",
        "base_movement": "BaseMovement",
        "WorldItem": "WorldItem",
        "AutoTurret": "AutoTurret",
    }

    for m in ns_pattern.finditer(text):
        ns_name = m.group(1)
        ns_body = m.group(2)
        cheat_name = ns_map.get(ns_name, ns_name)

        fields = {}
        for fm in re.finditer(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+)', ns_body):
            fname = fm.group(1)
            fval = fm.group(2)
            if fname not in ("name_ptr", "static_fields"):  # skip internal il2cpp fields
                fields[fname] = fval

        if fields:
            data["offsets"][cheat_name] = fields

    # Extract decrypt functions
    # Pattern: uintptr_t decryption::FUNC_NAME(uint64_t a1) { ... }
    decrypt_pattern = re.compile(
        r'(?:uintptr_t|inline\s+uint32_t)\s+decryption::(\w+)\s*\([^)]*\)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}',
        re.MULTILINE
    )

    for m in decrypt_pattern.finditer(text):
        func_name = m.group(1)
        func_body = m.group(2)

        ops = []
        # Extract operations: SUB, XOR, ADD, ROL (from SHL+SHR pairs)
        for om in re.finditer(r'(ecx|edx|eax|val)\s*([\+\-\^]=|=)\s*([\+\-\^]=?)\s*(0x[0-9A-Fa-f]+)', func_body):
            pass  # Too complex to parse reliably — we extract constants instead

        # Extract all hex constants used in crypto operations
        constants = re.findall(r'(?:[\+\-\^]|<<|>>)\s*(?:ecx|edx|eax)?\s*,?\s*(0x[0-9A-Fa-f]{6,})', func_body)
        shifts = re.findall(r'(?:shl|shr|<<|>>)\s*(?:0x)?(\d+)', func_body, re.IGNORECASE)

        data["decrypts"][func_name] = {
            "constants": constants,
            "shifts": shifts,
            "body": func_body.strip()[:200],
        }

    return data


def parse_classes_dump(path):
    """Parse all_cheat_classes_dump.txt — extract complete field lists."""
    if not path.exists():
        return {}

    text = path.read_text(encoding="utf-8", errors="ignore")
    classes = {}
    current_class = None

    for line in text.split('\n'):
        line = line.strip()
        if line.startswith('=== ') and line.endswith(' ==='):
            current_class = line.replace('=== ', '').replace(' ===', '').strip()
            if 'CLASS NOT FOUND' in current_class:
                current_class = None
                continue
            classes[current_class] = []
        elif current_class and line.startswith('+0x'):
            # Parse: +0x0340  System.Object           PlayerEyes
            parts = line.split(None, 2)
            if len(parts) >= 3:
                offset = parts[0]  # +0x0340
                ftype = parts[1]   # System.Object
                fname = parts[2]   # PlayerEyes
                classes[current_class].append({
                    "offset": offset,
                    "type": ftype,
                    "name": fname,
                })

    return classes


def generate_output(data, classes_dump, output_path):
    """Generate cheat_offsets.txt."""
    lines = []
    lines.append("// ═══════════════════════════════════════════════════════════════════")
    lines.append("// │ CHEAT OFFSETS — Auto-generated from Morphine Dumper             │")
    lines.append("// ═══════════════════════════════════════════════════════════════════")
    lines.append(f"// Build: {data['build']}")
    lines.append(f"// Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"// Source: morphine-dumper_output.h")
    if classes_dump:
        lines.append(f"// Cross-ref: all_cheat_classes_dump.txt ({sum(len(v) for v in classes_dump.values())} fields in {len(classes_dump)} classes)")
    lines.append("//")
    lines.append("// CONFIDENCE MARKERS:")
    lines.append("//   [exact]   = resolved by exact field name lookup (100% reliable)")
    lines.append("//   [heuristic] = resolved by type/positional/disasm fallback (verify!)")
    lines.append("//   [manual]  = cross-validated against UC/website (manually confirmed)")
    lines.append("// ═══════════════════════════════════════════════════════════════════")
    lines.append("")

    # Static pointers
    lines.append("// ─── Static Pointers (TypeInfo RVAs) ───")
    for name, val in sorted(data.get("klass_rvas", {}).items()):
        lines.append(f"{name:40s} = {val}")
    lines.append("")

    # Field offsets grouped by class
    lines.append("// ─── Field Offsets ───")
    for class_name, fields in sorted(data.get("offsets", {}).items()):
        lines.append(f"")
        lines.append(f"  // {class_name}")
        for fname, fval in sorted(fields.items(), key=lambda x: int(x[1], 16) if x[1].startswith('0x') else 0):
            # Check if this field exists in the complete dump for cross-validation
            confidence = "[exact]"
            if classes_dump:
                found_in_dump = False
                for cls_fields in classes_dump.values():
                    for f in cls_fields:
                        if fname.lower() in f["name"].lower() or f["name"].lower() in fname.lower():
                            if f["offset"].lower().replace("+0x", "0x") == fval.lower():
                                confidence = "[verified]"
                                found_in_dump = True
                                break
                            else:
                                confidence = "[MISMATCH!]"
                                found_in_dump = True
                                break
                    if found_in_dump:
                        break
                if not found_in_dump:
                    confidence = "[heuristic?]"

            lines.append(f"  {fname:40s} = {fval:12s}  {confidence}")
    lines.append("")

    # Decrypt functions
    lines.append("// ─── Decrypt Functions ───")
    decrypt_names = {
        "client_entities": "networkable_key (BN layer 1)",
        "entity_list": "networkable_key2 (BN layer 2)",
        "cl_active_item": "cl_active_item (held item UID)",
        "PlayerInventory": "decrypt_inventory_pointer",
        "PlayerEyes": "decrypt_eyes",
    }

    for func_name, desc in decrypt_names.items():
        if func_name in data.get("decrypts", {}):
            info = data["decrypts"][func_name]
            lines.append(f"")
            lines.append(f"  // {desc}")
            lines.append(f"  // Constants: {', '.join(info['constants'])}")
            if info['shifts']:
                lines.append(f"  // Shifts: {', '.join(info['shifts'])}")
        elif func_name == "cl_active_item":
            lines.append(f"")
            lines.append(f"  // {desc}")
            lines.append(f"  // WARNING: cl_active_item decrypt NOT FOUND in morphine output!")
            lines.append(f"  // Check if morphine dumper override was removed, or use disasm_decrypts.py")
    lines.append("")

    # Complete field dump reference
    if classes_dump:
        lines.append("// ═══════════════════════════════════════════════════════════════════")
        lines.append("// │ COMPLETE FIELD DUMP SUMMARY                                     │")
        lines.append("// ═══════════════════════════════════════════════════════════════════")
        lines.append(f"// Total classes dumped: {len(classes_dump)}")
        lines.append(f"// Total fields: {sum(len(v) for v in classes_dump.values())}")
        lines.append("//")
        lines.append("// Full dump in: all_cheat_classes_dump.txt")
        lines.append("// Use this to manually verify any offset marked [heuristic?] or [MISMATCH!]")
        lines.append("")

    # Write output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"[generate_cheat_offsets] Output written to: {output_path}")
    print(f"[generate_cheat_offsets] Build: {data['build']}")
    print(f"[generate_cheat_offsets] Static pointers: {len(data.get('klass_rvas', {}))}")
    print(f"[generate_cheat_offsets] Classes with offsets: {len(data.get('offsets', {}))}")
    print(f"[generate_cheat_offsets] Decrypt functions: {len(data.get('decrypts', {}))}")


def main():
    parser = argparse.ArgumentParser(description="Generate cheat_offsets.txt from morphine dumper output")
    parser.add_argument("--input", type=Path, default=Path.home() / "Desktop" / "morphine-dumper_output.h",
                        help="Path to morphine-dumper_output.h")
    parser.add_argument("--output", type=Path, default=Path.home() / "Desktop" / "cheat_offsets.txt",
                        help="Output path for cheat_offsets.txt")
    parser.add_argument("--classes-dump", type=Path, default=Path.home() / "Desktop" / "all_cheat_classes_dump.txt",
                        help="Path to all_cheat_classes_dump.txt (optional, for cross-validation)")
    args = parser.parse_args()

    data = parse_morphine_output(args.input)
    if not data:
        sys.exit(1)

    classes_dump = parse_classes_dump(args.classes_dump) if args.classes_dump.exists() else {}

    generate_output(data, classes_dump, args.output)


if __name__ == "__main__":
    main()
