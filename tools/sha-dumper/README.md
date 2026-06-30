# sha-dumper

Runtime memory dumper injected into RustClient.exe. Extracts all offsets, decrypt functions, static pointers, and native struct layouts from the running game via Il2Cpp API + signature scanning + HDE64 disassembly.

## What It Dumps

| Category | Method | Coverage |
|----------|--------|----------|
| Field offsets | Il2Cpp `field_get_offset()` | 35+ classes, all non-static fields |
| Static TypeInfos | `.data` section scan for Il2CppClass pointers | BN, Camera, TOD_Sky, EffectNetwork, etc. |
| GCHandle RVA | Export scan + call graph + LEA resolution | Automatic |
| BN decrypt functions | Signature scan + `cipher_t::from_loop()` | client_entities + entity_list |
| FOV decrypt | Cvar walk + disassembly | decrypt_fov |
| Extra decrypts | Cipher loop scan + op-type pattern matching | cl_active_item, inventory, eyes |
| Native camera | Heuristic value scan (FOV, matrices, culling) | viewMatrix, projectionMatrix, etc. |
| TOD_Sky instances | Static field scan + instance validation | TOD_Sky_Static::instances offset |
| Materials | `FindObjectsOfTypeAll` | All material name→ID pairs |
| Unity constants | Hardcoded (verified against engine) | 30+ layout offsets |

## Build

```
MSBuild sha-dumper.vcxproj /p:Configuration=Release /p:Platform=x64
```

Output: `tools/build/Release/sha-dumper.dll`

## Usage

The master update pipeline handles injection automatically:

```
auto_update.bat  →  sha_dumper_inject.py builds + injects + waits for output
```

### Manual injection:

```powershell
.\tools\build\Release\inject.ps1 .\tools\build\Release\sha-dumper.dll
```

Output is written to `%USERPROFILE%\Desktop\sha-dumper_output.txt`.

## Output Format

INI-style sections:
```
[base_networkable]
slot_rva = 0xE334210
static_fields = 0xB8
wrapper_off = 0x20
...

[cipher_bn_0]
fn_rva = 0xF8CF90
op_count = 3
op[0] = ROL 0x2
op[1] = XOR 0x111B9118
op[2] = ADD 0x79300E2E

[BasePlayer]
  clActiveItem = 0x520
  eyes = 0x3E0
  ...
```

Parsed by `parse_sha_dumper.py` into `master.json` format.

## Architecture

- `resolver/` — Main orchestration: BN chain, camera, cipher discovery
- `cipher/` — Cipher loop extraction via HDE64 disassembly (ROL/XOR/ADD/SUB)
- `mem/` — Pattern scanning, GCHandle RVA resolution
- `il2cpp/` — Full Il2Cpp API bindings (35+ exports)
- `report/` — INI-style output writer
- `mat/` — Material dumper via FindObjectsOfTypeAll
- `glue/` — Runtime helpers (klass lookup, RVA scan, static nested)
