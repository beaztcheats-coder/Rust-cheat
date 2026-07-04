#!/usr/bin/env python3
"""
sig_scanner.py — Signature-based static pointer scanner for GameAssembly.dll

Finds static pointer RVAs by scanning for RIP-relative LEA/MOV instructions
in executable sections. No game injection needed — reads GameAssembly.dll
from disk.

Two modes:
  --learn    : Discovers signatures from currently known RVAs (in offsets.hpp)
  --apply    : Uses saved signatures to find new RVAs after a game update
  (default)  : Learn if no signatures saved, otherwise apply + cross-validate

Output:
  output/sig_patterns.json     — Saved byte signatures (wildcarded displacement)
  output/sig_scan_results.json  — Found RVAs + confidence

Technique: scan for REX.W + LEA/MOV opcodes with RIP-relative ModRM, extract
the 4-byte displacement, calculate target = instr_rva + 7 + disp32.
Much faster than full disassembly — just byte matching.
"""
import json
import struct
import sys
import time
from pathlib import Path

import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_MEM, X86_REG_RIP

SCRIPT_DIR = Path(__file__).parent
OUTPUT_DIR = SCRIPT_DIR / "output"
CFG_PATH = SCRIPT_DIR / "config.json"
SIG_FILE = OUTPUT_DIR / "sig_patterns.json"
RESULTS_FILE = OUTPUT_DIR / "sig_scan_results.json"

# Static pointers to find — name → current RVA (from offsets.hpp)
STATIC_POINTERS = {
    "basenetworkable_pointer": 0x0FD36298,
    "camera_pointer": 0x0FD0A5C0,
    "Il2cppGetHandle": 0x10266600,
    "TOD_Sky_TypeInfo": 0x0FCEBC70,
    "EffectNetwork_Pointer": 0xE3790F8,
    "Class_SingletonComponent_UI_LoadingScreen": 0x0FD11958,
    "Class_SingletonComponent_MixerSnapshotManager__c": 0x0FD37CF0,
}

# Context bytes for signature uniqueness
CONTEXT_BEFORE = 6
CONTEXT_AFTER = 4

# RIP-relative ModRM byte: mod=00, rm=101 → byte & 0xC7 == 0x05
RIP_REL_MODRM_MASK = 0xC7
RIP_REL_MODRM_VAL = 0x05

# REX prefixes for 64-bit LEA/MOV
REX_W_PREFIXES = [0x48, 0x4C]  # REX.W and REX.WR
LEA_OPCODE = 0x8D
MOV_OPCODE = 0x8B


def load_config():
    with open(CFG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def get_exec_sections(pe, include_il2cpp=False):
    """Get executable sections. il2cpp is huge (200MB+) — only scan if needed."""
    sections = []
    for section in pe.sections:
        name = section.Name.rstrip(b'\x00').decode('ascii', errors='ignore')
        if section.Characteristics & 0x20000000:  # IMAGE_SCN_MEM_EXECUTE
            data = section.get_data()
            rva = section.VirtualAddress
            if name == 'il2cpp' and not include_il2cpp:
                print(f"  [PE] {name}: RVA=0x{rva:X}, size={len(data):,} bytes (SKIPPED — enable with il2cpp fallback)")
                continue
            sections.append((data, rva, name))
            print(f"  [PE] {name}: RVA=0x{rva:X}, size={len(data):,} bytes")
    return sections


import re

# Pre-compiled regex patterns for RIP-relative LEA/MOV instructions
# Matches: REX.W/WR + LEA/MOV + ModRM(RIP-relative)
# ModRM for RIP-relative: mod=00, rm=101 → values 05,0D,15,1D,25,2D,35,3D
_RIP_MODRM = b'[\x05\x0D\x15\x1D\x25\x2D\x35\x3D]'
RIP_PATTERNS = [
    re.compile(b'\x48\x8D' + _RIP_MODRM),  # REX.W + LEA r64, [RIP+disp]
    re.compile(b'\x4C\x8D' + _RIP_MODRM),  # REX.WR + LEA r8-r15, [RIP+disp]
    re.compile(b'\x48\x8B' + _RIP_MODRM),  # REX.W + MOV r64, [RIP+disp]
    re.compile(b'\x4C\x8B' + _RIP_MODRM),  # REX.WR + MOV r8-r15, [RIP+disp]
]


def find_rip_relative_refs(text_bytes, text_rva, target_rvas):
    """Fast scan for RIP-relative LEA/MOV instructions targeting our RVAs.

    Uses regex finditer (C-level) for fast pattern matching, then checks
    the 4-byte displacement at each match.
    """
    target_set = set(target_rvas)
    results = {t: [] for t in target_rvas}
    count = 0
    text_len = len(text_bytes)

    for pat_idx, pattern in enumerate(RIP_PATTERNS):
        rex = 0x48 if pat_idx < 2 else 0x4C
        opcode = 0x8D if pat_idx % 2 == 0 else 0x8B
        mnemonic = "lea" if opcode == 0x8D else "mov"

        for m in pattern.finditer(text_bytes):
            idx = m.start()
            if idx + 7 > text_len:
                continue

            modrm = text_bytes[idx + 2]
            disp = struct.unpack_from('<i', text_bytes, idx + 3)[0]
            instr_rva = text_rva + idx
            target = instr_rva + 7 + disp

            if target in target_set:
                reg_num = (modrm >> 3) & 0x7
                if rex == 0x4C:
                    reg_num += 8
                reg_names = ["rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                             "r8","r9","r10","r11","r12","r13","r14","r15"]
                reg = reg_names[reg_num]

                results[target].append({
                    "instr_rva": instr_rva,
                    "instr_size": 7,
                    "mnemonic": mnemonic,
                    "op_str": f"{reg}, [rip + 0x{disp & 0xFFFFFFFF:X}]" if disp >= 0 else f"{reg}, [rip - 0x{-disp:X}]",
                    "bytes": list(text_bytes[idx:idx+7]),
                    "disp": disp,
                    "reg": reg,
                })
                count += 1

    return results, count


def find_disp_offset(ref):
    """Find where the 4-byte displacement is within the instruction bytes.
    For standard REX.W + LEA/MOV + ModRM + disp32: disp at offset 3."""
    return 3


def build_signature(text_bytes, text_rva, ref, disp_offset):
    """Build a signature pattern with context bytes and wildcarded displacement."""
    instr_start = ref["instr_rva"] - text_rva
    instr_bytes = ref["bytes"]
    instr_len = ref["instr_size"]

    ctx_start = max(0, instr_start - CONTEXT_BEFORE)
    ctx_end = min(len(text_bytes), instr_start + instr_len + CONTEXT_AFTER)

    before = list(text_bytes[ctx_start:instr_start])
    after = list(text_bytes[instr_start + instr_len:ctx_end])

    pattern = []

    for b in before:
        pattern.append(f"{b:02X}")

    for i, b in enumerate(instr_bytes):
        if disp_offset <= i < disp_offset + 4:
            pattern.append("??")
        else:
            pattern.append(f"{b:02X}")

    for b in after:
        pattern.append(f"{b:02X}")

    return {
        "pattern": " ".join(pattern),
        "disp_offset_in_instr": disp_offset,
        "instr_len": instr_len,
        "ctx_before": len(before),
        "ctx_after": len(after),
        "instr_rva": ref["instr_rva"],
        "target_rva": ref.get("target_rva"),
    }


def find_pattern_fast(data, pattern, mask, start=0):
    """Find a byte pattern with mask in data."""
    plen = len(pattern)
    dlen = len(data)
    for i in range(start, dlen - plen + 1):
        match = True
        for j in range(plen):
            if mask[j] and data[i + j] != pattern[j]:
                match = False
                break
        if match:
            return i
    return -1


def count_pattern(data, pattern, mask):
    """Count all matches of a pattern in data."""
    count = 0
    pos = 0
    while True:
        idx = find_pattern_fast(data, pattern, mask, pos)
        if idx < 0:
            break
        count += 1
        pos = idx + 1
    return count


def learn_signatures():
    """Discover signatures from currently known RVAs."""
    cfg = load_config()
    ga_path = cfg["gameassembly_path"]

    print(f"[SIG] Loading GameAssembly.dll: {ga_path}")
    pe = pefile.PE(ga_path, fast_load=True)
    sections = get_exec_sections(pe)
    if not sections:
        print("[ERROR] No executable sections found")
        return 1

    target_rvas = list(STATIC_POINTERS.values())
    print(f"[SIG] Scanning .text for {len(target_rvas)} static pointer references...")

    t0 = time.time()
    # First pass: scan .text only (fast, 9MB)
    sections = get_exec_sections(pe, include_il2cpp=False)
    all_refs = {t: [] for t in target_rvas}
    total_count = 0
    section_map = {}

    for text_bytes, text_rva, sec_name in sections:
        section_map[sec_name] = (text_bytes, text_rva)
        refs, count = find_rip_relative_refs(text_bytes, text_rva, target_rvas)
        for t in target_rvas:
            all_refs[t].extend(refs[t])
        total_count += count

    elapsed = time.time() - t0
    print(f"[SIG] .text scan: {elapsed:.1f}s — found {total_count} references")

    # Check which targets are missing
    missing = [t for t in target_rvas if not all_refs[t]]
    if missing:
        missing_names = [n for n, t in STATIC_POINTERS.items() if t in missing]
        print(f"[SIG] {len(missing)} targets not found in .text — scanning il2cpp section...")
        print(f"       Missing: {', '.join(missing_names)}")

        # Second pass: scan il2cpp for missing targets only
        sections_il2cpp = get_exec_sections(pe, include_il2cpp=True)
        for text_bytes, text_rva, sec_name in sections_il2cpp:
            if sec_name != 'il2cpp':
                continue
            section_map[sec_name] = (text_bytes, text_rva)
            t1 = time.time()
            refs, count = find_rip_relative_refs(text_bytes, text_rva, missing)
            elapsed2 = time.time() - t1
            print(f"[SIG] il2cpp scan: {elapsed2:.1f}s — found {count} references for missing targets")
            for t in missing:
                all_refs[t].extend(refs[t])
            total_count += count

    # Build signatures
    signatures = {}
    for name, target_rva in STATIC_POINTERS.items():
        ref_list = all_refs.get(target_rva, [])
        if not ref_list:
            print(f"  [MISS] {name}: no RIP-relative reference to 0x{target_rva:X}")
            continue

        # Pick best reference (prefer LEA, first match)
        best_ref = ref_list[0]
        for r in ref_list:
            if r["mnemonic"] == "lea":
                best_ref = r
                break

        best_ref["target_rva"] = target_rva
        disp_offset = find_disp_offset(best_ref)

        # Find which section this instruction is in
        instr_rva = best_ref["instr_rva"]
        sec_data = None
        for sec_name, (data, rva) in section_map.items():
            if rva <= instr_rva < rva + len(data):
                sec_data = (data, rva, sec_name)
                break

        if not sec_data:
            print(f"  [MISS] {name}: couldn't find section for instr at 0x{instr_rva:X}")
            continue

        text_bytes, text_rva, sec_name = sec_data
        sig = build_signature(text_bytes, text_rva, best_ref, disp_offset)
        sig["target_rva"] = target_rva
        sig["mnemonic"] = best_ref["mnemonic"]
        sig["op_str"] = best_ref["op_str"]
        sig["section"] = sec_name
        signatures[name] = sig

        # Verify uniqueness
        pat_parts = sig["pattern"].split()
        pat_bytes = bytes([0 if p == "??" else int(p, 16) for p in pat_parts])
        pat_mask = [p != "??" for p in pat_parts]
        match_count = count_pattern(text_bytes, pat_bytes, pat_mask)

        unique = "UNIQUE" if match_count == 1 else f"{match_count} matches"
        print(f"  [OK]   {name}: 0x{target_rva:08X} via {best_ref['mnemonic']} {best_ref['op_str']} [{unique}]")

    # Save
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    with open(SIG_FILE, "w", encoding="utf-8") as f:
        json.dump(signatures, f, indent=2)
    print(f"\n[OK] Saved {len(signatures)} signatures to {SIG_FILE}")

    results = {name: {"rva": sig["target_rva"], "source": "sig_learn"}
              for name, sig in signatures.items()}
    with open(RESULTS_FILE, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2)

    return 0


def apply_signatures():
    """Use saved signatures to find new RVAs."""
    cfg = load_config()
    ga_path = cfg["gameassembly_path"]

    if not SIG_FILE.exists():
        print("[SIG] No saved signatures — run with --learn first")
        return 1

    with open(SIG_FILE, "r", encoding="utf-8") as f:
        signatures = json.load(f)

    print(f"[SIG] Loaded {len(signatures)} signatures")
    print(f"[SIG] Loading GameAssembly.dll: {ga_path}")

    pe = pefile.PE(ga_path, fast_load=True)

    print(f"[SIG] Scanning .text for signature matches...")

    t0 = time.time()
    results = {}

    # First pass: .text only
    sections = get_exec_sections(pe, include_il2cpp=False)

    for name, sig in signatures.items():
        pattern = sig["pattern"]
        disp_off = sig["disp_offset_in_instr"]
        instr_len = sig["instr_len"]
        ctx_before = sig["ctx_before"]

        pat_parts = pattern.split()
        pat_bytes = bytes([0 if p == "??" else int(p, 16) for p in pat_parts])
        pat_mask = [p != "??" for p in pat_parts]

        found = False
        for text_bytes, text_rva, sec_name in sections:
            matches = []
            pos = 0
            while True:
                idx = find_pattern_fast(text_bytes, pat_bytes, pat_mask, pos)
                if idx < 0:
                    break
                matches.append(idx)
                pos = idx + 1
                if len(matches) > 10:
                    break

            if not matches:
                continue

            for m in matches:
                instr_offset = m + ctx_before
                if instr_offset + instr_len > len(text_bytes):
                    continue
                disp = struct.unpack_from('<i', text_bytes, instr_offset + disp_off)[0]
                instr_rva = text_rva + instr_offset
                target_rva = instr_rva + instr_len + disp

                if target_rva > 0 and target_rva < 0x20000000:
                    results[name] = {
                        "found": True,
                        "rva": target_rva,
                        "instr_rva": instr_rva,
                        "section": sec_name,
                        "match_count": len(matches),
                    }
                    found = True
                    break

        if not found:
            results[name] = {"found": False, "error": "not found in .text"}

    # Fallback: scan il2cpp for missing targets
    missing = [name for name in signatures if name not in results or not results[name].get("found")]
    if missing:
        print(f"[SIG] {len(missing)} targets not found in .text — scanning il2cpp...")
        sections_il2cpp = get_exec_sections(pe, include_il2cpp=True)
        il2cpp_data = None
        for text_bytes, text_rva, sec_name in sections_il2cpp:
            if sec_name == 'il2cpp':
                il2cpp_data = (text_bytes, text_rva)
                break

        if il2cpp_data:
            text_bytes, text_rva = il2cpp_data
            t1 = time.time()
            for name in missing:
                sig = signatures[name]
                pattern = sig["pattern"]
                disp_off = sig["disp_offset_in_instr"]
                instr_len = sig["instr_len"]
                ctx_before = sig["ctx_before"]

                pat_parts = pattern.split()
                pat_bytes = bytes([0 if p == "??" else int(p, 16) for p in pat_parts])
                pat_mask = [p != "??" for p in pat_parts]

                # Use regex for faster scanning in il2cpp
                regex_pattern = b''
                for p, m in zip(pat_parts, pat_mask):
                    if m:
                        regex_pattern += re.escape(bytes([int(p, 16)]))
                    else:
                        regex_pattern += b'.'

                regex = re.compile(regex_pattern, re.DOTALL)
                for m in regex.finditer(text_bytes):
                    idx = m.start()
                    instr_offset = idx + ctx_before
                    if instr_offset + instr_len > len(text_bytes):
                        continue
                    disp = struct.unpack_from('<i', text_bytes, instr_offset + disp_off)[0]
                    instr_rva = text_rva + instr_offset
                    target_rva = instr_rva + instr_len + disp

                    if target_rva > 0 and target_rva < 0x20000000:
                        results[name] = {
                            "found": True,
                            "rva": target_rva,
                            "instr_rva": instr_rva,
                            "section": "il2cpp",
                            "match_count": 1,
                        }
                        break

            elapsed2 = time.time() - t1
            print(f"[SIG] il2cpp scan: {elapsed2:.1f}s")

    elapsed = time.time() - t0
    print(f"[SIG] Total scan: {elapsed:.1f}s")

    found_count = 0
    for name in signatures:
        result = results.get(name, {"found": False})
        if result.get("found"):
            old_rva = STATIC_POINTERS.get(name, 0)
            new_rva = result["rva"]
            changed = "CHANGED" if new_rva != old_rva else "same"
            matches = result.get("match_count", 0)
            unique = "UNIQUE" if matches == 1 else f"{matches} matches"
            print(f"  [OK]   {name}: 0x{new_rva:08X} [{changed}] [{unique}]")
            found_count += 1
        else:
            print(f"  [MISS] {name}: {result.get('error', 'not found')}")

    # Save results
    output = {}
    for name in signatures:
        result = results.get(name, {"found": False})
        if result.get("found"):
            output[name] = {"rva": result["rva"], "source": "sig_scan"}
        else:
            output[name] = {"rva": 0, "source": "sig_scan_failed", "error": result.get("error", "")}

    with open(RESULTS_FILE, "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)

    print(f"\n[OK] Found {found_count}/{len(signatures)} static pointers")

    # Merge into master.json
    merge_into_master(output)

    return 0 if found_count > 0 else 1


def merge_into_master(sig_results):
    """Merge sig scan results into master.json for compare_and_patch.py."""
    MASTER_JSON = OUTPUT_DIR / "master.json"
    if not MASTER_JSON.exists():
        print("[SIG] master.json not found — skipping merge")
        return

    try:
        with open(MASTER_JSON, "r", encoding="utf-8") as f:
            master = json.load(f)
    except:
        print("[SIG] Could not read master.json — skipping merge")
        return

    if "offsets" not in master:
        master["offsets"] = {}
    if "static_pointers" not in master["offsets"]:
        master["offsets"]["static_pointers"] = {}

    merged = 0
    for name, info in sig_results.items():
        rva = info.get("rva", 0)
        if rva > 0:
            existing = master["offsets"]["static_pointers"].get(name, {})
            existing_source = existing.get("source", "")
            # Only overwrite if no existing source or existing is not sha-dumper
            if "sha-dumper" not in existing_source:
                master["offsets"]["static_pointers"][name] = {
                    "rva": rva,
                    "source": "sig_scan",
                }
                merged += 1
            else:
                # Cross-validate: check if sig matches sha-dumper
                if existing.get("rva", 0) == rva:
                    existing["sig_validated"] = True
                    merged += 1
                else:
                    print(f"  [XVAL] {name}: sig=0x{rva:X} vs sha-dumper=0x{existing.get('rva', 0):X} — MISMATCH")

    with open(MASTER_JSON, "w", encoding="utf-8") as f:
        json.dump(master, f, indent=2)

    print(f"[SIG] Merged {merged} static pointers into master.json")


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Signature-based static pointer scanner")
    parser.add_argument("--learn", action="store_true", help="Learn signatures from known RVAs")
    parser.add_argument("--apply", action="store_true", help="Apply saved signatures to find new RVAs")
    args = parser.parse_args()

    if args.learn:
        return learn_signatures()
    elif args.apply:
        return apply_signatures()
    else:
        if SIG_FILE.exists():
            return apply_signatures()
        else:
            return learn_signatures()


if __name__ == "__main__":
    sys.exit(main())
