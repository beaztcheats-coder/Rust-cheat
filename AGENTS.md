# AGENTS.md — Rust External Cheat

## Build

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /m
```

- VS2022 vcxproj, C++20, x64 Release DLL
- Build can also be run via `tools\masterupdate\auto_update.bat` + `update_now.bat`
- Requires DirectX SDK (included in repo under `Microsoft DirectX SDK/`)

### Build flavors (3)

| Flavor | TargetName | Output DLL | Logging | Shutdown Cleanup | Menu |
|--------|-----------|-----------|---------|-----------------|------|
| **BeaZt debug** | `Rust Prv Ext_debug` | `Rust Prv Ext_debug.dll` | Auto-enabled (DLL name `_debug`) | Keeps log files; deletes configs/markers/decrypts only | BeaZt |
| **BeaZt production** | `Rust Prv Ext` | `Rust Prv Ext.dll` | Disabled (enable via `C:\rust_debug_enabled.txt` marker) | **Deletes ALL trace files** (logs, configs, markers, decrypts) | BeaZt |
| **Bomza production** | `bomzarust` | `bomzarust.dll` | Disabled (enable via `C:\rust_debug_enabled.txt` marker) | **Deletes ALL trace files** (logs, configs, markers, decrypts) | Bomza |

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

### Build all 3 flavors

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName="Rust Prv Ext_debug" /m
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName="Rust Prv Ext" /m
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName=bomzarust /m
```

### Debug logging

- **Production builds**: Logging disabled by default. Enable by creating `C:\rust_debug_enabled.txt` (run as admin: `echo enabled > C:\rust_debug_enabled.txt`)
- **Debug build** (`Rust Prv Ext_debug.dll`): Logging auto-enabled — DLL name contains `_debug`, detected at runtime via `GetModuleFileNameA` + `find("_debug")`
- **Log output**: `C:\cheat_debug.log` (BeaZt) / `C:\cheat_debug_bomza.log` (Bomza)
- **Stealth mode**: Production builds delete ALL trace files on shutdown (logs, configs, markers, decrypts). Debug builds keep logs but delete everything else.
- **`Logger.hpp`**: `g_UpdateLoggingEnabled` defaults to `false`. Set to `true` only by: (1) `_debug` DLL name check, or (2) `C:\rust_debug_enabled.txt` marker file. Both checks are in `MainThreadImpl()` at `Rust Prv Ext.cpp` lines ~254-272.

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
| `Hacks/Aimbot/aimbot.cpp` | Memory aimbot (writes `bodyAngles` to `PlayerInput+0x44`) |
| `Hacks/Visuals/visuals.cpp` | Box/Skeleton/Name/Distance/Health/Head ESP rendering |
| `Hacks/Cheat/Cheat.hpp` | `Do_Cheat()` main loop — iterates entity lists, dispatches to visuals/aimbot/misc |
| `Render/render.hpp` | ImGui D3D11 overlay: `Hijack()`, `Draw()`, `Menu()` (10-page UI, BeaZt/Vsharp branded) |
| `Config.hpp` | Save/Load config (`C:\rustcfg_vsharp.dat` / `C:\rustcfg_lite.dat` / `C:\rustcfg_private.dat`) — key=value format |
| `OffsetManager.hpp` | Decrypt config struct, `LoadDecryptConfig`/`SaveDecryptConfig`, embedded defaults |
| `Logger.hpp` | File logging (`C:\cheat_debug_vsharp.log` / `C:\cheat_debug_lite.log` / `C:\cheat_debug_private.log`) |
| `RuntimePaths.hpp` | Detects Vsharp/Lite/Private variant by DLL filename, returns config/log paths |
| `Rust Prv Ext.cpp` | `DllMain`, `MainThread`, init sequence, export functions (Main/Init/Run/etc.) |
| `tools/masterupdate/` | Auto-updater: fetches Morphine/sha-dumper offsets, generates patches + super prompt |

## Critical runtime files

| Path | Purpose | Notes |
|------|---------|-------|
| `C:\cheat_debug_vsharp.log` / `C:\cheat_debug_lite.log` / `C:\cheat_debug_private.log` | Diagnostic log | Flavor-specific per DLL name |
| `C:\rustcfg_vsharp.dat` / `C:\rustcfg_lite.dat` / `C:\rustcfg_private.dat` | User settings | Saved/loaded via menu or `Config::Save()`/`Config::Load()` |
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

The v1mper UC dump (June 6 2026) is from a **different GameAssembly.dll build**. All BasePlayer offsets from that dump were wrong for our binary. The authoritative source is the Morphine auto-generated output at `tools/masterupdate/output/offsets.hpp` — always verify against it.

## Verified correct offsets (Morphine build 23824285)

**Static pointers**: `basenetworkable_pointer=0xE334210`, `camera_pointer=0xE37ACA0`, `TOD_Sky_TypeInfo=0xE3593D0`, `FOV ConVar_Graphics=0xE334790`

**BaseCamera** (sha-dumper now uses structural validation — auto-applied when `validated>=1`, falls back to UC sources when `validated=0`): `viewMatrix=0x2FC` (fefe4444 #24841 + diagnostic), `projectionMatrix=0x18C` (old UC dump), `world_position=0x444` (fefe4444 #24841), `field_of_view=0x170` (v1mper IL2CPP dump + temopzso #24865), `culling_mask=0x74` (v1mper IL2CPP dump), `static_fields=0xB8`, `wrapper_class=0x90`, `entity=0x10`

**BasePlayer**: `PlayerEyes=0x2E0`, `PlayerInventory=0x2B8`, `PlayerInput=0x560`, `PlayerModel=0x678`, `PlayerFlags=0x670`, `ClActiveItem=0x520`, `BaseMovement=0x388`, `DisplayName=0x488`, `ModelState=0x398`, `Mounted=0x570`, `CurrentTeam=0x4F0`

**PlayerInventory**: `Belt=0x60`, `Wear=0x30`, `Main=0x58`, `loot=0x48`

**ItemContainer**: `ItemList=0x20` (Morphine), `ItemListFallback=0x10`

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

## PhysX Visibility Check (VisCheck)

External ray-based line-of-sight check using PhysX collision mesh data. Replaces unreliable `PlayerModel + 0xBC` flag.

**Approach**: Read `PxPhysics*` from `UnityPlayer.dll + PX_SDK_OFFSET` → navigate to scenes → actors → shapes → triangle meshes → build BVH client-side → Möller-Trumbore ray-triangle intersection (zero IOCTLs per frame after caching).

**Files**: `Hacks/PhysX/PhysX.hpp` — structs (np_physics_t, np_scene_t, np_rigid_static_t, etc.), BVH builder, PhysXRaycaster class with `CacheActors()`, `Raycast()`, `Linecast()`, `IsVisible()`.

**Caching**: `CacheActors()` called every 5 seconds from cache thread (when `ESP::VisCheck` enabled). Reads all PhysX actors, extracts triangle geometry (triangle meshes + boxes), builds BVH per actor. Ray tests are pure math.

**Runtime flow**:
1. Cache thread: `g_PhysX.CacheActors()` every 5s → reads PhysX scene/actor/shape data, builds BVHs
2. Position thread: stores `g_CameraWorldPos` from `BaseCamera::world_position`
3. Render thread (Do_Cheat): if `ESP::VisCheck && g_PhysX.IsReady()`, calls `g_PhysX.IsVisible(camPos, playerHeadPos)` per player → overrides `cached.isVisible` in both `cacheCopy` and `g_EspCache` (for aimbot)
4. Visuals: use `ESP::color::Visible` / `ESP::color::Invisible` based on `cached.isVisible`
5. Aimbot: `AIMBOT::VisibleOnly` filter uses `pcache.isVisible` (now PhysX-driven when VisCheck on)

**Known issues**:
- `PX_SDK_OFFSET = 0x1C3B3D0` is from July 2025 UC release — may need updating for current Rust build (23824285). If PhysX shows "not ready" in menu, this offset needs pattern scanning.
- PhysX struct layouts (np_rigid_static_t, m_shape_manager, shape_t) are from UC release — may need verification against current PhysX version.
- Windows `min`/`max` macros conflict with struct member names and `<algorithm>` — `#undef min`/`#undef max` in PhysX.hpp before STL includes.
- `Vector3` operators (`+`, `-`, `*`) are not `const`-qualified — use `pxmath::add`/`sub`/`mul` free functions for const refs.

## VisCheck (BVH Raycast)

External ray-based line-of-sight check using extracted MeshCollider geometry. Replaces the PhysX VisCheck and the unreliable `PlayerModel + 0xBC` flag.

**Approach**: Sha-dumper (injected into RustClient.exe) extracts all MeshCollider triangles via `Resources.FindObjectsOfTypeAll<MeshCollider>()`, transforms them to world space, and writes raw `Tri` structs (36 bytes each) to a `.tri` file. The cheat loads this file, builds a BVH (median-split, leaf size 8), and uses Möller-Trumbore ray-triangle intersection for visibility checks.

**Files**: 
- `Hacks/VisCheck/VisCheck.hpp` — BVH + Möller-Trumbore raycast engine, multi-path loader, BVH cache
- `tools/sha-dumper/src/mesh/mesh.cpp` — mesh extraction (MeshCollider + BoxCollider), writes `.tri` file to Desktop
- `tools/sha-dumper/src/mesh/mesh.hpp` — header

**Mesh filtering**: Meshes with <5 triangles or bounding box <0.5m³ are skipped (small debris). Reduces ~8M triangles to ~5M.

**Multi-path loader** (search order):
1. `C:\rust_mesh.tri` (loader copies here — admin process)
2. DLL's own directory via `GetModuleFileNameA` (ships next to DLL)
3. `%LOCALAPPDATA%\rust_mesh.tri` (last resort fallback)

**BVH cache**: After first BVH build (~20-30s for 5M triangles), the built BVH is serialized to `C:\rust_mesh.bvh` (or same dir as `.tri`). On subsequent loads, if the cache exists and the `.tri` file size matches, the BVH is loaded directly (~2s). Cache format: magic `BVH1` + version + counts + triFileSize + indices + nodes.

**Runtime flow**:
1. Cache thread: `vischeck::InitVisCheck()` on first cycle → loads `.tri`, builds BVH (or loads from `.bvh` cache)
2. Render thread (Do_Cheat): if `ESP::VisCheck && vischeck::g_VisCheckLoaded && Distance <= 400.f`, calls `g_VisCheck.IsVisible(camPos, playerHeadPos)` → overrides `cached.isVisible`
3. Visuals: use `ESP::color::Visible` / `ESP::color::Invisible` based on `cached.isVisible`
4. Aimbot: `AIMBOT::VisibleOnly` filter uses `pcache.isVisible`

**Fallback**: When no `.tri` file is loaded, VisCheck falls back to `PlayerModel._visible` Nullable<bool> flag at offset 0xBC (read as `{hasValue, value}` struct, ~60% accuracy).

**Production pipeline**:
1. Seller runs `auto_update.bat` (with Rust in-game) → sha-dumper dumps mesh → `output/rust_mesh.tri`
2. Seller runs `update_now.bat` → builds DLL + copies mesh to distribution
3. Package: `Rust Prv Ext.dll` + `rust_mesh.tri` + `rust_decrypts.dat` + `inject_private.bat`
4. Customer runs `inject_private.bat` (as admin) → copies `rust_mesh.tri` to `C:\` → injects DLL
5. Cheat loads mesh → builds BVH on first run → caches to `C:\rust_mesh.bvh` → instant on subsequent runs

**Mesh regeneration**: Required when the Rust map changes (game update). Run `auto_update.bat` with Rust in-game on a server. The world must be loaded (not main menu).

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

