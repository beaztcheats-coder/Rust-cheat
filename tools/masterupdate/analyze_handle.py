#!/usr/bin/env python3
"""Disassemble gchandle_get_target and generate a draft Il2cppGetHandle implementation."""
import json
import os
import re
from pathlib import Path

CONFIG_PATH = Path(__file__).parent / "config.json"


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def load_master():
    path = Path(__file__).parent / "output" / "master.json"
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def disassemble_with_capstone(data: bytes, base_addr: int):
    try:
        from capstone import Cs, CS_ARCH_X86, CS_MODE_64
    except ImportError:
        return None

    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True
    return list(md.disasm(data, base_addr))


def extract_immediates(insn_list) -> list:
    """Extract immediate values from disassembly."""
    imm = []
    for insn in insn_list:
        for op in insn.operands:
            if op.type == 2:  # immediate
                imm.append((insn.address, op.imm))
    return imm


def find_xor_keys(insn_list) -> list:
    """Find XOR instructions with immediate operands."""
    keys = []
    for insn in insn_list:
        if insn.mnemonic == "xor":
            for op in insn.operands:
                if op.type == 2:
                    keys.append((insn.address, op.imm))
    return keys


def find_shift_amounts(insn_list) -> list:
    """Find shift instructions and their amounts."""
    shifts = []
    for insn in insn_list:
        if insn.mnemonic in ("shl", "shr", "sar"):
            for op in insn.operands:
                if op.type == 2:
                    shifts.append((insn.address, insn.mnemonic, op.imm))
                elif op.type == 1:  # register shift count (cl)
                    shifts.append((insn.address, insn.mnemonic, "cl"))
    return shifts


def generate_draft(handle_rva: int, gc_array_rva: int, xor_keys: list, shifts: list) -> str:
    lines = [
        "// ============================================================",
        "// DRAFT Il2cppGetHandle implementation (REVIEW MANUALLY)",
        f"// Generated from gchandle_get_target @ 0x{handle_rva:08X}",
        f"// GC handle array RVA: 0x{gc_array_rva:08X}",
        "// ============================================================",
        "",
        "    inline uint64_t Il2cppGetHandle(uint64_t handle)",
        "    {",
        "        if (!handle) return 0;",
        "",
        f"        uint64_t table = read<uint64_t>(GameAssembly + 0x{gc_array_rva:08X});",
        "        // TODO: verify table indexing logic against disassembly",
    ]

    if shifts:
        for addr, mnemonic, amount in shifts[:3]:
            if amount != "cl":
                lines.append(f"        // 0x{addr:X}: {mnemonic} {amount}")

    if xor_keys:
        for addr, key in xor_keys[:3]:
            lines.append(f"        // 0x{addr:X}: xor 0x{key:08X}")
        lines.append(f"        // entry ^= 0x{xor_keys[0][1]:08X}; // VERIFY")

    lines.extend([
        "",
        "        uint64_t entry = read<uint64_t>(table + (handle >> 1) * 8); // EXAMPLE",
        "        return entry;",
        "    }",
        "",
        "// ============================================================",
        "// WARNING: This is a draft. You must verify the shift/XOR/table",
        "// indexing by reading the actual disassembly in output/handle_disasm.txt",
        "// ============================================================",
    ])
    return "\n".join(lines)


def dump_raw_bytes(dll_path: Path, rva: int, size: int = 512) -> bytes:
    try:
        import pefile
        pe = pefile.PE(str(dll_path))
        file_offset = pe.get_offset_from_rva(rva)
        with open(dll_path, "rb") as f:
            f.seek(file_offset)
            return f.read(size)
    except Exception as e:
        print(f"[!] Could not read GameAssembly.dll: {e}")
        return b""


def follow_jmp(dll_path: Path, instructions: list, image_base: int) -> tuple:
    """If the first instruction is an unconditional jmp, disassemble the target RVA too."""
    if not instructions:
        return instructions, None
    first = instructions[0]
    if first.mnemonic == "jmp":
        target_str = first.op_str
        try:
            target_addr = int(target_str, 16)
            target_rva = target_addr - image_base
            print(f"[INFO] Following jmp to 0x{target_addr:X} (RVA 0x{target_rva:08X})")
            raw2 = dump_raw_bytes(dll_path, target_rva, size=2048)
            if raw2:
                instructions2 = disassemble_with_capstone(raw2, target_addr)
                return instructions2, target_rva
        except ValueError:
            pass
    return instructions, None


def main():
    cfg = load_config()
    master = load_master()
    out_dir = Path(__file__).parent / "output"
    out_dir.mkdir(parents=True, exist_ok=True)

    handle_rva = master.get("il2cpp_api", {}).get("gchandle_get_target")
    gc_array_rva = master.get("gc_handles", {}).get("array_rva") or master.get("offsets", {}).get("il2cpphandle")

    if not handle_rva:
        print("[!] gchandle_get_target RVA not found in master.json")
        return

    print(f"[INFO] gchandle_get_target RVA: 0x{handle_rva:08X}")
    print(f"[INFO] GC handle array RVA: 0x{gc_array_rva:08X}" if gc_array_rva else "[INFO] GC handle array RVA not found")

    dll_path = Path(cfg.get("gameassembly_path", ""))
    if not dll_path.exists():
        print(f"[!] GameAssembly.dll not found at {dll_path}")
        print("    Set gameassembly_path in config.json to your Rust GameAssembly.dll")
        return

    # Read raw bytes
    raw = dump_raw_bytes(dll_path, handle_rva)
    if not raw:
        return

    # Try capstone disassembly
    image_base = 0x180000000  # Typical Windows DLL base, not critical for relative analysis
    instructions = disassemble_with_capstone(raw, image_base + handle_rva)

    # Follow unconditional jmp at entry point (common for exported Il2Cpp functions)
    instructions, resolved_rva = follow_jmp(dll_path, instructions, image_base)
    if resolved_rva is not None:
        handle_rva = resolved_rva
        raw = dump_raw_bytes(dll_path, handle_rva, size=2048)

    disasm_lines = []
    if instructions:
        for insn in instructions:
            disasm_lines.append(f"0x{insn.address:X}: {insn.mnemonic:8} {insn.op_str}")
        xor_keys = find_xor_keys(instructions)
        shifts = find_shift_amounts(instructions)
        immediates = extract_immediates(instructions)
    else:
        disasm_lines.append("Capstone not installed. Raw bytes only.")
        disasm_lines.append("Install with: pip install capstone")
        xor_keys = []
        shifts = []
        immediates = []

    # Save disassembly
    disasm_text = "\n".join(disasm_lines)
    (out_dir / "handle_disasm.txt").write_text(disasm_text, encoding="utf-8")
    print(f"[OK] Saved disassembly to output/handle_disasm.txt")

    # Save raw bytes as hex
    hex_lines = []
    for i in range(0, len(raw), 16):
        chunk = raw[i:i+16]
        hex_str = " ".join(f"{b:02X}" for b in chunk)
        ascii_str = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        hex_lines.append(f"{i:04X}: {hex_str:<48} {ascii_str}")
    (out_dir / "handle_bytes.txt").write_text("\n".join(hex_lines), encoding="utf-8")

    # Generate draft
    draft = generate_draft(handle_rva, gc_array_rva or 0, xor_keys, shifts)
    (out_dir / "handle_draft.cpp").write_text(draft, encoding="utf-8")
    print(f"[OK] Saved draft implementation to output/handle_draft.cpp")

    # Build structured JSON dump for AI analysis
    handle_dump = {
        "morphine_build": master.get("build", "unknown"),
        "source_url": cfg.get("morphine_offsets_url", "unknown"),
        "gameassembly_path": str(dll_path),
        "gchandle_get_target": {
            "rva": f"0x{handle_rva:08X}",
            "rva_int": handle_rva,
        },
        "gc_handle_array": {
            "rva": f"0x{gc_array_rva:08X}" if gc_array_rva else None,
            "rva_int": gc_array_rva,
        },
        "raw_bytes": {
            "length": len(raw),
            "hex": raw.hex(),
            "byte_array": list(raw),
        },
        "disassembly": disasm_lines,
        "analysis": {
            "xor_keys": [{"address": f"0x{addr:X}", "value": f"0x{imm:08X}", "value_int": imm} for addr, imm in xor_keys],
            "shifts": [{"address": f"0x{addr:X}", "operation": op, "amount": amount} for addr, op, amount in shifts],
            "immediates": [{"address": f"0x{addr:X}", "value": f"0x{imm:08X}", "value_int": imm} for addr, imm in immediates],
        },
        "notes": [
            "This is the Unity 6 Il2CPP il2cpp_gchandle_get_target function.",
            "Derive the handle-to-object decoding logic from the disassembly.",
            "Common patterns: shr rcx, 1 / xor rax, imm / table_base + index * 8.",
        ],
    }

    (out_dir / "handle_dump.json").write_text(json.dumps(handle_dump, indent=2), encoding="utf-8")
    print(f"[OK] Saved structured dump to output/handle_dump.json")

    # Generate ready-to-paste AI prompt
    ask_opencode_template = generate_ai_prompt_template(handle_dump)
    (out_dir / "ask_opencode.txt").write_text(ask_opencode_template, encoding="utf-8")
    print(f"[OK] Saved AI prompt template to output/ask_opencode.txt")

    # Summary
    summary_lines = [
        f"gchandle_get_target RVA: 0x{handle_rva:08X}",
        f"GC handle array RVA: 0x{gc_array_rva:08X}" if gc_array_rva else "GC handle array RVA: not found",
        f"XOR keys found: {len(xor_keys)}",
        f"Shift operations found: {len(shifts)}",
        f"Immediate values found: {len(immediates)}",
        "",
        "Output files:",
        "  - output/handle_disasm.txt",
        "  - output/handle_bytes.txt",
        "  - output/handle_draft.cpp",
        "  - output/handle_dump.json",
        "  - output/ask_opencode.txt",
        "",
        "Next steps:",
        "1. Open output/handle_disasm.txt and output/handle_dump.json",
        "2. Read output/ask_opencode.txt",
        "3. Paste the prompt + JSON into OpenCode or another AI",
        "4. Review the generated Il2cppGetHandle carefully before using",
    ]
    (out_dir / "handle_summary.txt").write_text("\n".join(summary_lines), encoding="utf-8")
    print(f"[OK] Saved summary to output/handle_summary.txt")


def generate_ai_prompt_template(dump: dict) -> str:
    """Generate a ready-to-paste prompt for OpenCode/AI analysis."""
    lines = [
        "=== COPY EVERYTHING BELOW THIS LINE INTO YOUR AI CHAT ===",
        "",
        "I am updating an external Rust cheat for Unity 6 Il2CPP. I have extracted the raw bytes and disassembly of il2cpp_gchandle_get_target.",
        "",
        "Based on the JSON dump below, please:",
        "",
        "1. Analyze the disassembly of gchandle_get_target and derive the exact handle-to-object decoding algorithm.",
        "2. Write a complete C++ implementation of Il2cppGetHandle(uint64_t handle) that uses my existing driver read<T>(address) template.",
        "3. Do NOT generate a complete offsets.hpp or sdk.hpp — only the Il2cppGetHandle function.",
        "4. Explain each step of your derivation so I can verify it.",
        "",
        "Context:",
        f"- GameAssembly.dll path: {dump.get('gameassembly_path', 'unknown')}",
        f"- gchandle_get_target RVA: {dump['gchandle_get_target']['rva']}",
        f"- GC handle array RVA: {dump['gc_handle_array']['rva']}",
        "- The function reads from game memory through a kernel driver using read<T>(address).",
        "- The handle value may be tagged (e.g. LSB=1) or encoded with shifts/XOR.",
        "",
        "Here is the structured dump:",
        "",
        "```json",
        json.dumps(dump, indent=2),
        "```",
        "",
        "Please provide only the Il2cppGetHandle C++ function, ready to paste into sdk.hpp.",
        "",
        "=== END OF PROMPT ===",
    ]
    return "\n".join(lines)


if __name__ == "__main__":
    main()
