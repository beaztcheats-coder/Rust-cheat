#!/usr/bin/env python3
"""
frida_decrypt_scan.py v3 - Extract decrypt algorithms via Frida + capstone.

ONLY scans the specific decrypt function RVAs found by sha-dumper.
Does NOT scan all methods — only reads bytes at the 6 known cipher addresses.

Uses capstone disassembler for 100% accurate algorithm extraction.
"""
import json
import sys
import time
import re
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_IMM

OUTPUT_DIR = Path(__file__).parent / "output"
BRIDGE_PATH = Path(r"C:\frida-il2cpp\dist\index.js")
SHA_DUMPER_OUTPUT = OUTPUT_DIR / "sha-dumper-output.txt"
OUTPUT_FILE = OUTPUT_DIR / "frida_decrypt_algorithms.json"

# Only scan these cipher sections from sha-dumper (ignore cipher_unknown_*)
KNOWN_CIPHERS = ["bn_0", "bn_1", "cla", "inventory", "eyes", "fov",
                 "cl_active_item", "player_inventory", "player_eyes", "decrypt_fov"]


KNOWN_CIPHERS = ["bn_0", "bn_1", "cla", "inventory", "eyes", "fov",
                 "cl_active_item", "player_inventory", "player_eyes", "decrypt_fov",
                 "base_networkable_0", "base_networkable_1"]


def get_decrypt_rvas():
    """Parse sha-dumper output to find KNOWN decrypt function RVAs only."""
    if not SHA_DUMPER_OUTPUT.exists():
        print("[WARN] sha-dumper-output.txt not found")
        return {}
    
    content = SHA_DUMPER_OUTPUT.read_text(encoding="utf-8", errors="ignore")
    rvas = {}
    
    # Find all cipher sections
    ini_sections = re.findall(r'\[(cipher_[^\]]+)\]([\s\S]*?)(?=\n\[|\Z)', content)
    for name, body in ini_sections:
        # Only keep known ciphers, skip unknown_N
        if name.startswith("cipher_unknown"):
            continue
        
        # Only keep KNOWN cipher names
        clean_name = name.replace("cipher_", "")
        if clean_name not in KNOWN_CIPHERS:
            continue
        
        rva_match = re.search(r'fn_rva\s*=\s*(0x[0-9A-Fa-f]+)', body, re.IGNORECASE)
        if rva_match:
            rva = int(rva_match.group(1), 16)
            # Clean up name (remove cipher_ prefix)
            clean_name = name.replace("cipher_", "")
            rvas[clean_name] = rva
    
    # Also try master.json for decrypt function RVAs
    master_path = OUTPUT_DIR / "master.json"
    if master_path.exists():
        with open(master_path, "r") as f:
            master = json.load(f)
        for name, info in master.get("decrypt_functions", {}).items():
            rva = info.get("read_offset") or info.get("fn_rva")
            if rva and isinstance(rva, (int, str)):
                try:
                    rva_int = int(rva, 16) if isinstance(rva, str) else int(rva)
                    if rva_int > 0x10000 and name not in rvas:
                        rvas[name] = rva_int
                except:
                    pass
    
    return rvas


def extract_cipher_ops(code_bytes):
    """Use capstone to disassemble and extract cipher operations.

    Handles ROL implemented as SHL+SHR+OR on different registers (compiler pattern).
    Also handles SHL+SHR on same register and native ROR.
    """
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True

    ops = []
    pending_shl = None   # (amount, reg) for SHL
    pending_shr = None    # (amount, reg) for SHR

    for insn in md.disasm(code_bytes, 0x1000):
        m = insn.mnemonic
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

    # Handle loop unrolling
    if len(ops) >= 4 and len(ops) % 2 == 0:
        half = len(ops) // 2
        if ops[:half] == ops[half:]:
            ops = ops[:half]

    return ops


FRIDA_AGENT = r"""
try {{
    setTimeout(() => {{
        Il2Cpp.perform(() => {{
            send({{action: "init"}});
            const ga = Process.getModuleByName('GameAssembly.dll').base;
            const targets = {targets_json};
            let scanned = 0;
            for (const t of targets) {{
                const addr = ga.add(t.rva);
                try {{
                    const buf = addr.readByteArray(256);
                    send({{
                        type: "fn_bytes",
                        name: t.name,
                        rva: "0x" + t.rva.toString(16),
                        bytes: Array.from(new Uint8Array(buf))
                    }});
                    scanned++;
                }} catch(e) {{
                    send({{type: "error", name: t.name, error: e.message}});
                }}
            }}
            send({{action: "done", scanned: scanned}});
        }}).catch(e => send({{action: "error", error: e.message}}));
    }});
}} catch(e) {{
    send({{action: "fatal", error: e.message}});
}}
"""


def main():
    import frida
    
    print("[DECRYPT] Frida + capstone decrypt scanner v3")
    
    rvas = get_decrypt_rvas()
    if not rvas:
        print("[ERROR] No decrypt RVAs found in sha-dumper output")
        return 1
    
    print(f"[OK] Found {len(rvas)} decrypt function RVAs:")
    for name, rva in sorted(rvas.items()):
        print(f"  {name}: 0x{rva:X}")
    
    # Check Rust running
    import subprocess
    result = subprocess.run(["tasklist", "/FI", "IMAGENAME eq RustClient.exe", "/NH"],
                          capture_output=True, text=True)
    if "RustClient.exe" not in result.stdout:
        print("[ERROR] RustClient.exe not running!")
        return 1
    
    bridge_code = BRIDGE_PATH.read_text(encoding="utf-8", errors="ignore")
    targets = [{"name": k, "rva": v} for k, v in rvas.items()]
    agent_code = bridge_code + "\n\n" + FRIDA_AGENT.format(targets_json=json.dumps(targets))
    
    results = []
    done = [False]
    
    def on_message(msg, data):
        if msg["type"] == "error":
            done[0] = True
            return
        if msg["type"] != "send":
            return
        payload = msg.get("payload", {})
        action = payload.get("action", "")
        
        if action == "init":
            print("[OK] Bridge initialized")
        elif action == "done":
            print(f"[OK] Scanned {payload.get('scanned', 0)} functions")
            done[0] = True
        elif action in ("fatal", "error"):
            print(f"[ERROR] {payload.get('error', '?')}")
            done[0] = True
        elif payload.get("type") == "fn_bytes":
            name = payload.get("name", "?")
            bytes_arr = payload.get("bytes", [])
            ops = extract_cipher_ops(bytes(bytes_arr))
            
            if len(ops) >= 2:
                print(f"\n  [{name}] {len(ops)} ops found:")
                for op_type, op_val in ops:
                    if op_val > 0xFFFF:
                        print(f"    {op_type.upper()} 0x{op_val:X}")
                    else:
                        print(f"    {op_type.upper()} {op_val}")
                results.append({
                    "name": name,
                    "rva": payload.get("rva"),
                    "ops": [{"op": o[0], "value": f"0x{o[1]:X}" if o[1] > 0xFFFF else o[1]} for o in ops],
                    "ops_raw": [[o[0], o[1]] for o in ops],
                })
            else:
                print(f"  [{name}] Only {len(ops)} ops — not a decrypt function")
        elif payload.get("type") == "error":
            print(f"  [{payload.get('name','?')}] Read error: {payload.get('error','?')}")
    
    session = frida.attach("RustClient.exe")
    script = session.create_script(agent_code)
    script.on("message", on_message)
    script.load()
    
    start = time.time()
    while not done[0]:
        time.sleep(0.5)
        if time.time() - start > 30:
            break
    
    session.detach()
    
    output = {"status": "SUCCESS" if results else "NO_DECRYPTS", "decrypts": len(results), "algorithms": results}
    OUTPUT_FILE.write_text(json.dumps(output, indent=2), encoding="utf-8")
    
    print(f"\n{'='*50}")
    print(f"  Found {len(results)} decrypt algorithms")
    print(f"  Output: {OUTPUT_FILE}")
    print(f"{'='*50}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
