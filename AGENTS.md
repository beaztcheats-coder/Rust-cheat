# AGENTS.md — Rust External Cheat

## Build

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /m
```

- VS2022 vcxproj, C++20, x64 Release DLL
- Build can also be run via `tools\masterupdate\getnewoffsets.bat` + `update_now.bat` (only 2 pipeline entry points — legacy scripts removed)
- Requires DirectX SDK (included in repo under `Microsoft DirectX SDK/`)

### Build flavors (4)

| Flavor | TargetName | Output DLL | Logging | Shutdown Cleanup | Menu |
|--------|-----------|-----------|---------|-----------------|------|
| **BeaZt debug** | `Rust Prv Ext_debug` | `Rust Prv Ext_debug.dll` | Auto-enabled (DLL name `_debug`) | Keeps log files; deletes configs/markers/decrypts only | BeaZt |
| **BeaZt production** | `Rust Prv Ext` | `Rust Prv Ext.dll` | Disabled (enable via `C:\rust_debug_enabled.txt` marker) | **Deletes ALL trace files** (logs, configs, markers, decrypts) | BeaZt |
| **Bomza production** | `bomzarust` | `bomzarust.dll` | Disabled (enable via `C:\rust_debug_enabled.txt` marker) | **Deletes ALL trace files** (logs, configs, markers, decrypts) | Bomza |
| **Better Cheats production** | `bettercheats` | `bettercheats.dll` | Disabled (enable via `C:\rust_debug_enabled.txt` marker) | **Deletes ALL trace files** (logs, configs, markers, decrypts) | Better Cheats |

### Flavor behavior

**Debug build** (`Rust Prv Ext_debug.dll`):
- Logging auto-enabled at runtime — DLL name contains `_debug`
- `g_UpdateLoggingEnabled` set to `true` by `MainThreadImpl()` before `Logger::Init()`
- On shutdown: deletes `rust_decrypts.dat`, `cheat_dll_loaded.txt`, `rust_debug_enabled.txt`, configs, spoofer files — **KEEPS** `cheat_debug.log` for post-shutdown analysis
- Log path: `C:\cheat_debug.log`
- Config path: `C:\rustcfg.dat`
- Use for: development, debugging, diagnostics

**Production BeaZt** (`Rust Prv Ext.dll`):
- Logging disabled by default (`g_UpdateLoggingEnabled = false` in `Logger.hpp`)
- Enable temporarily: create `C:\rust_debug_enabled.txt` marker (admin: `echo enabled > C:\rust_debug_enabled.txt`)
- On shutdown: **deletes ALL trace files** including `cheat_debug.log` — zero forensic footprint
- Log path: `C:\cheat_debug.log` (only if marker exists)
- Config path: `C:\rustcfg.dat`
- Use for: customer distribution

**Production Bomza** (`bomzarust.dll`):
- Same stealth behavior as BeaZt production — logging off, full trace cleanup on shutdown
- Detected via `RuntimePaths::IsBomza()` (DLL name contains `bomza`)
- Log path: `C:\cheat_debug_bomza.log` (only if marker exists)
- Config path: `C:\rustcfg_bomza.dat`
- Menu: Bomza branded (`Bomza/bomza_menu.hpp`)
- Spoofer prompt: 3-option (Spoof and Play / Just play / Clean after detection)
- Use for: Bomza customer distribution

**Better Cheats production** (`bettercheats.dll`):
- Same stealth behavior as BeaZt/Bomza production — logging off, full trace cleanup on shutdown
- Detected via `RuntimePaths::IsBetterCheats()` (DLL name contains `bettercheats`)
- Log path: `C:\cheat_debug_bc.log` (only if marker exists)
- Config path: `C:\rustcfg_bc.dat`
- Menu: Better Cheats branded (`BetterCheats/bettercheats_menu.hpp`, dark blue + yellow theme)
- No spoofer/cleaner prompt — straight to cheat (same as Bomza)
- Use for: Better Cheats customer distribution

### Build all 4 flavors

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName="Rust Prv Ext_debug" /m
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName="Rust Prv Ext" /m
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName=bomzarust /m
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName=bettercheats /m
```

### Debug logging

- **Production builds**: Logging disabled by default. Enable by creating `C:\rust_debug_enabled.txt` (run as admin: `echo enabled > C:\rust_debug_enabled.txt`)
- **Debug build** (`Rust Prv Ext_debug.dll`): Logging auto-enabled — DLL name contains `_debug`, detected at runtime via `GetModuleFileNameA` + `find("_debug")`
- **Log output**: `C:\cheat_debug.log` (BeaZt) / `C:\cheat_debug_bomza.log` (Bomza) / `C:\cheat_debug_bc.log` (Better Cheats)
- **Stealth mode**: Production builds delete ALL trace files on shutdown (logs, configs, markers, decrypts). Debug builds keep logs but delete everything else.
- **`Logger.hpp`**: `g_UpdateLoggingEnabled` defaults to `false`. Set to `true` only by: (1) `_debug` DLL name check, or (2) `C:\rust_debug_enabled.txt` marker file. Both checks are in `MainThreadImpl()` at `Rust Prv Ext.cpp` lines ~254-272.

### Spoofer/Cleaner startup prompt

**BeaZt** (debug + production): Shows 3-option console prompt before cheat init:
1. **[1] Spoof and Play** — calls `RunEmbeddedSpoofer()` → MessageBox "Spoof now? Yes/No" → 11 IOCTL spoofs → cheat continues
2. **[2] Clean after detection** — calls `CleanerMenu()` → interactive 8-phase deep cleaner → DLL exits via `FreeLibraryAndExitThread` (user restarts + re-injects)
3. **[3] Just play** — skip, cheat continues

**Bomza** (production): Shows 3-option console prompt:
1. **[1] Spoof and Play** — calls `RunEmbeddedSpoofer()` → same spoofer as BeaZt
2. **[2] Just play** — skip, cheat continues
3. **[3] Clean after detection** — calls `CleanerMenu()` → deep cleaner → DLL exits

**Better Cheats** (production): No prompt — straight to cheat.

**Guard macros**: Spoofer compiled for `#if !defined(BETTERCHEATS)` (BeaZt + Bomza). Cleaner compiled for `#if !defined(BETTERCHEATS)` (BeaZt + Bomza). Dead-stripped by `/OPT:REF` for BetterCheats.

**Spoofer GPU fix**: The spoofer NO LONGER deletes GPU-critical registry keys:
- ~~`SYSTEM\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}`~~ (Display Adapters device class — was causing GPU driver reinstallation)
- ~~`SYSTEM\CurrentControlSet\Control\GraphicsDrivers\ConfigTree`~~ (GPU display config tree)
- ~~`SYSTEM\CurrentControlSet\Enum\DISPLAY`~~ (monitor enumeration — was forcing monitor re-detection)
- ~~`netsh winsock reset`~~ and ~~`net stop/start winmgmt`~~ (was causing network instability)
- KEPT: NVIDIA ClientUUID deletion (safe, auto-regenerated), TPM wipe, file trace cleanup, DNS flush
- GPU HWID is spoofed at kernel level via `Spoof_GPU` IOCTL — registry cleanup was redundant

**Files**: `Hacks/Misc/spoofer.cpp` (spoof logic), `Hacks/Misc/cleaner.cpp` (cleaner menu, BeaZt only), `Rust Prv Ext.cpp` (prompt).

## Offset Pipeline (Morphine-Dumper Primary)

The pipeline uses **morphine-dumper** (injected DLL) as the primary source for all offsets, decrypts, and encrypts. `disasm_decrypts.py` (offline capstone) is the single source of truth for decrypt/encrypt algorithms. sha-dumper and Frida are optional cross-validation.

### Sources (priority order)

| Source | Provides | Requires Game? | Status |
|--------|----------|---------------|--------|
| **morphine-dumper** (injected DLL) | Field offsets, static TypeInfos, ALL decrypt fn_rvas, camera chain (Unity method oracles), model bones, GCHandle RVA, FOV encrypt/decrypt, ALL assembly classes (auto-discovered) | Yes (game running) | Working — **PRIMARY SOURCE** |
| **disasm_decrypts.py** (offline capstone) | Decrypt + encrypt algorithms (ROL/XOR/ADD/SUB chains) — reads fn_rvas from master.json or scans GameAssembly.dll .text section | No (offline) | Working — **AUTHORITY for decrypts** |
| **sha-dumper** (injected DLL) | Cipher section enumeration (930+ sections), mesh data, cross-validation | Yes (game running) | Working — **optional cross-validation** |
| **Frida** (runtime dump) | Field offsets, field types, method RVAs, cross-validation | Yes (game running) | Working — **optional cross-validation** |
| **Il2CppInspectorPro** (offline) | Field offsets, TypeInfo RVAs, method RVAs | No | Broken (.NET 10 preview required) |

### Pipeline flow (`getnewoffsets.bat`)

1. **morphine-dumper** (injected into game) — dumps offsets + decrypt fn_rvas + camera + bones + ALL assembly classes → `morphine-dump.h` + `morphine-dump.json`
2. **parse_morphine_dump.py** — reads JSON (preferred) or .h, produces `master.json`
3. **disasm_decrypts.py** — reads fn_rvas from master.json, disassembles with capstone, identifies 6 decrypt + 6 encrypt algorithms, merges into master.json. Falls back to standalone cipher scan if no fn_rvas.
4. **sig_scanner.py** (offline) — signature-based static pointer scan
5. **sha-dumper** (optional) — cipher section enumeration + mesh, cross-validation
6. **Frida** (optional) — runtime field offset cross-validation
7. **compare_and_patch.py** — generates 5 patch files from master.json (name collision fix prevents klass_rvas overwriting field offsets)
8. **verify_pipeline.py** — end-to-end PASS/FAIL check (decrypt/encrypt correctness, collision safety, fn_rva presence)
9. **apply_patches.py** — applies patches to offsets.hpp/sdk.hpp/OffsetManager.hpp

### morphine-dumper output

The dumper produces **two files** on Desktop:
- `morphine-dumper_output.h` — C++ header (human-readable, with `// fn_rva` comments + `encryption::` functions)
- `morphine-dumper_output.json` — structured JSON for pipeline consumption

JSON contains: `build`, `game_base`, `sections` (all offsets + klass_rvas + ALL assembly classes), `decrypt_functions` (with fn_rva + ops), `encrypt_functions` (auto-generated inverse of decrypt).

### disasm_decrypts.py — How it works

1. Reads fn_rvas from master.json (morphine-dumper) — preferred source
2. Falls back to sha-dumper output if master.json has no fn_rvas
3. Falls back to **standalone cipher scan** (scans .text section for `mov reg,2` prologues) if no dumper output at all
4. For each cipher, reads bytes at fn_rva from GameAssembly.dll, disassembles with capstone
5. Extracts ROL/XOR/ADD/SUB operations (handles SHL+SHR+OR ROL pattern, loop unrolling)
6. Matches against 6 known patterns (exact match → high confidence)
7. For unidentified ciphers, tries structural matching (op-type sequence only)
8. For still-missing decrypts, searches GameAssembly.dll binary for known seed constants
9. **Auto-generates encrypt functions** (inverse of decrypt: reverse order, ADD↔SUB, ROL→ROR, XOR stays)
10. Outputs `decrypt_algorithms.json` with: `algorithms` (6 decrypt), `encrypt_algorithms` (6 encrypt), `unidentified_ciphers` (for manual investigation)
11. Merges results into master.json

### Name collision fix (compare_and_patch.py)

`find_existing_offset()` now takes `allow_fallback` parameter. klass_rvas pass `allow_fallback=False` — TypeInfo pointers only match in the `"offsets"` namespace, never fall back to struct field namespaces. Additionally, a sanity check skips any patch where `new_value > 0x10000000` (pointer RVA) but `old_value < 0x10000` (struct field offset) — clearly a collision.

### One-click update workflow

```
1. Start Rust (join any server)
2. Run: tools\masterupdate\getnewoffsets.bat
3. Run: tools\masterupdate\update_now.bat
4. Inject and play
```

No Morphine web. No Frida (optional). No manual offset verification. `verify_pipeline.py` checks correctness automatically.

## What this is

External Rust cheat DLL injected via loader into host process. Reads/writes game memory through a kernel driver (`Bfo64` at `\\.\Bfo64` IOCTL). ImGui D3D11 overlay for menu. Target: **Rust (Steam) Unity 6 Il2Cpp** — not Mono, not older Unity.

## Architecture

| File | Role |
|------|------|
| `offsets.hpp` | Game offsets, feature toggle globals, color presets, `#define` constants |
| `sdk.hpp` | Decrypt functions, `Get_Position` (SSE transform chain), `Get_HeldItem`, `Get_Transformation` |
| `Driver.hpp` | `DriverInterface`, `read<T>`/`write<T>` templates (→ kernel IOCTL), `find_process`, `is_valid` |
| `Hacks/Cache/cache.cpp` | BN entity chain walk, entity classification, world prefab ESP, entity list population |
| `Hacks/Misc/misc.cpp` | Debug cam, NoRecoil, weapon features, FOV changer, BrightNight |
| `Hacks/Misc/spoofer.cpp` | Embedded HWID spoofer — 11 IOCTL spoofs via kernel driver, MessageBox prompt |
| `Hacks/Misc/cleaner.cpp` | Deep cleaner — 8-phase Rust HWID reset, interactive console menu |
| `Hacks/Aimbot/aimbot.cpp` | Memory aimbot (writes `bodyAngles` to `PlayerInput+0x44`) — runs in render thread |
| `Hacks/Visuals/visuals.cpp` | Box/Skeleton/Name/Distance/Health/Head ESP rendering, bone position anchoring |
| `Hacks/Cheat/Cheat.hpp` | `Do_Cheat()` main loop — reads camera matrix fresh, renders ESP, runs aimbot |
| `Render/render.hpp` | ImGui D3D11 overlay: `Hijack()`, `Draw()`, `Menu()` (10-page UI, BeaZt/Bomza/Better Cheats branded) |
| `Config.hpp` | Save/Load config (`C:\rustcfg.dat` / `C:\rustcfg_bomza.dat` / `C:\rustcfg_bc.dat`) — key=value format |
| `OffsetManager.hpp` | Decrypt config struct, `LoadDecryptConfig`/`SaveDecryptConfig`, embedded defaults |
| `Logger.hpp` | File logging (`C:\cheat_debug.log` BeaZt / `C:\cheat_debug_bomza.log` Bomza / `C:\cheat_debug_bc.log` Better Cheats) |
| `RuntimePaths.hpp` | Detects BeaZt/Bomza/Better Cheats variant by DLL filename, returns config/log/spoofer paths |
| `Rust Prv Ext.cpp` | `DllMain`, `MainThread`, init sequence, export functions (Main/Init/Run/etc.) |
| `tools/masterupdate/` | Auto-updater: morphine-dumper + capstone decrypt + optional sha-dumper/Frida, generates patches + verification + super prompt |
| `tools/morphine-dumper/` | **Primary source** — injected DLL: field offsets, static TypeInfos, decrypt fn_rvas, camera chain, model bones, GCHandle RVA, ALL assembly classes (auto-discovered), JSON output |
| `tools/masterupdate/disasm_decrypts.py` | **Authority for decrypts** — offline capstone disassembler, reads fn_rvas from master.json or standalone cipher scan, auto-generates encrypt functions (inverse of decrypt) |
| `tools/masterupdate/verify_pipeline.py` | End-to-end pipeline verification — checks decrypt/encrypt correctness, collision safety, fn_rva presence, outputs PASS/FAIL report |
| `tools/masterupdate/compare_and_patch.py` | Generates 5 patch files from master.json (name collision fix prevents klass_rvas overwriting field offsets) |
| `tools/sha-dumper/` | Optional cross-validation — injected DLL: scans GameAssembly for cipher sections, mesh data |

## Critical runtime files

| Path | Purpose | Notes |
|------|---------|-------|
| `C:\cheat_debug.log` / `C:\cheat_debug_bomza.log` / `C:\cheat_debug_bc.log` | Diagnostic log | BeaZt / Bomza / Better Cheats flavor-specific |
| `C:\rustcfg.dat` / `C:\rustcfg_bomza.dat` / `C:\rustcfg_bc.dat` | User settings | Saved/loaded via menu or `Config::Save()`/`Config::Load()` |
| `C:\rust_decrypts.dat` | Decrypt constants | Edit and reinject — **no recompile needed**. Auto-generated from embedded defaults if missing |

## Decrypt constant system

Three layers (highest priority wins):
1. **File**: `C:\rust_decrypts.dat` loaded at runtime via `OffsetManager::LoadDecryptConfig()`
2. **Embedded defaults**: `OffsetManager.hpp` lines 10-37 (used if file missing)
3. **Format**: key=value per line, e.g. `nk_rol=0x2\r\nnk_xor=0x111B9118\r\nnk_add=0x79300E2E\r\n`

File is locked while Rust is running — delete only when game is closed.

**Encrypt functions** use the same constants as their decrypt counterparts, just applied in reverse order with inverted operations (ADD→SUB, SUB→ADD, XOR→XOR, ROL→ROR). Auto-generated by both morphine-dumper (runtime) and disasm_decrypts.py (offline capstone). No separate encrypt constants file needed.

## BN chain (entity discovery)

```
GameAssembly + basenetworkable_pointer
  → TypeInfo → +0xB8 (static_fields) → +0x30 (wrapper_class)
    → [decrypt: networkable_key(nk_rol, nk_xor, nk_add)]
      → +0x10 (parent_static_fields)
        → [decrypt: networkable_key2(nk2_rol, nk2_add, nk2_xor)]
          → +0x18 (entity BufferList)
            → +0x18 = size (uint32), +0x10 = array ptr
              → +0x20 = LocalPlayer, +0x20 + i*8 = entity[i]
```

BN chain fails with `wrapper_class_ptr=0` for ~30 seconds after pressing HOME — this is **normal** while joining a server.

## DO NOT: Blindly trust UnknownCheats offsets

The v1mper UC dump (June 6 2026) is from a **different GameAssembly.dll build**. All BasePlayer offsets from that dump were wrong for our binary. The authoritative source is the morphine-dumper + capstone output at `tools/masterupdate/output/master.json` — always verify against it.

## Verified correct offsets (Build 24069519 — morphine desktop dump + UC confirmed)

**Static pointers**: `basenetworkable_pointer=0xFC410C0`, `camera_pointer=0xFC31910`, `Il2cppGetHandle=0x101045C0`, `TOD_Sky_TypeInfo=0xFC8BEA8`, `ConVar_Graphics=0xFC41900`, `convar_admin=0xFBEB120`

**BaseCamera**: `viewMatrix=0x2FC`, `projectionMatrix=0x18C`, `world_position=0x444`, `field_of_view=0x170`, `static_fields=0xB8`, `wrapper_class=0x30`, `entity=0x10`

**BasePlayer** (build 24069519): `PlayerEyes=0x340`, `PlayerInventory=0x318`, `PlayerInput=0x6F0`, `PlayerModel=0x2D8`, `PlayerFlags=0x6B8`, `ClActiveItem=0x568`, `BaseMovement=0x300`, `DisplayName=0x710`, `ModelState=0x3D8`, `Mounted=0x5C0`, `CurrentTeam=0x538`

**PlayerInventory**: `Belt=0x60` (containerBelt), `Wear=0x78` (containerWear), `Main=0x28` (containerMain), `loot=0x48`

**ItemContainer**: `ItemList=0x28`, `ItemListFallback=0x10`

**Item** (build 24069519): `ItemDefinition=0x80` (itemdefinition), `ItemId=0xC8` (uid), `HeldEntity_1=0x98` (held_entity), `Amount=0xB8`

**ItemDefinition**: `shortname=0x28`, `itemid=0x20`, `displayName=0x40`

**ModelState**: `flags=0x3C`

**PlayerModel**: `velocity=0x368` (newVelocity), `position=0x2F8`, `boneTransforms=0x98`, `visible=0xC4`

**BaseEntity**: `positionLerp=0x1E0`, `bounds=0x17C`, `flags=0x1B0`, `isVisible=0x150`, `children=0x70`

**BaseCombatEntity**: `lifestate=0x298`, `Health=0x2A4`, `MaxHealth=0x2A8`

**BaseNetworkable**: `static_fields=0xB8`, `wrapper_class=0x8`, `parent_static_fields=0x10`, `entity=0x20`, `children=0x70`

**BN chain**: `static_fields=0xB8`, `wrapper_class=0x8`, `parent_static_fields=0x10`, `entity=0x20`

**List<T> layout**: `list+0x10` = array ptr, `list+0x18` = size, `array+0x20+i*8` = element[i]

### Decrypt functions (build 24069519 — UC confirmed)

| Function | Operation sequence | Source |
|----------|-------------------|--------|
| `networkable_key` | ROL(0xB) → XOR(0xBCDFA6C7) → ADD(0x1A88518) | Morphine + UC (6 sources) |
| `networkable_key2` | ADD(0x4016C175) → ROL(0x1D) → ADD(0x7D75A2B0) → XOR(0x97FF1778) | Morphine + UC (5 sources) |
| `cl_active_item` | ROL(27) → ADD(0x56427D52) → ROL(8) | UC (3 sources + x86 disasm proof) |
| `decrypt_inventory_pointer` | ADD(0x86F83B72) → ROL(0x15) → ADD(0x41069A6F) | Morphine website |
| `decrypt_eyes` | ROL(0x5) → ADD(0x487FBCF7) → XOR(0xE70F1737) | Morphine website |
| `decrypt_fov` | ADD(0xF7ED7C0) → ROL(0x12) → SUB(0xA557A4EC) → XOR(0xD6A6E25E) | Morphine website |

### Encrypt functions (auto-generated — inverse of decrypt)

All 6 encrypt functions are auto-generated by morphine-dumper and disasm_decrypts.py as the exact inverse of their decrypt counterparts (reverse op order, ADD↔SUB, ROL→ROR, XOR stays XOR):

| Function | Operation sequence |
|----------|-------------------|
| `encrypt_networkable_key` | XOR(0xBCDFA6C7) → SUB(0x1A88518) → ROR(0xB) |
| `encrypt_networkable_key2` | XOR(0x97FF1778) → SUB(0x7D75A2B0) → ROR(0x1D) → SUB(0x4016C175) |
| `encrypt_cl_active_item` | ROR(8) → SUB(0x56427D52) → ROR(27) |
| `encrypt_inventory_pointer` | SUB(0x41069A6F) → ROR(0x15) → SUB(0x86F83B72) |
| `encrypt_eyes` | XOR(0xE70F1737) → SUB(0x487FBCF7) → ROR(0x5) |
| `encrypt_fov` | XOR(0xD6A6E25E) → ADD(0xA557A4EC) → ROR(0x12) → SUB(0xF7ED7C0) |

## GameAssembly.dll module finding (EAC bypass)

**Critical**: `get_module()` in `Driver.hpp` has a multi-stage fallback for finding `GameAssembly.dll`:

1. **OpenProcess + NtQueryVirtualMemory** (usermode, primary) — works when EAC hasn't fully initialized
2. **PE header scan via `ReadMemory_Raw`** (kernel driver, fallback) — bypasses ALL EAC hooks

**Initialization order** (MUST be this order):
```
find_process → GetProcessBase → GetCR3_1 → get_module(GameAssembly) → OffsetManager::Initialize → get_module(UnityPlayer)
```

`GetCR3_1` MUST be called BEFORE `get_module` — the PE scan uses `ReadMemory_Raw` which requires CR3 to translate virtual addresses.

**PE scan details** (`ReadMemory_Raw` with `noncachedread=false`):
- `ReadMemory_ACE`/`WriteMemory_ACE` use `noncachedread=true` (direct physical read) for ALL gameplay reads — 1ms per IOCTL, no BSOD risk
- `ReadMemory_Raw` uses `noncachedread=false` (MmCopyVirtualMemory) — ONLY for one-time PE scan at startup, never during gameplay
- **DO NOT** use hybrid noncachedread (address-based true/false switching) — it causes BSODs and 7x slower IOCTLs
- UC (Lox0n, 33k rep): "don't use MmCopyVirtualMemory at all" for ESP-level read frequency
- GameAssembly.dll can be loaded 5-15GB above process_base due to ASLR
- Scan covers 0x7FF000000000 to 0x7FFF00000000 (entire system DLL load region)
- 3-stage binary search: coarse (1GB steps) → fine (64KB steps) → wide (process_base + 32GB)

**DLL import table restrictions** (EAC detection):
- **NEVER** import `winmm.dll` — EAC detects it and blocks `OpenProcess` from that process
- **NEVER** call `timeBeginPeriod()` / `timeEndPeriod()` — forces winmm.dll load
- **NEVER** use `CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)` — triggers EAC detection
- **NEVER** create files with "cheat" in the name on `C:\` before get_module succeeds
- `d3dx11_43.dll` and `D3DCOMPILER_43.dll` imports are acceptable (used by overlay/logo loading)

## Performance architecture

**IOCTL optimization** (build 24087225+):
- **No IOCTL mutex** — all threads read from the driver concurrently (SHA source approach, never BSODs)
- **noncachedread=true for ALL reads** — direct physical memory read, ~1ms per IOCTL (was 7ms with MmCopyVirtualMemory hybrid mode). Hybrid mode caused BSODs + ESP dragging (10s stale positions + 10 FPS render loop)
- **Entity cap**: 3000 (was 8000) — saves ~40,000 IOCTLs per cache cycle
- **Cross-cycle pointer caching**: entity handles (Il2cppGetHandle results), object pointers, objectClass pointers cached across cycles — saves 7 IOCTLs/entity (5 from Il2cppGetHandle + 1 object + 1 objectClass). Caches cleared on `markUnavailable()` or when exceeding 500 entries.
- **Early distance cull**: Position read BEFORE classification — skips far entities before spending 5+ IOCTLs on object/class/name reads
- **Block reads**: Player combat data (lifestate + health + maxHealth) read in 1 IOCTL (was 3)
- **Block reads**: Visibility flags (3 bytes at 0x150) read in 1 IOCTL (was 3)
- **Prefab name caching**: `s_prefabNameCache` caches prefab names for 10 cycles per objectClass pointer

**Render loop** (matches SHA source + UC consensus):
- **No frame limiter** — Present() governs frame rate naturally
- **No thread priorities** — all threads run at default priority
- **No worker yields** — cache/position/skeleton threads run at full speed
- **WorldToScreen**: Clamps `w` to 0.0001f when behind camera (never returns invalid — prevents edge popping)
- **Camera matrix read fresh in render thread** — UC consensus (adamthaok): "Read matrix, W2S, draw, all in one go. Dont be reading it in a background thread then drawing later." Camera matrix is 1 cheap IOCTL, MUST be fresh to prevent ESP swimming. Entity data is cached in background threads.

**Overlay**:
- Overlay window synced to game window position every frame (prevents ESP misalignment in windowed mode)
- Camera matrix read fresh in render thread at start of `Do_Cheat()`

## Inventory Resolution (ResolvePlayerInventory)

PlayerInventory at `BasePlayer+0x2F0` is a **wrapper struct pointer**, NOT the inventory pointer directly. The correct resolution chain is:

```
BasePlayer + 0x2F0 → inv_raw (wrapper struct pointer)
inv_raw + 0x18     → encrypted GCHandle
decrypt_inventory_pointer(enc_handle) → decrypted GCHandle
Il2cppGetHandle(dec_handle) → PlayerInventory pointer
```

This is the SAME pattern as `networkable_key` / `networkable_key2` in the BN chain. `ResolvePlayerInventory()` in `sdk.hpp` tries this first, then falls back to `GetPlayerInventory()` (component walker). The method is cached via `static int invMethod`. Do NOT change this to direct decrypt or raw pointer — it will break held item resolution.

`Get_HeldItem()`, `Get_Hotbar_list()`, and `cache.cpp` wear read all use `ResolvePlayerInventory()` + multi-belt-offset fallback (tries Belt=0x30, then 0x60, then 0x28). `Get_HeldWeapon()` uses `ResolvePlayerInventory()` + belt approach (PASS 0), then children list UID match (PASS 1, returns on match without requiring valid recoil).

## Camera Matrix (Render Thread — Fresh Read)

The camera/view matrix is read **FRESH in the render thread** (`ReadFrameCameraMatrix()` at the start of `Do_Cheat()` in `Cheat.hpp`). This eliminates ESP swimming/lag — the matrix is always current when WorldToScreen is called. The position thread no longer reads the camera matrix (saves 7 IOCTLs per position cycle).

This matches the approach used by the SHA source (Timeless Rust) and VolkRust — both read the camera matrix fresh in the render thread.

## Aimbot (Render Thread — No Separate Thread)

The aimbot runs **inside the render thread** at the end of `Do_Cheat()` (after ESP rendering). This eliminates the race condition that caused 0xC0000005 crashes when `src->LocalPlayer` became stale during BN chain failures. The render thread validates `src->LocalPlayer` at the start, so the aimbot always uses a fresh, validated pointer.

## Thread Architecture

| Thread | Sleep | Cycle | Purpose |
|--------|-------|-------|---------|
| Cache | 1ms + 100ms target | ~100ms | Entity list + classification + inventory scan |
| Position | 1ms | ~9ms | Player positions + velocities (no camera) |
| Skeleton | 1ms | ~30ms | 15 bones, 5 players/iteration, TTL cached |
| Misc | 1ms | ~10ms | BrightNight, FOV, debug cam |
| Render | vsync | ~16ms | ESP + aimbot (fresh camera matrix each frame) |

## Skeleton ESP (Bone Caching with TTL)

- All 15 bones read per player, cached with per-player TTL
- TTL by distance: ≤20m → 50ms, ≤100m → 100ms, ≤300m → 200ms
- Staggered refresh: max 5 players per skeleton iteration (prevents IOCTL burst)
- Bone position anchoring: `boneOffset = currentHeadPos - boneAnchorPos` shifts all bones to current position
- `boneAnchorPos` stored in `EspCacheData` when bones are read
- Render thread applies offset in `do_Visuals()` before drawing skeleton

## IOCTL Stability

- `ioctl_fail_count` is cumulative (failures +1, successes -1). Time-based decay: every 500ms, subtracts 50 (floor at 0).
- `IOCTL_FAIL_LIMIT=500` — blocks all reads when reached (no `g_process_dead` requirement).
- `ioctl_blocked` auto-resets after 500ms.
- `markUnavailable()` in `cache.cpp` checks `ioctl_blocked` first — during transient blocks, it preserves existing entity lists and cache instead of clearing them. ESP continues rendering from last-known-good data.
- `find_process` requires 3 consecutive failures before shutdown.

## Menu: anti-pattern to avoid

**Never** wrap `ImGui::Checkbox` in style push/pop lambdas. The old `Danger()`/`Warning()`/`Normal()` pattern pushed `ImGuiCol_Text` before the checkbox and popped after — this corrupted the style stack in nested child windows, causing clicks to not register. Use a post-render badged label instead.

## VisCheck (ENABLED — build 24069519)

**Status**: VisCheck is **enabled** using Unity's built-in visibility flag at `BaseEntity+0x150`.

**How it works**:
- Reads `isVisible` (1 byte) at `BaseEntity+0x150` — Unity's occlusion culling flag
- VisMode=1 (Hybrid): uses 3-sample consensus with simple majority threshold
- VisHoldMs=0: no sticky visible state — instant transition
- No PhysX/BVH raycasting needed — pure flag read (1 IOCTL per player)

**Color coding**: Players behind walls shown in invisible color, visible players in normal color.

**Settings** (`offsets.hpp`): `VisMode=1`, `VisSamples=3`, `VisHoldMs=0`

## Constraints

- **External cheat**: all game memory reads through `read<T>(address)` → kernel driver IOCTL. Never direct memory access.
- Feature toggles are `inline` globals in `offsets.hpp` — no thread sync, but UI and cheat logic share the same thread.
- `C:\rust_decrypts.dat` is locked while game runs; cannot delete/edit unless game is closed.
- DLL output, not EXE; injected via loader.
- There are no tests, typecheck, or lint scripts. The only CI step is the MSBuild compile.
- **Camera matrix is read FRESH in the render thread** (`ReadFrameCameraMatrix()` in `Do_Cheat()`). Position thread does NOT read camera matrix. This eliminates ESP swimming.
- **Aimbot runs in the render thread** (not a separate thread). Eliminates race condition with cache thread that caused 0xC0000005 crashes.
- **Skeleton thread caps at 5 players per iteration** to prevent IOCTL burst when all bone caches expire simultaneously.
- **Animal `IsDead()` bug**: `BaseCombatEntity::lifestate` is an `int` (0=Alive, 1=Dead, 2=Sleeping). `IsDead()` reads it as `bool` (any non-zero = true), which incorrectly marks sleeping bears (lifestate=2) as dead. Animal classification reads `lifestate` as `int` and only skips if `== 1`.
- **sha-dumper camera scan now uses structural validation**: The `[native_camera]` scan uses matrix structure patterns (view matrix last row [0,0,0,1], projection matrix m[11]=-1/m[15]=0) instead of first-match heuristics. When `validated>=1`, offsets are auto-applied. When `validated=0`, falls back to UC-confirmed values. Camera chain offsets (static_fields=0xB8, wrapper_class=0x90, entity=0x10) are always safe to auto-patch.
- **Frida cross-validation**: `cross_validate_offsets.py` builds a mapping `{field_name → frida_hash}` by matching current offset values. Since Frida hashes are name-based (not offset-based), the mapping stays valid across game updates. When the game updates, the script finds each hash in the new Frida dump and detects offset changes automatically → generates `frida_patches.txt`.

## Debugging workflow

- Always inspect existing patterns before editing. Prefer small, targeted changes.
- Identify the root cause before changing files.
- After edits, run the build. Use errors as debugging input.
- The only verification step is building successfully. Then inject and check the flavor log (`C:\cheat_debug_lite.log` or `C:\cheat_debug_private.log`).
- Verify ESP renders entities (not just LocalPlayer) before claiming success.
- Summarize root cause, files changed, and remaining risks.

