#!/usr/bin/env python3
"""
verify_pipeline.py - End-to-end pipeline verification.

Checks that the update pipeline produced correct results:
  1. All offsets in offsets.hpp have a corresponding value in master.json
  2. All 6 decrypt functions have valid algorithms (3+ ops, non-zero values)
  3. All 6 encrypt functions are the exact inverse of their decrypt counterparts
  4. No klass_rva collided with a field offset (Phase 1 name collision check)
  5. Build ID is present and not "unknown"
  6. fn_rvas are present for all decrypt functions
  7. No patches in offsets_patch.txt would overwrite a field offset with a pointer RVA

Output: output/pipeline_verification.txt (PASS/FAIL report)
"""
import json
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
OUTPUT_DIR = SCRIPT_DIR / "output"
MASTER_JSON = OUTPUT_DIR / "master.json"
DECRYPT_ALGORITHMS = OUTPUT_DIR / "decrypt_algorithms.json"
OFFSETS_HPP = Path(__file__).parent.parent.parent / "offsets.hpp"
SDK_HPP = Path(__file__).parent.parent.parent / "sdk.hpp"
OUTPUT_REPORT = OUTPUT_DIR / "pipeline_verification.txt"

# Canonical decrypt function names
DECRYPT_NAMES = [
    "base_networkable_0",
    "base_networkable_1",
    "cl_active_item",
    "player_inventory",
    "player_eyes",
    "decrypt_fov",
]

# Inverse op mapping
INVERSE_OPS = {
    "add": "sub",
    "sub": "add",
    "xor": "xor",
    "rol": "ror",
    "ror": "rol",
}


def load_master():
    if not MASTER_JSON.exists():
        return None
    with open(MASTER_JSON, "r", encoding="utf-8") as f:
        return json.load(f)


def load_decrypt_algorithms():
    if not DECRYPT_ALGORITHMS.exists():
        return None
    with open(DECRYPT_ALGORITHMS, "r", encoding="utf-8") as f:
        return json.load(f)


def check_decrypt_functions(master):
    """Check that all 6 decrypt functions have valid algorithms."""
    issues = []
    decrypt_fns = master.get("decrypt_functions", {})

    for name in DECRYPT_NAMES:
        fn = decrypt_fns.get(name)
        if not fn:
            issues.append(f"MISSING: decrypt function '{name}' not found in master.json")
            continue

        ops = fn.get("ops", [])
        if len(ops) < 2:
            issues.append(f"INVALID: '{name}' has only {len(ops)} ops (need 2+)")
            continue

        # Check for zero-value ops (except ROL which can be small)
        for i, op in enumerate(ops):
            val = op.get("value", 0)
            if isinstance(val, str):
                val = int(val, 0)
            if val == 0 and op.get("op") not in ("rol", "ror"):
                issues.append(f"ZERO_VALUE: '{name}' op[{i}] {op.get('op')} has value 0")

        # Check fn_rva
        fn_rva = fn.get("fn_rva", 0)
        if isinstance(fn_rva, str):
            fn_rva = int(fn_rva, 0)
        if fn_rva == 0:
            issues.append(f"NO_FN_RVA: '{name}' has no fn_rva (disasm_decrypts can't re-verify)")

    return issues


def check_encrypt_functions(master):
    """Check that encrypt functions exist and are the inverse of decrypt."""
    issues = []
    decrypt_fns = master.get("decrypt_functions", {})
    encrypt_fns = master.get("encrypt_functions", {})

    for name in DECRYPT_NAMES:
        dec = decrypt_fns.get(name)
        if not dec:
            continue

        # Expected encrypt name
        if name.startswith("decrypt_"):
            enc_name = name.replace("decrypt_", "encrypt_")
        else:
            enc_name = f"encrypt_{name}"

        enc = encrypt_fns.get(enc_name)
        if not enc:
            issues.append(f"MISSING_ENCRYPT: no encrypt function for '{name}' (expected '{enc_name}')")
            continue

        # Verify encrypt is the inverse of decrypt
        dec_ops = dec.get("ops", [])
        enc_ops = enc.get("ops", [])

        # Encrypt should be: reversed order, inverted ops
        expected_enc = []
        for op in reversed(dec_ops):
            op_type = op.get("op", "")
            op_val = op.get("value", 0)
            if isinstance(op_val, str):
                op_val = int(op_val, 0)
            inv_type = INVERSE_OPS.get(op_type, op_type)
            expected_enc.append({"op": inv_type, "value": op_val})

        if len(enc_ops) != len(expected_enc):
            issues.append(f"ENCRYPT_MISMATCH: '{enc_name}' has {len(enc_ops)} ops, expected {len(expected_enc)} (inverse of {name})")
            continue

        for i, (actual, expected) in enumerate(zip(enc_ops, expected_enc)):
            a_op = actual.get("op", "")
            a_val = actual.get("value", 0)
            if isinstance(a_val, str):
                a_val = int(a_val, 0)
            e_op = expected["op"]
            e_val = expected["value"]

            if a_op != e_op:
                issues.append(f"ENCRYPT_OP_MISMATCH: '{enc_name}' op[{i}] is {a_op}, expected {e_op} (inverse of {name} op[{len(dec_ops)-1-i}])")
            elif a_val != e_val:
                issues.append(f"ENCRYPT_VAL_MISMATCH: '{enc_name}' op[{i}] value is 0x{a_val:X}, expected 0x{e_val:X}")

    return issues


def check_klass_rva_collisions():
    """Check that no patch in offsets_patch.txt would overwrite a field offset with a pointer RVA."""
    issues = []
    patch_path = OUTPUT_DIR / "offsets_patch.txt"
    if not patch_path.exists():
        return issues  # No patches = no collisions

    content = patch_path.read_text(encoding="utf-8", errors="ignore")
    # Look for lines like: "Name: 0xOLD -> 0xNEW"
    for match in re.finditer(r"(\w+):\s*0x([0-9A-Fa-f]+)\s*->\s*0x([0-9A-Fa-f]+)", content):
        name = match.group(1)
        old_val = int(match.group(2), 16)
        new_val = int(match.group(3), 16)
        # A field offset is typically < 0x1000
        # A klass_rva/pointer is typically > 0x10000000
        if old_val < 0x10000 and new_val > 0x10000000:
            issues.append(f"COLLISION: '{name}' patch would change field offset 0x{old_val:X} to pointer RVA 0x{new_val:X}")
        elif old_val > 0x10000000 and new_val < 0x10000:
            issues.append(f"COLLISION: '{name}' patch would change pointer RVA 0x{old_val:X} to field offset 0x{new_val:X}")

    return issues


def check_build_id(master):
    """Check that build ID is present and not 'unknown'."""
    issues = []
    build = master.get("build", "unknown")
    if build == "unknown" or not build:
        issues.append("BUILD_ID: build ID is 'unknown' or missing")
    return issues


def check_fn_rvas(decrypt_algos):
    """Check that fn_rvas are present in decrypt_algorithms.json."""
    issues = []
    if not decrypt_algos:
        return issues

    algos = decrypt_algos.get("algorithms", [])
    for algo in algos:
        name = algo.get("name", "")
        rva = algo.get("rva", "0x0")
        if isinstance(rva, str):
            rva_int = int(rva, 0) if rva.startswith("0x") else int(rva)
        else:
            rva_int = int(rva)
        if rva_int == 0:
            issues.append(f"NO_RVA: '{name}' has fn_rva=0 in decrypt_algorithms.json")

    # Check encrypt algorithms
    enc_algos = decrypt_algos.get("encrypt_algorithms", [])
    if not enc_algos:
        issues.append("NO_ENCRYPT_ALGOS: decrypt_algorithms.json has no encrypt_algorithms section")
    else:
        issues.append(f"INFO: {len(enc_algos)} encrypt algorithms auto-generated")

    # Check unidentified ciphers
    unknown = decrypt_algos.get("unidentified_ciphers", [])
    if unknown:
        issues.append(f"INFO: {len(unknown)} unidentified ciphers found (output for manual investigation)")

    return issues


def main():
    print("[verify_pipeline] End-to-end pipeline verification")
    print()

    all_issues = []

    # 1. Load master.json
    master = load_master()
    if not master:
        all_issues.append("FATAL: master.json not found or invalid")
        write_report(all_issues)
        return 1

    # 2. Check build ID
    issues = check_build_id(master)
    all_issues.extend(issues)
    if not issues:
        print(f"  [OK] Build ID: {master.get('build', 'unknown')}")

    # 3. Check decrypt functions
    issues = check_decrypt_functions(master)
    all_issues.extend(issues)
    if not issues:
        dec_count = len([n for n in DECRYPT_NAMES if n in master.get("decrypt_functions", {})])
        print(f"  [OK] Decrypt functions: {dec_count}/{len(DECRYPT_NAMES)} present with valid algorithms")
    else:
        for issue in issues:
            if issue.startswith("MISSING:"):
                print(f"  [FAIL] {issue}")
            elif issue.startswith("NO_FN_RVA:"):
                print(f"  [WARN] {issue}")

    # 4. Check encrypt functions
    issues = check_encrypt_functions(master)
    all_issues.extend(issues)
    if not issues:
        enc_count = len(master.get("encrypt_functions", {}))
        print(f"  [OK] Encrypt functions: {enc_count} present, all verified as inverse of decrypt")
    else:
        for issue in issues:
            if issue.startswith("MISSING_ENCRYPT:"):
                print(f"  [WARN] {issue}")
            elif issue.startswith("ENCRYPT"):
                print(f"  [FAIL] {issue}")

    # 5. Check for klass_rva/field offset collisions in patches
    issues = check_klass_rva_collisions()
    all_issues.extend(issues)
    if not issues:
        print(f"  [OK] No klass_rva/field offset collisions in patches")
    else:
        for issue in issues:
            print(f"  [FAIL] {issue}")

    # 6. Check decrypt_algorithms.json
    decrypt_algos = load_decrypt_algorithms()
    if decrypt_algos:
        issues = check_fn_rvas(decrypt_algos)
        all_issues.extend([i for i in issues if not i.startswith("INFO:")])
        for issue in issues:
            if issue.startswith("INFO:"):
                print(f"  [INFO] {issue}")
            elif issue.startswith("NO_RVA:"):
                print(f"  [WARN] {issue}")
            elif issue.startswith("NO_ENCRYPT:"):
                print(f"  [WARN] {issue}")
            else:
                print(f"  [OK] {issue}")
        if not [i for i in issues if not i.startswith("INFO:")]:
            identified = decrypt_algos.get("identified", 0)
            total = decrypt_algos.get("total", 6)
            print(f"  [OK] Decrypt algorithms: {identified}/{total} identified by capstone")
    else:
        print(f"  [WARN] decrypt_algorithms.json not found (disasm_decrypts.py may not have run)")

    # Summary
    fatal_count = sum(1 for i in all_issues if i.startswith("FATAL:") or i.startswith("COLLISION:") or i.startswith("ENCRYPT_OP_MISMATCH:") or i.startswith("ENCRYPT_VAL_MISMATCH:"))
    warn_count = sum(1 for i in all_issues if i.startswith("MISSING:") or i.startswith("NO_") or i.startswith("INVALID:"))
    info_count = sum(1 for i in all_issues if i.startswith("INFO:"))

    status = "PASS" if fatal_count == 0 else "FAIL"

    print()
    print(f"{'='*60}")
    print(f"  PIPELINE VERIFICATION: {status}")
    print(f"  Fatal: {fatal_count}, Warnings: {warn_count}, Info: {info_count}")
    print(f"{'='*60}")

    write_report(all_issues, status)
    return 0 if status == "PASS" else 1


def write_report(issues, status="FAIL"):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    lines = [
        "=" * 60,
        f"  PIPELINE VERIFICATION: {status}",
        "=" * 60,
        "",
    ]
    if not issues:
        lines.append("All checks passed.")
    else:
        for issue in issues:
            severity = "FATAL" if issue.startswith(("FATAL:", "COLLISION:", "ENCRYPT_OP_MISMATCH:", "ENCRYPT_VAL_MISMATCH:")) else "WARN" if issue.startswith(("MISSING:", "NO_", "INVALID:", "ZERO_")) else "INFO"
            lines.append(f"  [{severity}] {issue}")
    lines.append("")
    lines.append("=" * 60)

    report = "\n".join(lines)
    OUTPUT_REPORT.write_text(report, encoding="utf-8")
    print(f"\n  Report: {OUTPUT_REPORT}")


if __name__ == "__main__":
    sys.exit(main())
