#!/usr/bin/env python3
"""Find nk2 and fov ciphers among unknown cipher blocks in sha-dumper output."""
import re
import sys

sha_file = r"E:\github\rust\tools\masterupdate\output\sha-dumper-output.txt"
with open(sha_file, 'r') as f:
    content = f.read()

# Split into cipher blocks
blocks = re.split(r'(?=\[cipher_)', content)

# Known nk2 constants
NK2_ROL = 0x6
NK2_XOR = 0xC5D748E1
NK2_ADD = 0x48498B34

# Known fov constants
FOV_ROL = 0x1F
FOV_SUB = 0x0270C775
FOV_XOR = 0x93DAED4D

# Also search for fov pattern (ROL-SUB-XOR, 3 ops) with ANY constants
# And nk2 pattern (ROL-XOR-ADD, 3 ops) with ANY constants

print("=== Searching for nk2 (ROL-XOR-ADD, 3 ops) ===")
for block in blocks:
    if not block.startswith('[cipher_'): continue
    tag_m = re.match(r'\[cipher_(\w+)\]', block)
    if not tag_m: continue
    tag = tag_m.group(1)
    
    oc_m = re.search(r'op_count\s*=\s*(\d+)', block)
    if not oc_m: continue
    oc = int(oc_m.group(1))
    if oc != 3: continue
    
    ops = re.findall(r'op\[\d+\]\s*=\s*(\w+)\s+(0x[0-9A-Fa-f]+)', block)
    if len(ops) != 3: continue
    
    types = [t.upper() for t, _ in ops]
    vals = [int(v, 16) for _, v in ops]
    
    # nk2 pattern: ROL-XOR-ADD
    if types == ['ROL', 'XOR', 'ADD']:
        rva_m = re.search(r'fn_rva\s*=\s*(0x[0-9A-Fa-f]+)', block)
        rva = rva_m.group(1) if rva_m else "?"
        match = " *** EXACT MATCH ***" if vals == [NK2_ROL, NK2_XOR, NK2_ADD] else ""
        print(f"  cipher_{tag}: rva={rva} ROL={vals[0]:#x} XOR={vals[1]:#x} ADD={vals[2]:#x}{match}")

print()
print("=== Searching for fov (ROL-SUB-XOR, 3 ops) ===")
for block in blocks:
    if not block.startswith('[cipher_'): continue
    tag_m = re.match(r'\[cipher_(\w+)\]', block)
    if not tag_m: continue
    tag = tag_m.group(1)
    
    oc_m = re.search(r'op_count\s*=\s*(\d+)', block)
    if not oc_m: continue
    oc = int(oc_m.group(1))
    if oc != 3: continue
    
    ops = re.findall(r'op\[\d+\]\s*=\s*(\w+)\s+(0x[0-9A-Fa-f]+)', block)
    if len(ops) != 3: continue
    
    types = [t.upper() for t, _ in ops]
    vals = [int(v, 16) for _, v in ops]
    
    # fov pattern: ROL-SUB-XOR
    if types == ['ROL', 'SUB', 'XOR']:
        rva_m = re.search(r'fn_rva\s*=\s*(0x[0-9A-Fa-f]+)', block)
        rva = rva_m.group(1) if rva_m else "?"
        match = " *** EXACT MATCH ***" if vals == [FOV_ROL, FOV_SUB, FOV_XOR] else ""
        print(f"  cipher_{tag}: rva={rva} ROL={vals[0]:#x} SUB={vals[1]:#x} XOR={vals[2]:#x}{match}")

print()
print("=== Also searching for fov with SUB-XOR pattern (2 ops, might be unrolled) ===")
for block in blocks:
    if not block.startswith('[cipher_'): continue
    tag_m = re.match(r'\[cipher_(\w+)\]', block)
    if not tag_m: continue
    tag = tag_m.group(1)
    
    oc_m = re.search(r'op_count\s*=\s*(\d+)', block)
    if not oc_m: continue
    oc = int(oc_m.group(1))
    if oc != 2: continue
    
    ops = re.findall(r'op\[\d+\]\s*=\s*(\w+)\s+(0x[0-9A-Fa-f]+)', block)
    if len(ops) != 2: continue
    
    types = [t.upper() for t, _ in ops]
    vals = [int(v, 16) for _, v in ops]
    
    # Check if any op value matches fov constants
    if FOV_SUB in vals or FOV_XOR in vals or FOV_ROL in vals:
        rva_m = re.search(r'fn_rva\s*=\s*(0x[0-9A-Fa-f]+)', block)
        rva = rva_m.group(1) if rva_m else "?"
        print(f"  cipher_{tag}: rva={rva} types={types} vals={[hex(v) for v in vals]} *** CONTAINS FOV CONSTANT ***")

print()
print("=== Searching ALL ciphers for fov constants (any op_count) ===")
for block in blocks:
    if not block.startswith('[cipher_'): continue
    tag_m = re.match(r'\[cipher_(\w+)\]', block)
    if not tag_m: continue
    tag = tag_m.group(1)
    
    ops = re.findall(r'op\[\d+\]\s*=\s*(\w+)\s+(0x[0-9A-Fa-f]+)', block)
    vals = [int(v, 16) for _, v in ops]
    
    if FOV_SUB in vals or FOV_XOR in vals:
        rva_m = re.search(r'fn_rva\s*=\s*(0x[0-9A-Fa-f]+)', block)
        rva = rva_m.group(1) if rva_m else "?"
        types = [t for t, _ in ops]
        print(f"  cipher_{tag}: rva={rva} ops={list(zip(types, [hex(v) for v in vals]))} *** CONTAINS FOV CONSTANT ***")
