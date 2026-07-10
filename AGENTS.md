# AGENTS.md — Rust External Cheat

## Build

```
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "E:\github\Rust-cheat\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName=<name> /m
```

- VS2022 vcxproj, C++20, x64 Release DLL, llvm-msvc compiler
- Requires DirectX SDK (included in repo under `Microsoft DirectX SDK/`)

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

### Decrypt constants (build 24091435 — morphine confirmed)

All constants in `OffsetManager.hpp`. Can be overridden at runtime via `%TEMP%\rust_decrypts.dat`.

| Function | Operations |
|----------|-----------|
| networkable_key | ADD(0x16A2149) → ROL(7) → ADD(0x42D649F) |
| networkable_key2 | ROL(0x14) → SUB(0x1CC81122) → XOR(0xAACADDEB) → ADD(0x63DFF63C) |
| cl_active_item | XOR(0x320F4C7D) → ROL(0x1F) → ADD(0x62E951E6) |
| inventory_pointer | ROL(7) → XOR(0xEC9526A3) → ADD(0xAC5D1D91) → ROL(0xF) |
| eyes | SUB(0x2C865414) → XOR(0x873FAFE5) → ADD(0x3FD966E1) → ROL(6) |
| fov | ADD(0xCA34BF6D) → ROL(0x1D) → SUB(0xB723318) |

---

## Architecture

External Rust cheat DLL injected via loader into host process. Reads/writes game memory through kernel driver (`Bfo64` at `\\.\Bfo64` IOCTL). ImGui D3D11 overlay for menu. Target: **Rust (Steam) Unity 6 Il2Cpp**.

### File roles

| File | Role |
|------|------|
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

### Thread architecture

| Thread | Sleep | Purpose |
|--------|-------|---------|
| Cache | 1ms + 100ms target | Entity list + classification + inventory scan + world prefab ESP |
| Position | 1ms | Player positions + velocities + local player neck/crouch state |
| Skeleton | 1ms | 15 bones, 5 players/iteration, TTL cached |
| Misc | 1ms | BrightNight, FOV, debug cam |
| Render | vsync | ESP + aimbot (fresh camera matrix each frame) |

### Init sequence (MUST be this order)

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

**NEVER do any of these before `get_module()` succeeds:**

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

**DPI awareness**: Use ONLY `SetProcessDpiAwarenessContext((HANDLE)-4)`. It fails silently with `ERROR_ACCESS_DENIED` in injected DLLs (host process already set DPI awareness). This is SAFE — no DPI change, no EAC trigger. Overlay fixes (swap chain ResizeBuffers, ScreenToClient, MonitorFromWindow) still work without DPI awareness.

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
```

Returns `(-1, -1)` for entities behind camera or too close to near-clip plane. This prevents behind-camera entities from projecting to extreme screen positions ("ESP in the sky" bug). CCODIX approach — safer than clamping w to 0.0001f (SHA source approach).

---

## Cache (cache.cpp)

### Entity classification flow

All entities from BN chain go through:
1. Read prefab name from `objectClass + 0x50` (full prefab path like "assets/prefabs/plants/hemp/hemp.entity.prefab")
2. Classify: player (keyword + Il2CppClass fallback) → NPC (keyword) → animal (keyword) → world (else)
3. World entities matched by prefab name substring patterns

### Prefab name cache

- **Keyed by Il2CppClass pointer** (entity offset 0x00) — shared per entity TYPE, not per instance. 100 hemp = 1 cache entry.
- **1024 slots**, reset each cache cycle
- Entries expire after 10 cycles
- Fallback: `objectClass` (GameObject pointer) if klass read fails

### World ESP prefab patterns (build 24091435 verified)

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
