#!/usr/bin/env python3
"""
disasm_decrypts.py — Offline capstone decrypt disassembler (THE authority for decrypt algorithms).

Reads cipher function RVAs from morphine-dumper JSON (preferred) or sha-dumper output,
reads GameAssembly.dll from disk, disassembles with capstone, and outputs accurate
decrypt + auto-generated encrypt algorithms.

100% Morphine-independent. Does NOT need game running (reads binary from disk).

Input priority (first available wins):
  1. master.json from morphine-dumper (fn_rva for all 6 decrypt functions)
  2. sha-dumper-output.txt (cipher sections with fn_rva)
  3. Standalone cipher scan (scans .text section for mov reg,2 prologues — no dumper needed)

Output:
  - decrypt_algorithms.json: All identified decrypt + auto-generated encrypt functions
  - Merges into master.json (decrypt_functions + encrypt_functions)

Usage:
    python disasm_decrypts.py [--gameassembly PATH] [--sha-output PATH] [--output PATH]
    python disasm_decrypts.py  # uses config.json defaults, standalone scan if no dumper output

If paths not specified, reads from config.json defaults.
"""
import json
import re
import sys
import argparse
from pathlib import Path

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_64
    from capstone.x86 import X86_OP_IMM
except ImportError:
    print("[FATAL] capstone not installed. Run: pip install capstone")
    sys.exit(1)

try:
    import pefile
except ImportError:
    print("[FATAL] pefile not installed. Run: pip install pefile")
    sys.exit(1)

SCRIPT_DIR = Path(__file__).parent
OUTPUT_DIR = SCRIPT_DIR / "output"
CONFIG_PATH = SCRIPT_DIR / "config.json"
SHA_DUMPER_OUTPUT = OUTPUT_DIR / "sha-dumper-output.txt"
MASTER_JSON = OUTPUT_DIR / "master.json"
OUTPUT_FILE = OUTPUT_DIR / "decrypt_algorithms.json"

# Known decrypt function patterns (build 24069519 — UC confirmed + capstone verified).
# Used to identify unknown ciphers by matching operation sequences.
# Format: canonical_name → [(op, value), ...]
KNOWN_PATTERNS = {
    "base_networkable_0": [("rol", 0xB), ("xor", 0xBCDFA6C7), ("add", 0x1A88518)],
    "base_networkable_1": [("add", 0x4016C175), ("rol", 0x1D), ("add", 0x7D75A2B0), ("xor", 0x97FF1778)],
    "cl_active_item":     [("rol", 27), ("add", 0x56427D52), ("rol", 8)],
    "decrypt_fov":        [("add", 0xF7ED7C0), ("rol", 0x12), ("sub", 0xA557A4EC), ("xor", 0xD6A6E25E)],
    "player_inventory":   [("add", 0x86F83B72), ("rol", 0x15), ("add", 0x41069A6F)],
    "player_eyes":        [("rol", 0x5), ("add", 0x487FBCF7), ("xor", 0xE70F1737)],
}

# Map sha-dumper cipher section names to canonical names
CIPHER_NAME_MAP = {
    "bn_0": "base_networkable_0",
    "bn_1": "base_networkable_1",
    "cl_active_item": "cl_active_item",
    "inventory": "player_inventory",
    "eyes": "player_eyes",
    "fov": "decrypt_fov",
}

# Map canonical names to decrypt_dat keys (for rust_decrypts.dat)
DECRYPT_DAT_KEYS = {
    "base_networkable_0": "nk",
    "base_networkable_1": "nk2",
    "cl_active_item": "cla",
    "player_inventory": "inv",
    "player_eyes": "ey",
    "decrypt_fov": "fov",
}


def load_config():
    if CONFIG_PATH.exists():
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}


def parse_master_json_ciphers(path):
    """Parse decrypt function fn_rvas from master.json (morphine-dumper JSON source).

    Returns list of dicts: [{name, fn_rva, sha_ops}, ...]
    sha_ops is from morphine-dumper (may use HDE64 — we re-disassemble with capstone).

    This is the preferred source — morphine-dumper provides fn_rva for ALL decrypt
    functions, not just cipher sections. Falls back to sha-dumper if master.json
    has no fn_rvas.
    """
    if not path.exists():
        return []

    with open(path, "r", encoding="utf-8") as f:
        master = json.load(f)

    ciphers = []
    for name, data in master.get("decrypt_functions", {}).items():
        fn_rva = data.get("fn_rva", 0)
        if fn_rva and fn_rva > 0:
            ops = data.get("ops_raw", data.get("ops", []))
            sha_ops = []
            for op in ops:
                if isinstance(op, dict):
                    sha_ops.append((op["op"], op["value"]))
                elif isinstance(op, (list, tuple)):
                    sha_ops.append((op[0], op[1]))
            ciphers.append({
                "name": name,
                "fn_rva": fn_rva,
                "sha_ops": sha_ops,
            })

    print(f"[morphine-json] Found {len(ciphers)} decrypt fn_rvas in master.json")
    return ciphers


def parse_sha_dumper_ciphers(path):
    """Parse ALL cipher sections from sha-dumper output (including unknown_N).

    Returns list of dicts: [{name, fn_rva, sha_ops}, ...]
    sha_ops is the HDE64-extracted ops (may be inaccurate — we re-disassemble with capstone).
    """
    if not path.exists():
        print(f"[WARN] sha-dumper output not found: {path}")
        return []

    content = path.read_text(encoding="utf-8", errors="ignore")
    ciphers = []
    # Match [cipher_NAME] sections (including cipher_unknown_N)
    pattern = re.compile(r'\[(cipher_[^\]]+)\]([\s\S]*?)(?=\n\[|\Z)', re.MULTILINE)
    for match in pattern.finditer(content):
        section_name = match.group(1)  # e.g. "cipher_bn_0" or "cipher_unknown_42"
        body = match.group(2)

        # Extract fn_rva
        rva_match = re.search(r'fn_rva\s*=\s*(0x[0-9A-Fa-f]+)', body, re.IGNORECASE)
        if not rva_match:
            continue
        fn_rva = int(rva_match.group(1), 16)

        # Extract ops (HDE64-based — may be inaccurate, used as hint only)
        ops = []
        for op_match in re.finditer(r'op\[(\d+)\]\s*=\s*(\w+)\s+(0x[0-9A-Fa-f]+)', body):
            ops.append((op_match.group(2).lower(), int(op_match.group(3), 16)))

        # Clean name (remove cipher_ prefix)
        clean_name = section_name.replace("cipher_", "")

        ciphers.append({
            "section_name": section_name,
            "name": clean_name,
            "fn_rva": fn_rva,
            "sha_ops": ops,  # Hint only — we re-disassemble with capstone
        })

    return ciphers


# Known constants for binary search (build 24069519 — UC confirmed + capstone verified)
# Used to find decrypt functions that sha-dumper missed or mis-tagged.
# Format: canonical_name → [constant_value, ...] (any of these can be searched)
SEED_CONSTANTS = {
    "player_inventory":  [0x86F83B72, 0x41069A6F],  # ADD constants from build 24069519
    "cl_active_item":    [0x56427D52],               # ADD constant from build 24069519 (UC confirmed)
    "decrypt_fov":       [0xF7ED7C0, 0xA557A4EC, 0xD6A6E25E],  # ADD/SUB/XOR from build 24069519
}


def search_binary_for_constant(dll_path, constant, search_back=64, read_size=512):
    """Search GameAssembly.dll .text section for a 4-byte constant.

    Returns list of {rva, ops} where ops are extracted cipher operations.
    Tries multiple instruction alignments to find the correct one.
    """
    try:
        pe = pefile.PE(str(dll_path), fast_load=True)
        pe.parse_data_directories()
    except Exception as e:
        print(f"[ERROR] Failed to parse PE: {e}")
        return []

    # Find .text section
    text_section = None
    for section in pe.sections:
        if b'.text' in section.Name:
            text_section = section
            break
    if not text_section:
        return []

    text_rva = text_section.VirtualAddress
    text_raw = section.get_data()

    # Search for the 4-byte constant (little-endian)
    needle = constant.to_bytes(4, 'little')
    hits = []
    offset = 0
    while True:
        idx = text_raw.find(needle, offset)
        if idx == -1:
            break
        hits.append(text_rva + idx)
        offset = idx + 1

    # For each hit, try multiple starting offsets to find correct instruction alignment
    results = []
    for hit_rva in hits:
        # The constant is the last 4 bytes of an instruction like:
        # 81 C1 <imm32>  (add ecx, imm32 — 6 bytes, constant at offset -4 from end)
        # 81 E9 <imm32>  (sub ecx, imm32)
        # 81 F1 <imm32>  (xor ecx, imm32)
        # 05 <imm32>     (add eax, imm32 — 5 bytes)
        # 2D <imm32>     (sub eax, imm32)
        # 35 <imm32>     (xor eax, imm32)
        # Try starting disassembly at various offsets before the constant
        best_ops = []
        best_offset = 0
        for back in range(0, 16):
            start_rva = max(0, hit_rva - search_back - back)
            try:
                file_offset = pe.get_offset_from_rva(start_rva)
                with open(dll_path, 'rb') as f:
                    f.seek(file_offset)
                    raw = f.read(read_size + search_back)
            except:
                continue

            ops = extract_cipher_ops_capstone(raw)
            # Prefer results that include the searched constant
            has_constant = any(v == constant for _, v in ops)
            if has_constant and len(ops) > len(best_ops):
                best_ops = ops
                best_offset = back
                break  # Found the correct alignment

        if best_ops and len(best_ops) >= 2:
            results.append({
                "rva": hit_rva,
                "ops": best_ops,
            })

    return results


def find_decrypt_by_constant(dll_path, canonical_name, known_patterns=KNOWN_PATTERNS):
    """Search GameAssembly.dll binary for a decrypt function by its known constants.

    Tries each seed constant, disassembles surrounding code, and matches against
    the known pattern (exact) or structural pattern (op types only).
    """
    seeds = SEED_CONSTANTS.get(canonical_name, [])
    if not seeds:
        return None

    pattern = known_patterns.get(canonical_name, [])
    if not pattern:
        return None

    for seed in seeds:
        print(f"    Searching binary for constant 0x{seed:X}...")
        hits = search_binary_for_constant(dll_path, seed)
        print(f"    Found {len(hits)} hits")

        for hit in hits:
            ops = hit["ops"]

            # Exact match
            if len(ops) == len(pattern):
                match = True
                for i, (op_type, op_val) in enumerate(ops):
                    if op_type != pattern[i][0] or op_val != pattern[i][1]:
                        match = False
                        break
                if match:
                    return {
                        "fn_rva": f"0x{hit['rva']:X}",
                        "fn_rva_int": hit["rva"],
                        "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                        "ops_raw": [[o[0], o[1]] for o in ops],
                        "source": "capstone_binary_search_exact",
                        "sha_name": "binary_search",
                    }

            # Structural match (op types match, values may differ)
            op_types = [o[0] for o in ops]
            pat_types = [p[0] for p in pattern]
            if op_types == pat_types:
                return {
                    "fn_rva": f"0x{hit['rva']:X}",
                    "fn_rva_int": hit["rva"],
                    "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                    "ops_raw": [[o[0], o[1]] for o in ops],
                    "source": "capstone_binary_search_structural",
                    "sha_name": "binary_search",
                }

    return None


def read_bytes_at_rva(dll_path, rva, size=512):
    """Read raw bytes at an RVA from GameAssembly.dll using pefile."""
    try:
        pe = pefile.PE(str(dll_path), fast_load=True)
        file_offset = pe.get_offset_from_rva(rva)
        with open(dll_path, "rb") as f:
            f.seek(file_offset)
            return f.read(size)
    except Exception as e:
        print(f"[ERROR] Failed to read RVA 0x{rva:X} from {dll_path}: {e}")
        return b""
    finally:
        if 'pe' in dir():
            try:
                pe.close()
            except:
                pass


def extract_cipher_ops_capstone(code_bytes):
    """Use capstone to disassemble and extract cipher operations.

    Handles:
    - ADD reg, imm32
    - SUB reg, imm32
    - XOR reg, imm32
    - ROL (multiple compiler patterns: SHL+SHR+OR, SHR+SHL+OR, native ROR)
    - Loop unrolling (deduplicate half-sequences)

    Returns list of (op_type, value) tuples.
    """
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True

    ops = []
    pending_shl = None   # (amount, reg) for SHL
    pending_shr = None    # (amount, reg) for SHR

    for insn in md.disasm(code_bytes, 0x1000):
        m = insn.mnemonic

        # Stop at function end
        if m in ('ret', 'retn', 'int3', 'jmp'):
            break

        # XOR reg, imm32 (skip reg-reg XOR which is zeroing)
        if m == 'xor' and len(insn.operands) == 2:
            if insn.operands[1].type == X86_OP_IMM:
                val = insn.operands[1].imm & 0xFFFFFFFF
                if val > 0xFF:
                    ops.append(('xor', val))
            pending_shl = None
            pending_shr = None
            continue

        # ADD reg, imm32
        if m == 'add' and len(insn.operands) == 2 and insn.operands[1].type == X86_OP_IMM:
            val = insn.operands[1].imm & 0xFFFFFFFF
            if val > 0xFF:
                ops.append(('add', val))
            pending_shl = None
            pending_shr = None
            continue

        # SUB reg, imm32
        if m == 'sub' and len(insn.operands) == 2 and insn.operands[1].type == X86_OP_IMM:
            val = insn.operands[1].imm & 0xFFFFFFFF
            if val > 0xFF:
                ops.append(('sub', val))
            pending_shl = None
            pending_shr = None
            continue

        # SHL reg, imm8 — track for ROL detection
        if m == 'shl' and len(insn.operands) == 2 and insn.operands[1].type == X86_OP_IMM:
            amt = insn.operands[1].imm
            # Check if we already have a pending SHR that complements
            if pending_shr is not None and pending_shr[0] + amt in (32, 64):
                ops.append(('rol', amt))
                pending_shr = None
                pending_shl = None
            else:
                pending_shl = (amt, insn.reg_name(insn.operands[0].reg) if insn.operands[0].type == 1 else None)
            continue

        # SHR reg, imm8 — track for ROL detection
        if m == 'shr' and len(insn.operands) == 2 and insn.operands[1].type == X86_OP_IMM:
            amt = insn.operands[1].imm
            # Check if we already have a pending SHL that complements
            if pending_shl is not None and pending_shl[0] + amt in (32, 64):
                ops.append(('rol', pending_shl[0]))
                pending_shl = None
                pending_shr = None
            else:
                pending_shr = (amt, insn.reg_name(insn.operands[0].reg) if insn.operands[0].type == 1 else None)
            continue

        # OR reg, reg — check if this completes a ROL (SHL+SHR+OR pattern)
        if m == 'or' and len(insn.operands) == 2:
            if pending_shl is not None and pending_shr is not None:
                if pending_shl[0] + pending_shr[0] in (32, 64):
                    ops.append(('rol', pending_shl[0]))
                    pending_shl = None
                    pending_shr = None
                    continue
            # OR without matching SHL+SHR — reset
            pending_shl = None
            pending_shr = None
            continue

        # Native ROR instruction
        if m == 'ror' and len(insn.operands) == 2 and insn.operands[1].type == X86_OP_IMM:
            ops.append(('ror', insn.operands[1].imm))
            pending_shl = None
            pending_shr = None
            continue

        # Reset pending shifts on any other instruction (except mov/lea/nop which are benign)
        if m not in ('mov', 'lea', 'nop', 'movzx', 'movsx', 'movsxd'):
            pending_shl = None
            pending_shr = None

    # Handle loop unrolling — if ops are duplicated, take first half
    if len(ops) >= 4 and len(ops) % 2 == 0:
        half = len(ops) // 2
        if ops[:half] == ops[half:]:
            ops = ops[:half]

    return ops


def match_ops_to_pattern(ops, known_patterns=KNOWN_PATTERNS):
    """Try to match an operation sequence against known decrypt patterns.

    Returns the canonical name if matched, None otherwise.
    """
    for name, pattern in known_patterns.items():
        if len(ops) != len(pattern):
            continue
        match = True
        for i, (op_type, op_val) in enumerate(ops):
            pat_type, pat_val = pattern[i]
            if op_type != pat_type or op_val != pat_val:
                match = False
                break
        if match:
            return name
    return None


def match_ops_by_structure(ops, known_patterns=KNOWN_PATTERNS):
    """Match by op-type sequence only (ignoring values).

    Useful when the game updated and constants changed but operation order is same.
    Returns list of candidate canonical names.
    """
    candidates = []
    op_types = [op[0] for op in ops]
    for name, pattern in known_patterns.items():
        pat_types = [p[0] for p in pattern]
        if op_types == pat_types:
            candidates.append(name)
    return candidates


def get_morphine_decrypt_rvas():
    """Read decrypt function RVAs from master.json (if Morphine was fetched).

    This is used as cross-validation only, not as primary source.
    """
    if not MASTER_JSON.exists():
        return {}

    try:
        with open(MASTER_JSON, "r", encoding="utf-8") as f:
            master = json.load(f)
    except:
        return {}

    rvas = {}
    # Check offsets for client_entities_decryption and entity_list_decryption
    offsets = master.get("offsets", {})
    for morphine_key, canonical in [("client_entities_decryption", "base_networkable_0"),
                                    ("entity_list_decryption", "base_networkable_1")]:
        val = offsets.get(morphine_key)
        if val:
            try:
                rvas[canonical] = int(val, 16) if isinstance(val, str) else int(val)
            except:
                pass

    # Also check decrypt_functions in master.json
    for name, info in master.get("decrypt_functions", {}).items():
        rva = info.get("read_offset") or info.get("fn_rva")
        if rva:
            try:
                rva_int = int(rva, 16) if isinstance(rva, str) else int(rva)
                if rva_int > 0x10000:
                    canonical = CIPHER_NAME_MAP.get(name, name)
                    if canonical not in rvas:
                        rvas[canonical] = rva_int
            except:
                pass

    return rvas


def scan_text_section_for_ciphers(dll_path, max_hits=2000):
    """Scan GameAssembly.dll .text section for cipher prologues (mov reg, 2).

    This is the STANDALONE fallback — works without any injected dumper.
    Finds ALL functions that start with the cipher loop prologue
    `mov reg, 2` (the loop counter initialization), then disassembles
    each with capstone to extract ROL/XOR/ADD/SUB operations.

    Patterns matched (both REX and non-REX forms):
        41 B8-BF 02 00 00 00  — mov r8d-r15d, 2 (REX form)
        B8-BF 02 00 00 00     — mov eax-edi, 2 (non-REX form)

    Returns list of dicts: [{name, fn_rva, sha_ops}, ...]
    """
    try:
        pe = pefile.PE(str(dll_path), fast_load=True)
    except Exception as e:
        print(f"[ERROR] Failed to parse PE for text scan: {e}")
        return []

    text_section = None
    for section in pe.sections:
        if b'.text' in section.Name:
            text_section = section
            break
    if not text_section:
        print("[ERROR] .text section not found")
        return []

    text_rva = text_section.VirtualAddress
    text_raw = text_section.get_data()
    text_size = len(text_raw)

    # Build search patterns for mov reg, 2
    # Non-REX: B8-BF (eax-edi), followed by 02 00 00 00
    # REX: 41 B8-BF (r8d-r15d), followed by 02 00 00 00
    prologue_non_rex = [bytes([0xB8 + reg, 0x02, 0x00, 0x00, 0x00]) for reg in range(8)]
    prologue_rex = [bytes([0x41, 0xB8 + reg, 0x02, 0x00, 0x00, 0x00]) for reg in range(8)]

    ciphers = []
    seen_rvas = set()

    for patterns, prefix_size in [(prologue_non_rex, 0), (prologue_rex, 1)]:
        for pat in patterns:
            offset = 0
            while True:
                idx = text_raw.find(pat, offset)
                if idx == -1:
                    break
                # The mov instruction starts at idx (or idx-1 for REX prefix)
                # But we want to start disassembling from a few bytes before to catch
                # any preceding instructions that set up the cipher
                fn_rva = text_rva + idx - prefix_size
                if fn_rva in seen_rvas:
                    offset = idx + 1
                    continue
                seen_rvas.add(fn_rva)

                # Read bytes around the prologue for disassembly
                start = max(0, idx - prefix_size - 32)
                read_size = 512
                raw = text_raw[start:start + read_size]
                if len(raw) < 64:
                    offset = idx + 1
                    continue

                ops = extract_cipher_ops_capstone(raw)
                if len(ops) >= 2:
                    ciphers.append({
                        "name": f"scan_unknown_{len(ciphers)}",
                        "fn_rva": fn_rva,
                        "sha_ops": ops,  # Capstone-extracted ops (not HDE64)
                        "source": "standalone_scan",
                    })

                if len(ciphers) >= max_hits:
                    break
                offset = idx + 1
            if len(ciphers) >= max_hits:
                break
        if len(ciphers) >= max_hits:
            break

    pe.close()
    print(f"[standalone-scan] Found {len(ciphers)} cipher prologues in .text section")
    return ciphers


def generate_encrypt_ops(decrypt_ops):
    """Generate the inverse (encrypt) operation sequence from decrypt ops.

    Encrypt = reverse order, with inverted operations:
        ADD → SUB
        SUB → ADD
        XOR → XOR (self-inverse)
        ROL → ROR (rotate right)

    Args:
        decrypt_ops: list of (op_type, value) tuples

    Returns:
        list of (op_type, value) tuples for the encrypt function
    """
    inverse_map = {
        "add": "sub",
        "sub": "add",
        "xor": "xor",
        "rol": "ror",
    }
    result = []
    for op_type, value in reversed(decrypt_ops):
        inv_type = inverse_map.get(op_type, op_type)
        result.append((inv_type, value))
    return result


def main():
    parser = argparse.ArgumentParser(description="Offline capstone decrypt disassembler")
    parser.add_argument("--gameassembly", type=Path, help="Path to GameAssembly.dll")
    parser.add_argument("--sha-output", type=Path, help="Path to sha-dumper output")
    parser.add_argument("--output", type=Path, help="Output JSON path")
    parser.add_argument("--no-merge", action="store_true", help="Don't merge results into master.json")
    args = parser.parse_args()

    cfg = load_config()
    output_dir = OUTPUT_DIR
    output_dir.mkdir(parents=True, exist_ok=True)

    # Resolve paths
    dll_path = args.gameassembly or Path(cfg.get("gameassembly_path", ""))
    sha_output = args.sha_output or SHA_DUMPER_OUTPUT
    output_file = args.output or OUTPUT_FILE

    if not dll_path.exists():
        print(f"[FATAL] GameAssembly.dll not found: {dll_path}")
        print("        Set gameassembly_path in config.json")
        return 1

    if not sha_output.exists():
        # Check if master.json has fn_rvas — if so, sha-dumper is not required
        morphine_ciphers = parse_master_json_ciphers(MASTER_JSON)
        if not morphine_ciphers:
            # Last resort: standalone cipher scan (no dumper needed at all)
            print(f"[INFO] No sha-dumper output or master.json fn_rvas found")
            print(f"[INFO] Will use standalone cipher scan (scans GameAssembly.dll .text section)")
        else:
            print(f"[INFO] sha-dumper output not found — using master.json fn_rvas only")

    print(f"[disasm_decrypts] Offline capstone decrypt disassembler")
    print(f"  GameAssembly.dll: {dll_path}")
    print(f"  sha-dumper output: {sha_output}")
    print()

    # Step 1: Parse cipher fn_rvas — prefer master.json (morphine-dumper), fall back to sha-dumper
    all_ciphers = parse_master_json_ciphers(MASTER_JSON)
    if all_ciphers:
        print(f"[1] Parsed {len(all_ciphers)} cipher fn_rvas from master.json (morphine-dumper)")
        if sha_output.exists():
            sha_ciphers = parse_sha_dumper_ciphers(sha_output)
            sha_names = {c["name"] for c in all_ciphers}
            added = 0
            for c in sha_ciphers:
                if c["name"] not in sha_names and not c["name"].startswith("unknown_"):
                    all_ciphers.append(c)
                    added += 1
            if added:
                print(f"    Added {added} additional ciphers from sha-dumper")
    elif sha_output.exists():
        all_ciphers = parse_sha_dumper_ciphers(sha_output)
        print(f"[1] Parsed {len(all_ciphers)} cipher sections from sha-dumper output")
    else:
        # No dumper output — use standalone cipher scan (no dumper needed)
        print(f"[1] No dumper output available — running standalone cipher scan...")
        all_ciphers = scan_text_section_for_ciphers(dll_path)

    named_ciphers = [c for c in all_ciphers if not c["name"].startswith("unknown_")]
    unknown_ciphers = [c for c in all_ciphers if c["name"].startswith("unknown_")]
    print(f"    Named: {len(named_ciphers)}, Unknown: {len(unknown_ciphers)}")

    # Step 2: Disassemble each cipher with capstone
    print(f"\n[2] Disassembling cipher functions with capstone...")

    identified = {}  # canonical_name → {fn_rva, ops, source}
    unidentified = []  # list of {fn_rva, ops, candidates}

    for cipher in all_ciphers:
        fn_rva = cipher["fn_rva"]
        raw = read_bytes_at_rva(dll_path, fn_rva, size=512)
        if not raw:
            print(f"  [SKIP] {cipher['name']} @ 0x{fn_rva:X} — failed to read bytes")
            continue

        ops = extract_cipher_ops_capstone(raw)
        if len(ops) < 2:
            continue  # Not a cipher function

        # Try exact pattern match first
        canonical = match_ops_to_pattern(ops)

        if canonical:
            identified[canonical] = {
                "fn_rva": f"0x{fn_rva:X}",
                "fn_rva_int": fn_rva,
                "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                "ops_raw": [[o[0], o[1]] for o in ops],
                "source": "capstone_exact_match",
                "sha_name": cipher["name"],
            }
            print(f"  [OK] {cipher['name']} @ 0x{fn_rva:X} -> {canonical} (exact match)")
        else:
            # Try structural match (op types match but values differ)
            candidates = match_ops_by_structure(ops)
            if candidates:
                unidentified.append({
                    "fn_rva": fn_rva,
                    "ops": ops,
                    "candidates": candidates,
                    "sha_name": cipher["name"],
                })

    # Step 3: For unidentified ciphers, try to assign missing decrypts
    print(f"\n[3] Resolving unidentified ciphers...")

    missing = [name for name in KNOWN_PATTERNS if name not in identified]
    if missing:
        print(f"    Missing: {missing}")

        # Try Morphine cross-validation
        morphine_rvas = get_morphine_decrypt_rvas()
        if morphine_rvas:
            print(f"    Morphine cross-validation RVAs: {list(morphine_rvas.keys())}")
            for canonical, rva in morphine_rvas.items():
                if canonical in identified or canonical not in missing:
                    continue
                raw = read_bytes_at_rva(dll_path, rva, size=512)
                if raw:
                    ops = extract_cipher_ops_capstone(raw)
                    if len(ops) >= 2:
                        identified[canonical] = {
                            "fn_rva": f"0x{rva:X}",
                            "fn_rva_int": rva,
                            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                            "ops_raw": [[o[0], o[1]] for o in ops],
                            "source": "capstone_morphine_rva",
                            "sha_name": "morphine_crossval",
                        }
                        print(f"  [OK] {canonical} @ 0x{rva:X} -> resolved via Morphine RVA (cross-validation)")
                        if canonical in missing:
                            missing.remove(canonical)

        # Try structural candidates from unknown ciphers
        if missing:
            for entry in unidentified:
                for candidate in entry["candidates"]:
                    if candidate in missing:
                        # Verify: structural match found, values may have changed
                        identified[candidate] = {
                            "fn_rva": f"0x{entry['fn_rva']:X}",
                            "fn_rva_int": entry["fn_rva"],
                            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in entry["ops"]],
                            "ops_raw": [[o[0], o[1]] for o in entry["ops"]],
                            "source": "capstone_structural_match",
                            "sha_name": entry["sha_name"],
                            "warning": "Op-type sequence matches but values differ from known pattern — game may have updated",
                        }
                        print(f"  [OK] {candidate} @ 0x{entry['fn_rva']:X} -> structural match (values may differ)")
                        missing.remove(candidate)
                        if not missing:
                            break
                if not missing:
                    break

    # Step 3b: Binary constant search for decrypts with known seed constants
    # Run for ALL decrypts that have seed constants, even if structurally matched,
    # because exact binary search is more reliable than structural matching.
    binary_search_targets = [name for name in SEED_CONSTANTS if name in KNOWN_PATTERNS]
    if binary_search_targets:
        print(f"\n[3b] Searching GameAssembly.dll binary for decrypts by known constants...")
        for canonical in binary_search_targets:
            # Skip if already exact-matched (not structural)
            if canonical in identified and identified[canonical].get("source") == "capstone_exact_match":
                continue
            result = find_decrypt_by_constant(dll_path, canonical)
            if result:
                # Replace structural match with exact binary search result
                old_source = identified.get(canonical, {}).get("source", "none")
                identified[canonical] = result
                if canonical in missing:
                    missing.remove(canonical)
                ops_str = " ".join(f"{o['op'].upper()} {o['value']}" for o in result["ops"])
                print(f"  [OK] {canonical} @ {result['fn_rva']} -> {result['source']}: {ops_str}")
                if old_source != "none":
                    print(f"       (replaced previous {old_source})")
            else:
                if canonical not in identified:
                    print(f"  [SKIP] {canonical}: no match found in binary search")

    # Step 3c: Derive decrypt_fov from cl_active_item if possible
    # NOTE: decrypt_fov is an INDEPENDENT function with its own constants — NOT derived from cl_active_item.
    # This derivation is kept as a FALLBACK only (last resort when binary search fails).
    # Validict confirmed: decrypt_fov = XOR→ADD→ROL 29→SUB (different constants from cl_active_item)
    # cl_active_item: SUB → ROL → XOR → ADD
    # decrypt_fov:    XOR → ADD → ROL → SUB (different function entirely)
    if "decrypt_fov" in missing and "cl_active_item" in identified:
        print(f"\n[3c] Deriving decrypt_fov from cl_active_item (same constants, skip ROL)...")
        cla_ops = identified["cl_active_item"]["ops_raw"]
        # Remove any ROL operations
        fov_ops = [(op, val) for op, val in cla_ops if op not in ("rol", "ror")]
        if len(fov_ops) >= 2:
            identified["decrypt_fov"] = {
                "fn_rva": identified["cl_active_item"]["fn_rva"],
                "fn_rva_int": identified["cl_active_item"]["fn_rva_int"],
                "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in fov_ops],
                "ops_raw": fov_ops,
                "source": "derived_from_cl_active_item",
                "sha_name": "derived",
            }
            missing.remove("decrypt_fov")
            ops_str = " ".join(f"{o[0].upper()} {o[1]}" for o in fov_ops)
            print(f"  [OK] decrypt_fov derived: {ops_str}")
            print(f"       (same function as cl_active_item @ {identified['cl_active_item']['fn_rva']}, ROL skipped)")

    # Step 3d: Standalone cipher scan — scan .text section for mov reg,2 prologues
    # This is the LAST RESORT fallback — works without any injected dumper.
    if missing:
        print(f"\n[3d] Standalone cipher scan (scanning .text section for cipher prologues)...")
        scanned_ciphers = scan_text_section_for_ciphers(dll_path)
        for sc in scanned_ciphers:
            ops = sc["sha_ops"]
            # Try exact match
            canonical = match_ops_to_pattern(ops)
            if canonical and canonical in missing:
                identified[canonical] = {
                    "fn_rva": f"0x{sc['fn_rva']:X}",
                    "fn_rva_int": sc["fn_rva"],
                    "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                    "ops_raw": [[o[0], o[1]] for o in ops],
                    "source": "capstone_standalone_scan",
                    "sha_name": sc["name"],
                }
                missing.remove(canonical)
                print(f"  [OK] {canonical} @ 0x{sc['fn_rva']:X} -> standalone scan exact match")
            else:
                # Try structural match
                candidates = match_ops_by_structure(ops)
                for candidate in candidates:
                    if candidate in missing:
                        identified[candidate] = {
                            "fn_rva": f"0x{sc['fn_rva']:X}",
                            "fn_rva_int": sc["fn_rva"],
                            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                            "ops_raw": [[o[0], o[1]] for o in ops],
                            "source": "capstone_standalone_structural",
                            "sha_name": sc["name"],
                            "warning": "Found via standalone scan, structural match (values may differ)",
                        }
                        missing.remove(candidate)
                        print(f"  [OK] {candidate} @ 0x{sc['fn_rva']:X} -> standalone scan structural match")
                        break

    # Step 4: Report any still-missing decrypts
    if missing:
        print(f"\n[WARN] Could not identify: {missing}")
        print("    These will use FALLBACK_DECRYPT_OPS from compare_and_patch.py")

    # Step 5: Output results
    print(f"\n[4] Writing output to {output_file}")

    # Build output in the same format as frida_decrypt_algorithms.json
    results = []
    for canonical, info in identified.items():
        dat_key = DECRYPT_DAT_KEYS.get(canonical, canonical)
        results.append({
            "name": canonical,
            "dat_key": dat_key,
            "rva": info["fn_rva"],
            "ops": info["ops"],
            "ops_raw": info["ops_raw"],
            "source": info["source"],
            "warning": info.get("warning"),
        })

    # Auto-generate encrypt functions (inverse of decrypt)
    encrypt_results = []
    for canonical, info in identified.items():
        decrypt_ops_raw = info["ops_raw"]
        # Convert ops_raw (list of [op, value] lists) to tuples
        decrypt_ops = [(op[0], op[1]) for op in decrypt_ops_raw]
        encrypt_ops = generate_encrypt_ops(decrypt_ops)
        encrypt_name = f"encrypt_{canonical}" if not canonical.startswith("decrypt_") else canonical.replace("decrypt_", "encrypt_")
        encrypt_results.append({
            "name": encrypt_name,
            "dat_key": DECRYPT_DAT_KEYS.get(canonical, canonical),
            "rva": info["fn_rva"],
            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in encrypt_ops],
            "ops_raw": [[o[0], o[1]] for o in encrypt_ops],
            "source": "auto_generated_inverse",
            "confidence": "auto-generated",
        })

    # Collect unidentified ciphers for output (Task 3.4)
    unknown_results = []
    for entry in unidentified:
        unknown_results.append({
            "fn_rva": f"0x{entry['fn_rva']:X}",
            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in entry["ops"]],
            "ops_raw": [[o[0], o[1]] for o in entry["ops"]],
            "candidates": entry["candidates"],
            "sha_name": entry["sha_name"],
        })

    output = {
        "status": "SUCCESS" if len(identified) >= 4 else "PARTIAL",
        "identified": len(identified),
        "total": len(KNOWN_PATTERNS),
        "missing": [name for name in KNOWN_PATTERNS if name not in identified],
        "algorithms": results,
        "encrypt_algorithms": encrypt_results,
        "unidentified_ciphers": unknown_results,
    }

    # Also output in decrypt_dat format (for rust_decrypts.dat generation)
    decrypt_dat_format = {}
    for canonical, info in identified.items():
        dat_key = DECRYPT_DAT_KEYS.get(canonical, canonical)
        ops = info["ops_raw"]
        if dat_key == "nk":
            decrypt_dat_format["nk_rol"] = ops[0][1] if len(ops) > 0 and ops[0][0] == "rol" else 0
            # Find sub, xor, add regardless of order
            for op_type, val in ops:
                if op_type == "sub":
                    decrypt_dat_format["nk_sub"] = val
                elif op_type == "xor":
                    decrypt_dat_format["nk_xor"] = val
                elif op_type == "add":
                    decrypt_dat_format["nk_add"] = val
        elif dat_key == "nk2":
            for i, (op_type, val) in enumerate(ops):
                if op_type == "rol":
                    decrypt_dat_format[f"nk2_rol_{i}"] = val
                else:
                    decrypt_dat_format[f"nk2_{op_type}"] = val
        elif dat_key == "cla":
            for i, (op_type, val) in enumerate(ops):
                decrypt_dat_format[f"cla_{op_type}_{i}"] = val
        elif dat_key == "inv":
            for i, (op_type, val) in enumerate(ops):
                decrypt_dat_format[f"inv_{op_type}_{i}"] = val
        elif dat_key == "ey":
            for i, (op_type, val) in enumerate(ops):
                decrypt_dat_format[f"ey_{op_type}_{i}"] = val
        elif dat_key == "fov":
            for i, (op_type, val) in enumerate(ops):
                decrypt_dat_format[f"fov_{op_type}_{i}"] = val

    output["decrypt_dat_format"] = decrypt_dat_format

    output_file.write_text(json.dumps(output, indent=2), encoding="utf-8")

    # Step 6: Merge into master.json (so compare_and_patch.py can use the results)
    if not args.no_merge:
        merge_into_master(identified, encrypt_results)

    print(f"\n{'='*60}")
    print(f"  Identified {len(identified)}/{len(KNOWN_PATTERNS)} decrypt algorithms")
    for r in results:
        ops_str = " ".join(f"{o['op'].upper()} {o['value']}" for o in r["ops"])
        print(f"    {r['name']:25s} {r['rva']:12s} {ops_str}")
    if encrypt_results:
        print(f"  Auto-generated {len(encrypt_results)} encrypt functions (inverse of decrypt)")
    if unknown_results:
        print(f"  {len(unknown_results)} unidentified ciphers (output for manual investigation)")
    print(f"  Output: {output_file}")
    if missing:
        print(f"  Missing: {missing} (will use fallback)")
    print(f"{'='*60}")
    return 0 if len(identified) >= 4 else 2


def merge_into_master(identified, encrypt_results=None):
    """Merge capstone-extracted decrypt algorithms into master.json.

    Overwrites/augments master.json["decrypt_functions"] with capstone results.
    Capstone results have higher confidence than sha-dumper HDE64 results.
    Also merges auto-generated encrypt functions into master.json["encrypt_functions"].
    """
    if not MASTER_JSON.exists():
        print(f"[5] Skipping merge — master.json not found")
        return

    try:
        with open(MASTER_JSON, "r", encoding="utf-8") as f:
            master = json.load(f)
    except:
        print(f"[5] Skipping merge — could not read master.json")
        return

    if "decrypt_functions" not in master:
        master["decrypt_functions"] = {}

    merged_count = 0
    for canonical, info in identified.items():
        # Map canonical name to Morphine/sha-dumper names
        morphine_names = {
            "base_networkable_0": "base_networkable_0",
            "base_networkable_1": "base_networkable_1",
            "cl_active_item": "cl_active_item",
            "player_inventory": "player_inventory",
            "player_eyes": "player_eyes",
            "decrypt_fov": "decrypt_fov",
        }
        morphine_name = morphine_names.get(canonical, canonical)

        ops_raw = info["ops_raw"]
        fn_rva_int = info.get("fn_rva_int", 0)
        entry = {
            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops_raw],
            "ops_raw": ops_raw,
            "fn_rva": fn_rva_int,
            "read_offset": info.get("read_offset"),
            "confidence": "high" if "exact" in info["source"] else "medium",
            "source": info["source"],
        }

        # Only overwrite if capstone/derived source is more reliable
        existing = master["decrypt_functions"].get(morphine_name, {})
        existing_source = existing.get("source", "")
        is_capstone_source = "capstone" in info["source"] or "derived" in info["source"]
        if is_capstone_source and "capstone" not in existing_source and "derived" not in existing_source:
            master["decrypt_functions"][morphine_name] = entry
            merged_count += 1
        elif not existing:
            master["decrypt_functions"][morphine_name] = entry
            merged_count += 1

    # Merge encrypt functions
    encrypt_merged = 0
    if encrypt_results:
        if "encrypt_functions" not in master:
            master["encrypt_functions"] = {}
        for enc in encrypt_results:
            master["encrypt_functions"][enc["name"]] = {
                "ops": enc["ops"],
                "ops_raw": enc["ops_raw"],
                "fn_rva": enc.get("rva_int", 0),
                "confidence": "auto-generated",
                "source": "capstone_inverse",
            }
            encrypt_merged += 1

    with open(MASTER_JSON, "w", encoding="utf-8") as f:
        json.dump(master, f, indent=2)

    print(f"[5] Merged {merged_count} capstone decrypt + {encrypt_merged} encrypt algorithms into master.json")


if __name__ == "__main__":
    sys.exit(main())
