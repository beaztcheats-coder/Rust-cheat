# AGENTS.md — Rust External Cheat

## Build

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\Rust-cheat\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName=<name> /m
```

<<<<<<< HEAD
- VS2022 vcxproj, C++20, x64 Release DLL, llvm-msvc compiler
- Requires DirectX SDK (included in repo under `Microsoft DirectX SDK/`)

=======
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

>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6
### Build all 4 flavors

| Flavor | TargetName | Output DLL | Logging | Menu |
|--------|-----------|-----------|---------|------|
| **BeaZt debug** | `Rust Prv Ext_debug` | `Rust Prv Ext_debug.dll` | Auto-enabled | BeaZt |
| **BeaZt prod** | `Rust Prv Ext` | `Rust Prv Ext.dll` | Off (marker to enable) | BeaZt |
| **Bomza prod** | `bomzarust` | `bomzarust.dll` | Off (marker to enable) | Bomza |
| **Better Cheats** | `bettercheats` | `bettercheats.dll` | Off (marker to enable) | Better Cheats |

```
/p:TargetName="Rust Prv Ext_debug"   /p:TargetName="Rust Prv Ext"   /p:TargetName=bomzarust   /p:TargetName=bettercheats
```

### Debug logging

- **Debug build**: Logging auto-enabled (DLL name contains `_debug`)
- **Production**: Disabled. Enable via `C:\rust_debug_enabled.txt` marker file
- **Log paths**: `%TEMP%\cheat_debug.log` (BeaZt) / `cheat_debug_bomza.log` (Bomza) / `cheat_debug_bc.log` (Better Cheats)
- **Logger filter**: `Logger.hpp` filters out verbose diagnostic messages — only important messages (FAIL, CRASH, SKIP, FATAL, Setup, BN chain FAIL, CAM_CHAIN, Do_Cheat, Shutdown) appear in the log
- Production builds delete ALL trace files on shutdown (logs, configs, markers, decrypts)

<<<<<<< HEAD
---

## Current Game Build: 24091435

All offsets and decrypt constants verified against morphine dump (https://offsets.getmorphine.fun/offsets.hpp) for build 24091435. GameAssembly timestamp: `0x6A4CF525`.

### Static pointers (GameAssembly RVAs)

| Pointer | RVA | Notes |
|---------|-----|-------|
| basenetworkable_pointer | 0xFC72970 | BaseNetworkable TypeInfo |
| camera_pointer | 0xFC68100 | MainCamera TypeInfo |
| Il2cppGetHandle | 0x10132020 | gc_handles::array_rva |
| TOD_Sky_TypeInfo | 0xFD0A5D0 | TOD_Sky Static TypeInfo |
| EffectNetwork_Pointer | 0xFCAC040 | static_type_info |
| ConVar_Graphics | 0xFCF2C10 | convar_graphics |
| convar_admin | 0xFC816A8 | |

### Field offsets

| Class | Field | Offset |
|-------|-------|--------|
| BaseNetworkable | static_fields | 0xB8 |
| BaseNetworkable | wrapper_class | 0x8 |
| BaseNetworkable | parent_static_fields | 0x10 |
| BaseNetworkable | entity | 0x20 |
| BaseNetworkable | children | 0x70 |
| BaseCamera | static_fields | 0xB8 |
| BaseCamera | wrapper_class | 0x38 |
| BaseCamera | entity | 0x10 |
| BaseCamera | viewMatrix | 0x2FC |
| BaseCamera | projectionMatrix | 0x18C |
| BaseCamera | world_position | 0x444 |
| BaseCamera | field_of_view | 0x170 |
| BasePlayer | PlayerEyes | 0x490 |
| BasePlayer | PlayerInventory | 0x3A0 |
| BasePlayer | PlayerInput | 0x338 |
| BasePlayer | PlayerModel | 0x3D8 |
| BasePlayer | PlayerFlags | 0x6B8 |
| BasePlayer | ClActiveItem | 0x568 |
| BasePlayer | BaseMovement | 0x5B8 |
| BasePlayer | DisplayName | 0x2E8 |
| BasePlayer | ModelState | 0x518 |
| BasePlayer | Mounted | 0x5C0 |
| BasePlayer | CurrentTeam | 0x538 |
| BaseCombatEntity | Lifestate | 0x298 |
| BaseCombatEntity | Health | 0x2A4 |
| BaseCombatEntity | MaxHealth | 0x2A8 |
| BaseEntity | positionLerp | 0x1C8 |
| BaseEntity | bounds | 0x17C |
| BaseEntity | flags | 0x1B0 |
| BaseEntity | isVisible | 0x150 |
| PlayerInventory | Belt | 0x58 |
| PlayerInventory | Wear | 0x78 |
| PlayerInventory | Main | 0x30 |
| PlayerInventory | loot | 0x48 |
| ItemContainer | ItemList | 0x60 |
| Item | ItemDefinition | 0xA8 |
| Item | ItemId | 0xB8 |
| Item | HeldEntity_1 | 0xA8 |
| Item | Amount | 0x24 |
| ItemDefinition | itemid | 0x20 |
| ItemDefinition | shortname | 0x28 |
| ItemDefinition | displayName | 0x40 |
| PlayerModel | position | 0x2F8 |
| PlayerModel | velocity | 0x31C |
| PlayerModel | visible | 0xC4 |
| PlayerModel | isNpc | 0x490 |
| PlayerModel | boneTransforms | 0x98 |
| TOD_Sky_Static | instances | 0x8 |
| TOD_Sky | NightParameters | 0x60 |
| TOD_Sky | AmbientParameters | 0x98 |
| TOD_NightParameters | AmbientMultiplier | 0x5C |
| PlayerInput | bodyAngles | 0x44 |
| ModelState | flags | 0x0 |
| ModelState | Ducked | 0x10 |
| BaseNetworkable2 | prefab_id | 0x54 |
=======
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
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

### Decrypt constants (build 24091435 — morphine confirmed)

<<<<<<< HEAD
All constants in `OffsetManager.hpp`. Can be overridden at runtime via `%TEMP%\rust_decrypts.dat`.
=======
| Source | Provides | Requires Game? | Status |
|--------|----------|---------------|--------|
| **morphine-dumper** (injected DLL) | Field offsets, static TypeInfos, ALL decrypt fn_rvas, camera chain (Unity method oracles), model bones, GCHandle RVA, FOV encrypt/decrypt, ALL assembly classes (auto-discovered) | Yes (game running) | Working — **PRIMARY SOURCE** |
| **disasm_decrypts.py** (offline capstone) | Decrypt + encrypt algorithms (ROL/XOR/ADD/SUB chains) — reads fn_rvas from master.json or scans GameAssembly.dll .text section | No (offline) | Working — **AUTHORITY for decrypts** |
| **sha-dumper** (injected DLL) | Cipher section enumeration (930+ sections), mesh data, cross-validation | Yes (game running) | Working — **optional cross-validation** |
| **Frida** (runtime dump) | Field offsets, field types, method RVAs, cross-validation | Yes (game running) | Working — **optional cross-validation** |
| **Il2CppInspectorPro** (offline) | Field offsets, TypeInfo RVAs, method RVAs | No | Broken (.NET 10 preview required) |
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

| Function | Operations |
|----------|-----------|
| networkable_key | ADD(0x16A2149) → ROL(7) → ADD(0x42D649F) |
| networkable_key2 | ROL(0x14) → SUB(0x1CC81122) → XOR(0xAACADDEB) → ADD(0x63DFF63C) |
| cl_active_item | XOR(0x320F4C7D) → ROL(0x1F) → ADD(0x62E951E6) |
| inventory_pointer | ROL(7) → XOR(0xEC9526A3) → ADD(0xAC5D1D91) → ROL(0xF) |
| eyes | SUB(0x2C865414) → XOR(0x873FAFE5) → ADD(0x3FD966E1) → ROL(6) |
| fov | ADD(0xCA34BF6D) → ROL(0x1D) → SUB(0xB723318) |

<<<<<<< HEAD
---
=======
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
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

## Architecture

External Rust cheat DLL injected via loader into host process. Reads/writes game memory through kernel driver (`Bfo64` at `\\.\Bfo64` IOCTL). ImGui D3D11 overlay for menu. Target: **Rust (Steam) Unity 6 Il2Cpp**.

### File roles

| File | Role |
|------|------|
<<<<<<< HEAD
| `offsets.hpp` | Game offsets, feature toggle globals, color presets |
| `sdk.hpp` | Decrypt functions, Get_Position (SSE transform chain), Get_HeldItem, ResolvePlayerInventory |
| `Driver.hpp` | DriverInterface, read<T>/write<T> templates, get_module (PE scan fallback), find_process |
| `Hacks/Cache/cache.cpp` | BN entity chain walk, entity classification (player/NPC/animal/world), prefab name caching, world prefab ESP |
| `Hacks/Misc/misc.cpp` | NoRecoil, weapon features, FOV changer, BrightNight, TimeChanger, debug cam, AdminFlags |
| `Hacks/Aimbot/aimbot.cpp` | Memory aimbot (writes bodyAngles to PlayerInput+0x44) — runs in render thread |
| `Hacks/Visuals/visuals.cpp` | Box/Skeleton/Name/Distance/Health ESP rendering |
| `Hacks/Cheat/Cheat.hpp` | Do_Cheat() main loop — reads camera matrix fresh, renders ESP, runs aimbot |
| `Render/render.hpp` | ImGui D3D11 overlay: Hijack(), Draw(), Menu() |
| `Config.hpp` | Save/Load config, ApplyLegitDefaults() |
| `OffsetManager.hpp` | Decrypt config struct, LoadDecryptConfig, embedded defaults |
| `Logger.hpp` | File logging with verbose message filter |
| `RuntimePaths.hpp` | Detects flavor by DLL filename (PE header fallback for manually mapped DLLs) |
| `Rust Prv Ext.cpp` | DllMain, MainThreadImpl, init sequence, VEH crash handler |
| `vectorMath.hpp` | WorldToScreen, g_ScreenW/g_ScreenH globals |
=======
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
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

### Thread architecture

| Thread | Sleep | Purpose |
|--------|-------|---------|
| Cache | 1ms + 100ms target | Entity list + classification + inventory scan + world prefab ESP |
| Position | 1ms | Player positions + velocities + local player neck/crouch state |
| Skeleton | 1ms | 15 bones, 5 players/iteration, TTL cached |
| Misc | 1ms | BrightNight, FOV, debug cam |
| Render | vsync | ESP + aimbot (fresh camera matrix each frame) |

<<<<<<< HEAD
### Init sequence (MUST be this order)
=======
## Decrypt constant system

Three layers (highest priority wins):
1. **File**: `C:\rust_decrypts.dat` loaded at runtime via `OffsetManager::LoadDecryptConfig()`
2. **Embedded defaults**: `OffsetManager.hpp` lines 10-37 (used if file missing)
3. **Format**: key=value per line, e.g. `nk_rol=0x2\r\nnk_xor=0x111B9118\r\nnk_add=0x79300E2E\r\n`

File is locked while Rust is running — delete only when game is closed.

**Encrypt functions** use the same constants as their decrypt counterparts, just applied in reverse order with inverted operations (ADD→SUB, SUB→ADD, XOR→XOR, ROL→ROR). Auto-generated by both morphine-dumper (runtime) and disasm_decrypts.py (offline capstone). No separate encrypt constants file needed.

## BN chain (entity discovery)
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

```
MainThreadImpl()
  → VEH handler
  → DPI awareness (SetProcessDpiAwarenessContext — fails silently, safe)
  → Temp file cleanup
  → Console + banner
  → Logging setup
  → Spoofer prompt (BeaZt/Bomza only)
  → find_process → GetProcessBase → GetCR3_1 → get_module(GameAssembly)
  → OffsetManager::Initialize → get_module(UnityPlayer)
  → Start cache/position/skeleton workers
  → Render loop (overlay + Do_Cheat)
```

---

## Driver (Bfo64)

<<<<<<< HEAD
### Read methods

- **ReadMemory_ACE** — used for ALL gameplay reads (ESP, aimbot, cache). `noncachedread=true` (direct physical read). ~1ms per IOCTL.
- **ReadMemory_Raw** — used ONLY for one-time PE scan at startup (get_module). Same IOCTL but bypasses fail-counting/blocking logic.
- **WriteMemory_ACE** — used for all writes (bodyAngles, recoil, FOV, etc.). `noncachedread=true`.

### get_module (GameAssembly.dll finding)

1. **OpenProcess + NtQueryVirtualMemory** (usermode, primary)
2. **PE header scan via ReadMemory_Raw** (kernel driver, fallback) — scans 0x7FF000000000 to 0x7FFF00000000, identifies by timestamp 0x6A4CF525 or image size >0x10000000

### IOCTL stability

- `ioctl_fail_count`: cumulative (failures +1, successes -1). Decays by 50 every 500ms.
- `IOCTL_FAIL_LIMIT=500` — blocks all reads when reached
- `ioctl_blocked` auto-resets after 500ms
- Entity cap: 8000

---

## EAC Restrictions — CRITICAL
=======
The v1mper UC dump (June 6 2026) is from a **different GameAssembly.dll build**. All BasePlayer offsets from that dump were wrong for our binary. The authoritative source is the morphine-dumper + capstone output at `tools/masterupdate/output/master.json` — always verify against it.

## Verified correct offsets (Build 24069519 — morphine desktop dump + UC confirmed)

**Static pointers**: `basenetworkable_pointer=0xFC410C0`, `camera_pointer=0xFC31910`, `Il2cppGetHandle=0x101045C0`, `TOD_Sky_TypeInfo=0xFC8BEA8`, `ConVar_Graphics=0xFC41900`, `convar_admin=0xFBEB120`

**BaseCamera**: `viewMatrix=0x2FC`, `projectionMatrix=0x18C`, `world_position=0x444`, `field_of_view=0x170`, `static_fields=0xB8`, `wrapper_class=0x30`, `entity=0x10`

**BasePlayer** (build 24069519): `PlayerEyes=0x340`, `PlayerInventory=0x318`, `PlayerInput=0x6F0`, `PlayerModel=0x2D8`, `PlayerFlags=0x6B8`, `ClActiveItem=0x568`, `BaseMovement=0x300`, `DisplayName=0x710`, `ModelState=0x3D8`, `Mounted=0x5C0`, `CurrentTeam=0x538`

**PlayerInventory**: `Belt=0x60` (containerBelt), `Wear=0x78` (containerWear), `Main=0x28` (containerMain), `loot=0x48`

**ItemContainer**: `ItemList=0x28`, `ItemListFallback=0x10`

**Item** (build 24069519): `ItemDefinition=0x80` (itemdefinition), `ItemId=0xC8` (uid), `HeldEntity_1=0x98` (held_entity), `Amount=0xB8`
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

**NEVER do any of these before `get_module()` succeeds:**

<<<<<<< HEAD
| Restricted API | Why | Effect |
|---------------|-----|--------|
| `LoadLibraryW(L"shcore.dll")` | New DLL load detected by EAC | Blocks OpenProcess → "GameAssembly.dll not found" |
| `SetProcessDpiAwareness()` | Process-wide state change | Same EAC trigger |
| `SetProcessDPIAware()` | Process-wide state change | Same EAC trigger |
| `SetThreadDpiAwarenessContext()` | Succeeds in injected DLL (unlike SetProcessDpiAwarenessContext) | DPI state change triggers EAC → blocks OpenProcess |
| `winmm.dll` import | EAC detects it | Blocks OpenProcess |
| `timeBeginPeriod()` / `timeEndPeriod()` | Forces winmm.dll load | Same as above |
| `CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)` | Triggers EAC detection | Blocks OpenProcess |
| Files with "cheat" in name on C:\ | EAC file scan | Detection |
=======
**ModelState**: `flags=0x3C`

**PlayerModel**: `velocity=0x368` (newVelocity), `position=0x2F8`, `boneTransforms=0x98`, `visible=0xC4`

**BaseEntity**: `positionLerp=0x1E0`, `bounds=0x17C`, `flags=0x1B0`, `isVisible=0x150`, `children=0x70`

**BaseCombatEntity**: `lifestate=0x298`, `Health=0x2A4`, `MaxHealth=0x2A8`

**BaseNetworkable**: `static_fields=0xB8`, `wrapper_class=0x8`, `parent_static_fields=0x10`, `entity=0x20`, `children=0x70`

**BN chain**: `static_fields=0xB8`, `wrapper_class=0x8`, `parent_static_fields=0x10`, `entity=0x20`
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

**DPI awareness**: Use ONLY `SetProcessDpiAwarenessContext((HANDLE)-4)`. It fails silently with `ERROR_ACCESS_DENIED` in injected DLLs (host process already set DPI awareness). This is SAFE — no DPI change, no EAC trigger. Overlay fixes (swap chain ResizeBuffers, ScreenToClient, MonitorFromWindow) still work without DPI awareness.

<<<<<<< HEAD
**"GameAssembly.dll not found" troubleshooting**: Check for ANY restricted API being called before `get_module()` in the init sequence. Any DLL load or process-wide state change between DPI awareness and `get_module()` can trigger EAC.

---

## Overlay (render.hpp)

### Key fixes applied

1. **Mouse position**: `ScreenToClient(window_handle, &p_cursor)` converts screen coords to window-relative — fixes click offset when overlay is not at (0,0)
2. **No per-frame HWND_TOPMOST**: Removed `SetWindowPos(HWND_TOPMOST)` at end of Draw(). Topmost re-asserted every 60 frames only — prevents z-fighting with Discord/Steam overlays
3. **SWP_FRAMECHANGED**: Added after WS_EX_TRANSPARENT toggle — ensures click-through change takes effect immediately
4. **Swap chain ResizeBuffers**: When overlay window dimensions change, `ResizeBuffers()` + recreate render target view — fixes blank/invisible overlay on some PCs
5. **ShowWindow(SW_SHOW)**: Replaced `SW_SHOWDEFAULT` (which uses host process STARTUPINFO) with `SW_SHOW`
6. **MonitorFromWindow**: Overlay created on the monitor the game window is on — fixes multi-monitor
7. **Fullscreen → borderless**: If exclusive fullscreen detected, forces game window to `WS_POPUP` (borderless) — DWM must be active for overlay to render
8. **WS_EX_NOACTIVATE**: Window never gets focus. Mouse input via `GetAsyncKeyState`, keyboard via `GetAsyncKeyState` + `ToAscii`

### Screen dimensions

- `g_ScreenW`/`g_ScreenH` (vectorMath.hpp) — updated every frame from `GetClientRect(game_window)`
- Used by WorldToScreen AND ScreenPointValid (visuals.cpp, Cheat.hpp) — single consistent source
- `io.DisplaySize` from ImGui_ImplWin32_NewFrame (GetClientRect of overlay window) — matches g_ScreenW/H when overlay is synced to game

---

## WorldToScreen (vectorMath.hpp)

```cpp
if (w < 0.098f) return Vector2(-1, -1);  // discard behind-camera entities
=======
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
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6
```

Returns `(-1, -1)` for entities behind camera or too close to near-clip plane. This prevents behind-camera entities from projecting to extreme screen positions ("ESP in the sky" bug). CCODIX approach — safer than clamping w to 0.0001f (SHA source approach).

<<<<<<< HEAD
---

## Cache (cache.cpp)

### Entity classification flow

All entities from BN chain go through:
1. Read prefab name from `objectClass + 0x50` (full prefab path like "assets/prefabs/plants/hemp/hemp.entity.prefab")
2. Classify: player (keyword + Il2CppClass fallback) → NPC (keyword) → animal (keyword) → world (else)
3. World entities matched by prefab name substring patterns

### Prefab name cache
=======
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
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

- **Keyed by Il2CppClass pointer** (entity offset 0x00) — shared per entity TYPE, not per instance. 100 hemp = 1 cache entry.
- **1024 slots**, reset each cache cycle
- Entries expire after 10 cycles
- Fallback: `objectClass` (GameObject pointer) if klass read fails

### World ESP prefab patterns (build 24091435 verified)

<<<<<<< HEAD
Patterns match substrings in lowercased full prefab paths. Key patterns:

| Entity | Pattern(s) | Default |
|--------|-----------|---------|
| Hemp | `hemp.entity.prefab`, `hemp-collectable.prefab` | ON |
| Stone ore | `ore_stone`, `stone-ore.prefab` | ON |
| Metal ore | `ore_metal`, `metal-ore.prefab` | ON |
| Sulfur ore | `ore_sulfur`, `sulfur-ore.prefab` | ON |
| Stash | `small_stash_deployed.prefab` | ON |
| Backpack | `item_drop_backpack.prefab` | ON |
| Turret | `autoturret_deployed`, `ballista`, `catapult` | ON |
| Shotgun trap | `guntrap` | ON |
| Flame turret | `flameturret` | ON |
| SAM site | `sam_site` | ON |
| Bear trap | `beartrap` | ON |
| Landmine | `landmine` | ON |
| Cargo ship | `cargoship` | OFF |
| Locked crate | `codelockedhackablecrate` | OFF |
| Barrels | `loot_barrel_1`, `loot_barrel_2`, `oil_barrel` | OFF |
| Crates | `crate_normal`, `crate_elite`, `crate_tools` | OFF |
| Vehicles | `rowboat`, `rhib`, `kayak`, `tugboat`, `minicopter`, `scraptransportporthelicopter`, `attackhelicopter`, `hotairballoon`, `motorbike`, `snowmobile`, `modularcar` | OFF |
| TC | `cupboard.tool` | OFF |
| Vending | `vending` | OFF |
| Workbench | `workbench1/2/3.deployed`, `recycler`, `researchtable`, `repairbench`, `mixingtable`, `refinery`, `cookingworkbench` | OFF |
| Lockers | `locker.deployed.prefab` | OFF |
| Sleeping bag | `sleepingbag` | OFF |
| Bed | `bed_deployed` | OFF |
| Large storage | `storage_barrel_`, `box.wooden.large`, `coffinstorage`, `woodbox_deployed` | OFF |

---

## Aimbot (aimbot.cpp)

- Runs in render thread (not separate thread) — eliminates race condition crashes
- Writes bodyAngles to `PlayerInput + 0x44`
- Bone selection: 0=Head, 1=Neck, 2=Chest, 3=Pelvis, 4=ClosestToCrosshair, 5=Smart
- **Crouch fix**: Position thread always reads local player's neck transform + crouch state (ModelState::Ducked). Aimbot fallback uses `g_LocalPlayerPos + (crouching ? 0.9f : 1.5f)` instead of hardcoded 1.5f
- **Silent Aim removed** from all menus and aimbot code. `AIMBOT::Silent` still exists as `false` default but is dead code
- Uses cached entity data (zero IOCTLs for target acquisition) + fresh camera matrix for angle calculation

---
=======
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
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6

## Debugging workflow

1. Enable verbose logging: set `kCacheVerboseLogs = true` in cache.cpp:46, rebuild debug DLL
2. Check `%TEMP%\cheat_debug.log` for: BN chain results, entity counts, prefab names, AIMBOT_WRITE diagnostics
3. After debugging, set `kCacheVerboseLogs = false` and rebuild
4. Logger filter (Logger.hpp) auto-skips verbose messages — only important messages appear in production log

## Constraints

- All game memory reads through kernel driver IOCTL — never direct memory access
- `rust_decrypts.dat` is locked while game runs — delete only when game is closed
- DLL output, not EXE — injected via loader
- No tests/typecheck/lint — only CI step is MSBuild compile
- Camera matrix read FRESH in render thread (not cached)
- Aimbot runs in render thread (not separate thread)
- Skeleton thread caps at 5 players per iteration
- Function-local `static std::unordered_map` CRASHES in manually mapped DLLs — use pointer-based lazy init
