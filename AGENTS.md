# AGENTS.md — Rust External Cheat

## Build

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /m
```

- VS2022 vcxproj, C++20, x64 Release DLL
- Build can also be run via `tools\masterupdate\getnewoffsets.bat` (renamed from `auto_update.bat`) + `update_now.bat`
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

### Spoofer/Cleaner startup prompt (BeaZt only)

On injection, BeaZt debug and production DLLs show a console prompt before cheat init:
1. **[1] Spoof and Play** — calls `RunEmbeddedSpoofer()` → MessageBox "Spoof now? Yes/No" → 11 IOCTL spoofs via kernel driver → cheat continues
2. **[2] Clean after detection** — calls `CleanerMenu()` → interactive 8-phase deep cleaner → DLL exits via `FreeLibraryAndExitThread` (user restarts + re-injects)
3. **[3] Just play** — skip, cheat continues

**Bomza production**: No prompt — straight to cheat (`#if !defined(BOMZA) && !defined(BETTERCHEATS)` guard). Spoofer/cleaner code compiled but dead-stripped by `/OPT:REF`.

**Better Cheats production**: No prompt — straight to cheat (same guard as Bomza).

**Files**: `Hacks/Misc/spoofer.cpp` (spoof logic), `Hacks/Misc/cleaner.cpp` (cleaner menu), `Rust Prv Ext.cpp` lines ~280-324 (prompt).

## Offset Pipeline (100% Morphine-Independent)

The pipeline works **without Morphine** using sha-dumper + capstone + Frida:

### Sources (priority order)

| Source | Provides | Requires Game? | Status |
|--------|----------|---------------|--------|
| **sha-dumper** (injected DLL) | Field offsets, static TypeInfos, BN chain offsets, camera offsets, GCHandle RVA, materials, mesh | Yes (game running) | Working |
| **disasm_decrypts.py** (offline capstone) | Decrypt algorithms (ROL/XOR/ADD/SUB chains) — reads GameAssembly.dll from disk | No (offline) | Working — 5/6 decrypts identified |
| **Frida** (runtime dump) | Field offsets, field types, method RVAs, cross-validation | Yes (game running) | Working (optional cross-validation) |
| **Morphine** (web download) | All offsets + decrypts | No | **OPTIONAL** — cross-validation only |
| **Il2CppInspectorPro** (offline) | Field offsets, TypeInfo RVAs, method RVAs | No | Broken (.NET 10 preview required) |

### Pipeline flow (`getnewoffsets.bat`)

1. **Morphine prompt** (optional, defaults to skip in 10s)
2. **Il2CppInspectorPro** (offline, currently broken — .NET 10)
3. **sha-dumper** (injected into game, dumps offsets + ciphers + camera + mesh)
4. **parse_sha_dumper.py** — merges sha-dumper output into master.json
5. **disasm_decrypts.py** — reads GameAssembly.dll from disk, disassembles all cipher functions with capstone, identifies 6 decrypt algorithms, merges into master.json
6. **Frida** (optional, cross-validation)
7. **compare_and_patch.py** — generates 5 patch files from master.json
8. **apply_patches.py** — applies patches to offsets.hpp/sdk.hpp/OffsetManager.hpp

### disasm_decrypts.py — How it works

1. Parses ALL cipher sections from sha-dumper output (930+ sections including unknowns)
2. For each cipher, reads bytes at fn_rva from GameAssembly.dll using `pefile`
3. Disassembles with `capstone` (CS_ARCH_X86, CS_MODE_64)
4. Extracts ROL/XOR/ADD/SUB operations (handles SHL+SHR+OR ROL pattern, loop unrolling)
5. Matches against known patterns (exact match → high confidence)
6. For unidentified ciphers, tries structural matching (op-type sequence only)
7. For still-missing decrypts, searches GameAssembly.dll binary for known constants
8. Merges results into master.json["decrypt_functions"]

**Key advantage over sha-dumper HDE64**: Capstone correctly decodes ALL x86-64 instruction variants, including ROL implemented as SHL+SHR+OR on different registers. The sha-dumper's HDE64-based extractor missed ROL operations, producing wrong algorithms.

### One-click update workflow

```
1. Start Rust (join any server)
2. Run: tools\masterupdate\getnewoffsets.bat  (choose [1] Skip Morphine)
3. Run: tools\masterupdate\update_now.bat
4. Inject and play
```

No Morphine. No Frida (optional). No manual offset verification.

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
| `Hacks/Aimbot/aimbot.cpp` | Memory aimbot (writes `bodyAngles` to `PlayerInput+0x44`) |
| `Hacks/Visuals/visuals.cpp` | Box/Skeleton/Name/Distance/Health/Head ESP rendering |
| `Hacks/Cheat/Cheat.hpp` | `Do_Cheat()` main loop — iterates entity lists, dispatches to visuals/aimbot/misc |
| `Render/render.hpp` | ImGui D3D11 overlay: `Hijack()`, `Draw()`, `Menu()` (10-page UI, BeaZt/Bomza/Better Cheats branded) |
| `Config.hpp` | Save/Load config (`C:\rustcfg.dat` / `C:\rustcfg_bomza.dat` / `C:\rustcfg_bc.dat`) — key=value format |
| `OffsetManager.hpp` | Decrypt config struct, `LoadDecryptConfig`/`SaveDecryptConfig`, embedded defaults |
| `Logger.hpp` | File logging (`C:\cheat_debug.log` BeaZt / `C:\cheat_debug_bomza.log` Bomza / `C:\cheat_debug_bc.log` Better Cheats) |
| `RuntimePaths.hpp` | Detects BeaZt/Bomza/Better Cheats variant by DLL filename, returns config/log/spoofer paths |
| `Rust Prv Ext.cpp` | `DllMain`, `MainThread`, init sequence, export functions (Main/Init/Run/etc.) |
| `tools/masterupdate/` | Auto-updater: sha-dumper + capstone decrypt + Frida cross-validation, generates patches + super prompt |
| `tools/masterupdate/disasm_decrypts.py` | **Offline capstone decrypt disassembler** — reads GameAssembly.dll from disk, disassembles decrypt functions with capstone, 100% Morphine-independent |
| `tools/masterupdate/compare_and_patch.py` | Generates 5 patch files from master.json (offsets, sdk, decrypt, OffsetManager, rust_decrypts.dat) |
| `tools/sha-dumper/` | Injected DLL: scans GameAssembly for field offsets, static TypeInfos, BN decrypts, camera offsets, materials, mesh |

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

The v1mper UC dump (June 6 2026) is from a **different GameAssembly.dll build**. All BasePlayer offsets from that dump were wrong for our binary. The authoritative source is the sha-dumper + capstone output at `tools/masterupdate/output/master.json` — always verify against it.

## Verified correct offsets (Build 24037537)

**Static pointers**: `basenetworkable_pointer=0x0FD36298`, `camera_pointer=0x0FD0A5C0`, `TOD_Sky_TypeInfo=0x0FCEBC70`, `FOV ConVar_Graphics=0xFD05BC0`

**BaseCamera** (sha-dumper now uses structural validation — auto-applied when `validated>=1`, falls back to UC sources when `validated=0`): `viewMatrix=0x2FC` (fefe4444 #24841 + diagnostic), `projectionMatrix=0x18C` (old UC dump), `world_position=0x444` (fefe4444 #24841), `field_of_view=0x170` (v1mper IL2CPP dump + temopzso #24865), `culling_mask=0x74` (v1mper IL2CPP dump), `static_fields=0xB8`, `wrapper_class=0x90`, `entity=0x10`

**BasePlayer**: `PlayerEyes=0x2E0`, `PlayerInventory=0x2B8`, `PlayerInput=0x560`, `PlayerModel=0x678`, `PlayerFlags=0x670`, `ClActiveItem=0x520`, `BaseMovement=0x388`, `DisplayName=0x488`, `ModelState=0x398`, `Mounted=0x570`, `CurrentTeam=0x4F0`

**PlayerInventory**: `Belt=0x60`, `Wear=0x30`, `Main=0x58`, `loot=0x48`

**ItemContainer**: `ItemList=0x20` (sha-dumper + capstone), `ItemListFallback=0x10`

**Item**: `ItemDefinition=0x38`, `ItemId=0x98`, `HeldEntity_1=0x40`, `Amount=0x64`

**ItemDefinition**: `shortname=0x28`, `itemid=0x20`, `displayName=0x40`

**BN chain**: `static_fields=0xB8`, `wrapper_class=0x30`, `parent_static_fields=0x10`, `entity=0x18`

**List<T> layout**: `list+0x10` = array ptr, `list+0x18` = size, `array+0x20+i*8` = element[i]

## Inventory Resolution (ResolvePlayerInventory)

PlayerInventory at `BasePlayer+0x2B8` is a **wrapper struct pointer**, NOT the inventory pointer directly. The correct resolution chain is:

```
BasePlayer + 0x2B8 → inv_raw (wrapper struct pointer)
inv_raw + 0x18     → encrypted GCHandle
decrypt_inventory_pointer(enc_handle) → decrypted GCHandle
Il2cppGetHandle(dec_handle) → PlayerInventory pointer
```

This is the SAME pattern as `networkable_key` / `networkable_key2` in the BN chain. `ResolvePlayerInventory()` in `sdk.hpp` tries this first, then falls back to `GetPlayerInventory()` (component walker). The method is cached via `static int invMethod`. Do NOT change this to direct decrypt or raw pointer — it will break held item resolution.

`Get_HeldItem()`, `Get_Hotbar_list()`, and `cache.cpp` wear read all use `ResolvePlayerInventory()` + direct `Belt=0x60` + `ItemList=0x20`.

## IOCTL Stability (ESP Freeze Prevention)

- `ioctl_fail_count` is cumulative (failures +1, successes -1). Time-based decay: every 500ms, subtracts 10 (floor at 0). Prevents slow accumulation from causing periodic blackouts.
- `IOCTL_FAIL_LIMIT=300` — blocks all reads when reached.
- `ioctl_blocked` auto-resets after 1000ms (was 3000ms).
- `markUnavailable()` in `cache.cpp` checks `ioctl_blocked` first — during transient blocks, it preserves existing entity lists and cache instead of clearing them. ESP continues rendering from last-known-good data.
- `find_process` requires 3 consecutive failures before shutdown (was 1).

## Skeleton ESP

- Full skeleton (15 bones): within 20m
- Partial skeleton (9 key bones): within 300m (`kPartialSkeletonDist` in `cache.cpp`)
- Batched polyline rendering (4 chains) with per-bone interpolation on skeleton thread
- Menu slider `ESP::SkeletonDist` controls render-side distance (10–300m)

## Menu: anti-pattern to avoid

**Never** wrap `ImGui::Checkbox` in style push/pop lambdas. The old `Danger()`/`Warning()`/`Normal()` pattern pushed `ImGuiCol_Text` before the checkbox and popped after — this corrupted the style stack in nested child windows, causing clicks to not register. Use a post-render badged label instead.

## VisCheck (DISABLED — Deferred)

**Status**: VisCheck is currently **disabled and removed from the menu**. The toggle, BVH status, and VisCheck color rows (Visible/Invisible/SkelVisible/SkelInvisible) are commented out in both `Beazt/beazt_menu.hpp` and `Bomza/bomza_menu.hpp`. `ESP::VisCheck` defaults to `false` and is forced off on startup in `render.hpp`.

**Code retained** (not deleted, for future re-enablement):
- `Hacks/VisCheck/VisCheck.hpp` — BVH + Möller-Trumbore raycast engine, multi-path `.tri` loader, BVH cache
- `Hacks/PhysX/PhysX.hpp` — PhysX raycaster (alternative approach, also unused)
- `tools/sha-dumper/src/mesh/` — mesh extraction code
- Config save/load for `ESP::VisCheck` and VisCheck colors still exists in `Config.hpp`

**To re-enable later**:
1. Uncomment the VisCheck card in `Beazt/beazt_menu.hpp` (search for `// VisCheck disabled`)
2. Uncomment the `ImGui::Checkbox("VisCheck", ...)` line in `Bomza/bomza_menu.hpp`
3. Generate `rust_mesh.tri` via `getnewoffsets.bat` (requires Rust in-game on a server)
4. Verify `PX_SDK_OFFSET` in `VisCheck.hpp` matches current Rust build
5. Run `update_now.bat` to build + distribute mesh with DLL

**Known issues** (to address when re-enabling):
- `PX_SDK_OFFSET = 0x1C3B3D0` is from July 2025 UC release — may need updating
- PhysX struct layouts may need verification against current PhysX version
- `Vector3` operators not `const`-qualified — use `pxmath::` free functions
- Mesh dump timeout in `sha_dumper_inject.py` is now 300s (was 180s)

## Constraints

- **External cheat**: all game memory reads through `read<T>(address)` → kernel driver IOCTL. Never direct memory access.
- Feature toggles are `inline` globals in `offsets.hpp` — no thread sync, but UI and cheat logic share the same thread.
- `C:\rust_decrypts.dat` is locked while game runs; cannot delete/edit unless game is closed.
- DLL output, not EXE; injected via loader.
- There are no tests, typecheck, or lint scripts. The only CI step is the MSBuild compile.
- **Camera matrix reading MUST be BEFORE `playerList.empty()` check** in PositionRefresh (cache.cpp). If gated by playerList, camera matrix is never read when no other players are on server → ALL ESP blocked (NPC, animal, world, everything).
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

