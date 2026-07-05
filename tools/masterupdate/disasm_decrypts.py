#!/usr/bin/env python3
"""
disasm_decrypts.py — Offline capstone decrypt disassembler.

Reads sha-dumper output for decrypt function RVAs, reads GameAssembly.dll
from disk, disassembles with capstone, and outputs accurate decrypt algorithms.

100% Morphine-independent. Does NOT need game running (reads binary from disk).
Requires: sha-dumper output (for cipher function RVAs) + GameAssembly.dll on disk.

Usage:
    python disasm_decrypts.py [--gameassembly PATH] [--sha-output PATH] [--output PATH]

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

# Known decrypt function patterns (build 24037537, from validict verification).
# Used to identify unknown ciphers by matching operation sequences.
# Format: canonical_name → [(op, value), ...]
KNOWN_PATTERNS = {
    "base_networkable_0": [("rol", 0x16), ("sub", 0x512FB7E6), ("xor", 0x3C25B628), ("add", 0x606330A1)],
    "base_networkable_1": [("rol", 0x12), ("xor", 0xE54E9BFF), ("rol", 0x8), ("xor", 0xCECB4770)],
    "cl_active_item":     [("sub", 0x1D2981D5), ("rol", 0x2), ("xor", 0x8DA4E5D3), ("add", 0x6189597E)],
    "decrypt_fov":        [("xor", 0x8041A4D4), ("add", 0x2270CDAC), ("rol", 0x1D), ("sub", 0x3BA7A498)],
    "player_inventory":   [("rol", 0x8), ("add", 0x18E53C82), ("rol", 0x1)],
    "player_eyes":        [("sub", 0x6FB58358), ("xor", 0x6DC93C8F), ("rol", 0x15), ("add", 0x4E3D6061)],
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


# Known constants for binary search (from FALLBACK_DECRYPT_OPS, build 24037537)
# Used to find decrypt functions that sha-dumper missed or mis-tagged.
# Format: canonical_name → [constant_value, ...] (any of these can be searched)
SEED_CONSTANTS = {
    "player_inventory":  [0x18E53C82, 0x0F898622],  # Fallback ADD + structural match ADD
    "cl_active_item":    [0x1D2981D5, 0x8DA4E5D3, 0x6189597E],  # Validict confirmed — CLA_TRYALL proved match
    "decrypt_fov":       [0x8041A4D4, 0x2270CDAC, 0x3BA7A498],  # Validict-confirmed (independent function, NOT derived from cl_active_item)
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
        print(f"[FATAL] sha-dumper output not found: {sha_output}")
        print("        Run getnewoffsets.bat first to generate sha-dumper output")
        return 1

    print(f"[disasm_decrypts] Offline capstone decrypt disassembler")
    print(f"  GameAssembly.dll: {dll_path}")
    print(f"  sha-dumper output: {sha_output}")
    print()

    # Step 1: Parse all cipher sections from sha-dumper output
    all_ciphers = parse_sha_dumper_ciphers(sha_output)
    print(f"[1] Parsed {len(all_ciphers)} cipher sections from sha-dumper output")

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

    output = {
        "status": "SUCCESS" if len(identified) >= 4 else "PARTIAL",
        "identified": len(identified),
        "total": len(KNOWN_PATTERNS),
        "missing": [name for name in KNOWN_PATTERNS if name not in identified],
        "algorithms": results,
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
        merge_into_master(identified)

    print(f"\n{'='*60}")
    print(f"  Identified {len(identified)}/{len(KNOWN_PATTERNS)} decrypt algorithms")
    for r in results:
        ops_str = " ".join(f"{o['op'].upper()} {o['value']}" for o in r["ops"])
        print(f"    {r['name']:25s} {r['rva']:12s} {ops_str}")
    print(f"  Output: {output_file}")
    if missing:
        print(f"  Missing: {missing} (will use fallback)")
    print(f"{'='*60}")
    return 0 if len(identified) >= 4 else 2


def merge_into_master(identified):
    """Merge capstone-extracted decrypt algorithms into master.json.

    Overwrites/augments master.json["decrypt_functions"] with capstone results.
    Capstone results have higher confidence than sha-dumper HDE64 results.
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
        entry = {
            "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops_raw],
            "ops_raw": ops_raw,
            "read_offset": info["fn_rva"],
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

    with open(MASTER_JSON, "w", encoding="utf-8") as f:
        json.dump(master, f, indent=2)

    print(f"[5] Merged {merged_count} capstone decrypt algorithms into master.json")


if __name__ == "__main__":
    sys.exit(main())
