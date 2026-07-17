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
- **Logger filter**: `Logger.hpp` filters out ~50 verbose patterns — only important messages (FAIL, CRASH, SKIP, FATAL, Setup, BN chain, CAM_CHAIN, Do_Cheat, Shutdown) appear in the log
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
| TOD_Sky_TypeInfo | 0xFD0A5D0 | TOD_Sky Static TypeInfo (also used as Class_TOD_Sky_Static) |
| EffectNetwork_Pointer | 0xFCAC040 | static_type_info |
| ConVar_Graphics | 0xFCF2C10 | convar_graphics (in FOV namespace) |
| cam_typeinfo_singleton | 0xE2D5CC8 | Diagnostic only (kCacheVerboseLogs) |
| cam_typeinfo_camera_c | 0xE2C3EC8 | Diagnostic only |

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
| BasePlayer | ModelTransform | 0x1A8 |
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
| BaseCombatEntity | model | 0x1A8 |
| BaseEntity | positionLerp | 0x1C8 |
| BaseEntity | bounds | 0x17C |
| BaseEntity | flags | 0x1B0 |
| BaseEntity | model | 0x1A8 |
| BaseEntity | isVisible | 0x150 |
| BaseEntity | isAnimatorVisible | 0x151 |
| BaseEntity | isShadowVisible | 0x152 |
| BaseEntity | objRef | 0x10 |
| BaseEntity | posChain0 | 0x20 |
| BaseEntity | posChain1 | 0x20 |
| BaseEntity | posChain2 | 0x8 |
| BaseEntity | posChain3 | 0x28 |
| BaseEntity | posFinal | 0x90 |
| PlayerInventory | Belt | 0x58 |
| PlayerInventory | Wear | 0x78 |
| PlayerInventory | Main | 0x30 |
| PlayerInventory | loot | 0x48 |
| PlayerInventory | BeltFallback1 | 0x78 |
| PlayerInventory | BeltFallback2 | 0x30 |
| ItemContainer | ItemList | 0x60 |
| ItemContainer | ItemListFallback | 0x10 |
| Item | ItemDefinition | 0xA8 |
| Item | ItemId | 0xB8 |
| Item | HeldEntity_1 | 0xA8 |
| Item | Amount | 0x24 |
| Item | ItemIdFallback1 | 0x38 |
| Item | ItemIdFallback2 | 0x70 |
| ItemDefinition | itemid | 0x20 |
| ItemDefinition | shortname | 0x28 |
| ItemDefinition | displayName | 0x40 |
| ItemDefinition | category | 0x58 |
| ItemDefinition | stackable | 0x78 |
| PlayerModel | SkinnedMultiMesh | 0x420 |
| PlayerModel | is_npc | 0x490 |
| PlayerModel | position | 0x2F8 |
| PlayerModel | velocity | 0x31C |
| PlayerModel | visible | 0xC4 |
| PlayerModel | boneTransforms | 0x98 |
| PlayerModel | rootBone | 0x98 |
| PlayerModel | headBone | 0xF8 |
| PlayerModel | eyeBone | 0x120 |
| HeldEntity | ownerItemUID | 0x2D0 |
| HeldEntity | viewModel | 0x2C8 |
| Model | boneTransforms | 0x50 |
| RecoilProperties | RecoilYawMin | 0x18 |
| RecoilProperties | RecoilYawMax | 0x1C |
| RecoilProperties | RecoilPitchMin | 0x20 |
| RecoilProperties | RecoilPitchMax | 0x24 |
| RecoilProperties | AimconeCurveScale | 0x60 |
| RecoilProperties | NewRecoilOverride | 0x80 |
| BaseProjectile | recoil | 0x3F0 |
| BaseProjectile | primaryMagazine | 0x3C8 |
| BaseProjectile | AimCone | 0x400 |
| BaseProjectile | HasADS | 0x41C |
| BaseProjectile | automatic | 0x380 |
| BaseProjectile | is_reloading | 0x3D0 |
| BaseProjectile | reload_time | 0x3C0 |
| BaseProjectile | velocity_scale | 0x37C |
| BaseProjectile | aimSway | 0x3E8 |
| BaseProjectile | aimSwaySpeed | 0x3EC |
| BaseProjectile | StancePenalty | 0x418 |
| BaseProjectile | SightAimConeScale | 0x45C |
| BaseProjectile | HipAimCone | 0x404 |
| BaseProjectile | AimconePenalty | 0x408 |
| BaseProjectile | isBurstWeapon | 0x427 |
| BaseProjectile | canChangeFireModes | 0x428 |
| BaseMovement2 | admin_cheat | 0x20 |
| BaseMovement2 | target_movement | 0x128 |
| TOD_Sky_Static | instances | 0x8 |
| TOD_Sky | Instances | 0x10 |
| TOD_Sky | NightParameters | 0x60 |
| TOD_Sky | AmbientParameters | 0x98 |
| TOD_Sky | CycleParameters | 0x40 |
| TOD_Sky | timeSinceAmbientUpdate | 0x23C |
| TOD_NightParameters | AmbientMultiplier | 0x5C |
| TOD_AmbientParameters | Saturation | 0x14 |
| TOD_AmbientParameters | UpdateInterval | 0x18 |
| PlayerInput | bodyAngles | 0x44 |
| PlayerInput | yaw | 0x60 |
| PlayerEyes | viewOffset | 0x40 |
| PlayerEyes | bodyRotation | 0x50 |
| PlayerEyes | headPosition | 0x60 |
| ModelState | flags | 0x0 |
| ModelState | Ducked | 0x10 |
| ModelState | OnGround | 0x04 |
| ModelState | Flying | 0x40 |
| BaseNetworkable2 | prefab_id | 0x54 |
| FOV | ConVar_Graphics | 0xFCF2C10 |
| FOV | fovField | 0x560 |
| FOV | fovWrite | 0x110 |
| FOV | cameraFovBypass | 0x170 |
| EffectNetwork | static_fields | 0xB8 |
| EffectNetwork | instance | 0x8 |
| EffectNetwork | hitPosition | 0x90 |
| ItemMagazine | contents | 0x1C |

### Decrypt constants (build 24091435 — morphine confirmed)

All constants in `OffsetManager.hpp`. Can be overridden at runtime via `rust_decrypts.dat` (DLL dir or `C:\`).

| Function | Operations |
|----------|-----------|
| networkable_key | ADD(0x16A2149) -> ROL(7) -> ADD(0x42D649F), count=2, then Il2cppGetHandle |
| networkable_key2 | ROL(0x14) -> SUB(0x1CC81122) -> XOR(0xAACADDEB) -> ADD(0x63DFF63C), count=2, then Il2cppGetHandle |
| cl_active_item | XOR(0x320F4C7D) -> ROL(0x1F) -> ADD(0x62E951E6), count=1 |
| inventory_pointer | ROL(7) -> XOR(0xEC9526A3) -> ADD(0xAC5D1D91) -> ROL(0xF), count=2 |
| eyes | SUB(0x2C865414) -> XOR(0x873FAFE5) -> ADD(0x3FD966E1) -> ROL(6), count=2 |
| fov | ADD(0xCA34BF6D) -> ROL(0x1D) -> SUB(0xB723318) |

---

## Architecture

External Rust cheat DLL injected via loader into host process. Reads/writes game memory through kernel driver (`Bfo64` at `\\.\Bfo64` IOCTL). ImGui D3D11 overlay for menu. Target: **Rust (Steam) Unity 6 Il2Cpp**.

### File roles

| File | Lines | Role |
|------|-------|------|
| `Rust Prv Ext.cpp` | 884 | DllMain, MainThreadImpl, init sequence, VEH crash handler, worker thread starters, shutdown |
| `offsets.hpp` | 760 | Game offsets, all feature toggle globals, color presets |
| `sdk.hpp` | 1083 | Rust class, BaseEntity, decrypt functions, Get_Position (SSE chain), HeldItem, ResolvePlayerInventory, Get_HeldItem |
| `Driver.hpp` | 802 | DriverInterface, read<T>/write<T> templates, get_module (PE scan fallback), find_process, IOCTL stability |
| `Hacks/Cache/cache.cpp` | 1746 | BN entity chain walk, entity classification, prefab name cache, world prefab ESP, skeleton/position refresh |
| `Hacks/Cache/cache.hpp` | 112 | EspCacheData struct, global cache declarations, cache class |
| `Hacks/Misc/misc.cpp` | 706 | Recoil, FOV changer, BrightNight, TimeChanger, debug cam, AdminFlags, burst mode |
| `Hacks/Aimbot/aimbot.cpp` | 628 | Memory aimbot (writes bodyAngles), target selection, humanize, prediction |
| `Hacks/Visuals/visuals.cpp` | 771 | Box/Skeleton/Name/Distance/Health ESP rendering for players, NPCs, animals |
| `Hacks/Cheat/Cheat.hpp` | 476 | Do_Cheat() main loop — reads camera matrix fresh, renders ESP, runs aimbot |
| `Render/render.hpp` | 2145 | ImGui D3D11 overlay: Hijack(), Draw(), Render(), menu rendering, input handling |
| `Config.hpp` | 980 | Save/Load config, ApplyLegitDefaults(), presets |
| `OffsetManager.hpp` | 173 | Decrypt config struct, LoadDecryptConfig, embedded defaults |
| `Logger.hpp` | 98 | File logging with verbose message filter (~50 skip patterns) |
| `RuntimePaths.hpp` | 161 | Detects flavor by DLL filename (PE header fallback for manually mapped DLLs) |
| `vectorMath.hpp` | 755 | WorldToScreen, g_ScreenW/g_ScreenH, Matrix4x4, Vector3/Vector2, CalcAngle, BoneList enum |
| `Hotkeys.hpp` | 160 | HotkeyBind system, toggle/hold modes, conflict detection |
| `VisCheck/VisCheck.hpp` | 755 | BVH raycast visibility checking (PhysX mesh or .tri file) |
| `Translation.hpp` | — | TR() macro for EN/FR string localization |
| `draw.hpp` | — | Drawing helpers (text outline, arc, etc.) |
| `Overlay.hpp` | 17 | Overlay::DrawOverlay() — calls Renderer Setup + Render |

### Deleted files (cleanup)

These files were deleted during codebase cleanup. Do NOT recreate them:

| File | Why deleted |
|------|-------------|
| `Occlusion.hpp` | Dead code — AABB occlusion system never wired up, all functions unused |
| `tracers.hpp` | Dead code — TracerDetectShot/TracerRender never called |
| `Hacks/AntiCheat/anybrain.hpp` | Dead code — RVAs were 0x0, feature never functional, morphine dump doesn't have the class |

---

## Init Sequence (MUST be this order — `Rust Prv Ext.cpp:295`)

```
MainThreadImpl()
  1. VEH handler (AddVectoredExceptionHandler — logs access violations + stack trace)
  2. DPI awareness (SetProcessDpiAwarenessContext((HANDLE)-4) — fails silently, safe)
  3. Temp file cleanup (deletes rust_decrypts.dat, rust_mesh.tri, markers from DLL dir, C:\, %TEMP%)
  4. Diagnostic marker (writes cheat_dll_loaded.txt to DLL directory)
  5. Console + banner (CreateConsole, flavor-specific banner)
  6. Logging setup (checks _debug in DLL name or C:\rust_debug_enabled.txt marker)
  7. Spoofer prompt (BeaZt/Bomza only — NOT Better Cheats)
  8. Object construction (Drv, src, esp, hx, aim, cac — all new'd)
  9. Config load + translations init
  10. Driver validation (opens \\.\Bfo64)
  11. find_process("RustClient.exe") — loops until found
  12. HOME key wait (user must be at main menu)
  13. GetProcessBase -> GetCR3_1
  14. Internal reads detection (if same process ID, g_UseInternalReads=true)
  15. get_module(GameAssembly) — OpenProcess+NtQueryVirtualMemory (primary), PE scan (fallback)
  16. OffsetManager::Initialize (loads decrypt config, validates pointers)
  17. get_module(UnityPlayer)
  18. Start cache/position/skeleton workers + cache watchdog
  19. Process-death watchdog (20ms poll — sets g_process_dead when Rust exits)
  20. Misc thread (1ms sleep — calls do_misc when LocalPlayer valid + BnStableCycles >= 3)
  21. Render loop (overlay + Do_Cheat)
  22. Shutdown (block IOCTLs, wait for heartbeats, null Drv, delete temp files, self-delete DLL)
```

---

## Thread Architecture

| Thread | Started by | Sleep | Purpose | Mutexes used |
|--------|-----------|-------|---------|-------------|
| Cache | StartCacheWorker | 1ms + 100ms target | BN chain walk, entity classification, inventory scan, world ESP, list swap | entity_mutex, npc_mutex, animal_mutex, prefab_mutex, g_EspCacheMutex (write), g_TrailsMutex |
| Position | StartFastRefreshWorker | 1ms | Player positions + velocities + local player neck/crouch state | g_LocalPlayerDataMutex, g_EspCacheMutex (write), entity_mutex, npc_mutex, animal_mutex |
| Skeleton | StartSkeletonWorker | 1ms | 15 bones, 5 players/iteration, TTL cached (50/100/200ms by distance) | g_EspCacheMutex (read+write), entity_mutex, g_LocalPlayerDataMutex |
| Misc | inline lambda | 1ms + 10ms | BrightNight, FOV, debug cam, recoil, admin flags | s_miscMutex |
| Render | Overlay::DrawOverlay | vsync (Present 1,0) | ESP + aimbot (fresh camera matrix each frame) | g_EspCacheMutex (read try_to_lock), g_LocalPlayerDataMutex, src->matrixMutex, list mutexes (try_to_lock) |
| Watchdog | StartCacheWatchdog | 1000ms | Restarts stale workers (30s cache, 15s position, 30s skeleton) with 60s cooldown | — |
| Process-death | inline lambda | 20ms | Polls Drv->IsProcessAlive(), sets g_process_dead | — |

### Thread safety rules

- **g_EspCacheMutex** (shared_mutex): Cache/Position/Skeleton threads use `unique_lock` for writes. Render thread uses `shared_lock` with `try_to_lock` — never blocks, falls back to previous frame's data if lock fails. Aimbot uses `shared_lock`.
- **g_LocalPlayerDataMutex** (mutex): Position thread writes, Render/Aimbot/Misc threads read.
- **List mutexes** (entity_mutex, etc.): Cache thread writes via swap. Render thread reads with `try_to_lock`.
- **src->matrixMutex**: Render thread publishes (PublishMatrix), Aimbot/Misc threads read (GetMatrixSnapshot).
- **Function-local `static std::unordered_map` CRASHES in manually mapped DLLs** — use pointer-based lazy init pattern:
  ```cpp
  static std::unordered_map<K, V>* pMap = nullptr;
  if (!pMap) pMap = new std::unordered_map<K, V>();
  ```

---

## Driver (Bfo64) — `Driver.hpp`

### Read/Write methods

| Method | noncachedread | Used for | Notes |
|--------|--------------|----------|-------|
| ReadMemory_ACE | false (MmCopyVirtualMemory) | ALL gameplay reads (ESP, aimbot, cache) | Safe for freed/invalid pages. Fail-counted. |
| ReadMemory_Raw | true (direct physical read) | ONLY PE scan at startup | No fail-counting, no blocking. |
| WriteMemory_ACE | false (MmCopyVirtualMemory) | All writes (bodyAngles, recoil, FOV, etc.) | Safe. Fail-counted. |

Previous `noncachedread=true` for ACE methods caused BSODs on stale pointers — changed to `false` (MmCopyVirtualMemory).

### get_module (GameAssembly.dll finding)

1. **Method 1 (primary)**: `OpenProcess(PROCESS_QUERY_INFORMATION)` -> `VirtualQueryEx` loop -> `NtQueryVirtualMemory` (class 2 = MemoryMappedFilenameInformation) -> `wcsstr` for module name
2. **Method 2 (fallback — PE scan via ReadMemory_Raw)**:
   - Step 1: Coarse scan 0x7FF000000000-0x7FFF00000000 at 1GB intervals (16 reads), match by timestamp `0x6A4CF525` or size > 0x10000000
   - Step 2: Fine scan +/-512MB around hit at 64KB intervals
   - Step 3: Wide scan from process_base to +32GB at 64KB intervals
   - Step 4: Finer scan from process_base to +4GB at 4KB intervals

### IOCTL stability

- `ioctl_fail_count`: atomic int, cumulative (failures +1, successes -1). Decays by 50 every 500ms via `DecayFailCount()`.
- `IOCTL_FAIL_LIMIT = 500` — blocks all reads when reached
- `ioctl_blocked`: atomic bool, auto-resets after 500ms
- `IOCTL_MAX_SIZE = 0x100000` (1MB)
- Entity cap: 8000
- `g_UseInternalReads`: true when DLL injected into RustClient.exe itself (uses memcpy instead of IOCTL)

### is_valid function

Rejects: 0, 0xCCCCCCCCCCCCCCCC, 0xFFFFFFFFFFFFFFFF, < 0x10000, > 0x7FFFFFFFFFFFFFFF

---

## EAC Restrictions — CRITICAL

**NEVER do any of these before `get_module()` succeeds:**

| Restricted API | Why | Effect |
|---------------|-----|--------|
| `LoadLibraryW(L"shcore.dll")` | New DLL load detected by EAC | Blocks OpenProcess -> "GameAssembly.dll not found" |
| `SetProcessDpiAwareness()` | Process-wide state change | Same EAC trigger |
| `SetProcessDPIAware()` | Process-wide state change | Same EAC trigger |
| `SetThreadDpiAwarenessContext()` | Succeeds in injected DLL (unlike SetProcessDpiAwarenessContext) | DPI state change triggers EAC -> blocks OpenProcess |
| `winmm.dll` import | EAC detects it | Blocks OpenProcess |
| `timeBeginPeriod()` / `timeEndPeriod()` | Forces winmm.dll load | Same as above |
| `CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)` | Triggers EAC detection | Blocks OpenProcess |
| Files with "cheat" in name on C:\ | EAC file scan | Detection |

**DPI awareness**: Use ONLY `SetProcessDpiAwarenessContext((HANDLE)-4)`. It fails silently with `ERROR_ACCESS_DENIED` in injected DLLs (host process already set DPI awareness). This is SAFE — no DPI change, no EAC trigger. Overlay fixes (swap chain ResizeBuffers, ScreenToClient, MonitorFromWindow) still work without DPI awareness.

**"GameAssembly.dll not found" troubleshooting**: Check for ANY restricted API being called before `get_module()` in the init sequence.

---

## Cache System (cache.cpp + cache.hpp)

### EspCacheData struct (cache.hpp:23)

| Field | Type | Purpose |
|-------|------|---------|
| headPos | Vector3 | Player head position (root + 1.8f) |
| feetPos | Vector3 | Player feet/root position |
| spine1Pos | Vector3 | Spine1 bone position |
| velocity | Vector3 | Player velocity (for prediction) |
| bones[15] | Vector3[15] | 15 bone positions (head, neck, spine1, hips, knees, feet, arms, hands) |
| boneAnchorPos | Vector3 | Player position when bones were last read (for offset shifting) |
| name[64] | char[64] | Player display name |
| weaponName[64] | char[64] | Held weapon shortname |
| animalType[32] | char[32] | Animal type string |
| belt[6][64] | char[6][64] | Belt item names |
| wear[5][64] | char[5][64] | Wear item names |
| ammo | int | Current weapon ammo (-1 = unknown) |
| skeletonValid | bool | Bones are valid for rendering |
| isDead | bool | Lifestate == 1 |
| isSleeping | bool | PlayerFlags & Sleeping |
| isWounded | bool | PlayerFlags & Wounded |
| isVisible | bool | Filtered visibility |
| isVisibleRaw | bool | Raw game visibility flag (BaseEntity+0x150) |
| isCrouching | bool | ModelState & Ducked |
| isGrounded | bool | ModelState & OnGround |
| health | float | Current health |
| maxHealth | float | Max health |
| teamId | uint64_t | Current team |
| cacheTick | uint64_t | Last position update time |
| skeletonTick | uint64_t | Last skeleton update time |
| invTick | uint64_t | Last inventory scan time |

### BN chain walk (cache.cpp:133)

1. `GameAssembly + basenetworkable_pointer` -> BaseNetworkable TypeInfo
2. `+ static_fields (0xB8)` -> static_fields
3. `+ wrapper_class (0x8)` -> wrapper_class_ptr
4. `decrypt::networkable_key(wrapper_class_ptr)` -> wrapper_class (ADD-ROL-ADD + Il2cppGetHandle)
5. `+ parent_static_fields (0x10)` -> parent_sf
6. `decrypt::networkable_key2(parent_sf)` -> parent_class (ROL-SUB-XOR-ADD + Il2cppGetHandle)
7. `+ entity (0x20)` -> BufferList
8. `BufferList + 0x18` -> entity_size (capped at 8000)
9. `BufferList + 0x10` -> entity_array
10. `entity_array + 0x20` -> localPlayer (first entity)
11. Iterate `entity_array + 0x20 + (i * 0x8)` for each entity

### Entity classification flow

1. Read prefab name from `objectClass + 0x50` (full path like "assets/prefabs/player/player.prefab")
2. **Player**: keyword match (`player.prefab`, `localplayer`, `assets/prefabs/player/`) AND not corpse/prop/NPC. Fallback: Il2CppClass name contains "BasePlayer"
3. **NPC**: keyword match (`scientistnpc`, `scarecrow`, `tunneldweller`, `underwaterdweller`, `murderer`, `bandit_guard`, `peacekeeper`, `heavyscientist`, etc.)
4. **Animal**: keyword match (`bear`, `wolf`, `boar`, `stag`, `chicken`, `horse`, `panther`, `tiger`, `snake`, etc.)
5. **World**: everything else — matched by prefab name substring patterns

### Prefab name cache

- **Keyed by `objectClass` pointer** (GameObject pointer) — unique per entity INSTANCE (not per type). 100 hemp = 100 cache entries.
- **1024 slots**, reset each cache cycle
- Entries expire after 10 cycles
- Fallback: `objectClass` (GameObject pointer) if klass read fails

### tempEspCache merge logic (cache.cpp:1206)

1. Build `tempEspCache` (unordered_map) during entity iteration
2. After iteration: acquire `g_EspCacheMutex` (unique_lock)
3. **Merge**: Update slow fields (name, weaponName, flags, health, team, belt, wear, ammo), preserve fast fields (bones, headPos if fresher)
4. **Stale removal**: Erase entries not in newKeys set
5. Swap entity/npc/animal/prefab lists via individual mutexes

### Skeleton refresh (cache.cpp:1506)

- TTL based on distance: close (<=20m) = 50ms, medium (20-100m) = 100ms, far (100-300m) = 200ms
- Max 5 players per iteration (prevents IOCTL burst)
- Bone handle cache refreshed every 200ms (saves 2 IOCTLs per bone position read)
- Array-based cache (128 slots, safe for manually mapped DLLs)
- **boneAnchorPos**: Read from player's root position BEFORE reading bones (with +1.8f offset). Stored in BoneCacheEntry.anchorPos. This eliminates the race condition where the position thread (1ms) updates headPos between bone reads and anchor capture.

### Position refresh (cache.cpp:1293)

- Updates headPos (root + 1.8f), feetPos (root), velocity, cacheTick for all players
- Also updates NPC and animal positions
- Reads local player neck transform + crouch state (ModelState::Ducked) into globals

### World ESP prefab patterns (build 24091435 verified)

Patterns match substrings in lowercased full prefab paths.

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

## SDK (sdk.hpp)

### Position methods

- `Get_ObjectPosition()` (line 366): Direct read chain — entity+0x10 -> +0x20 -> +0x20 -> +0x8 -> +0x28 -> +0x90 (6 IOCTLs)
- `Get_ObjectPosition_Fast()` (line 378): Via PositionLerp interpolator — entity+0x1C8 -> +0x20 -> +0x90 (2-3 IOCTLs, falls back to full chain)

### SSE Transform chain

`Transformation::Get_Position_From_Cache()` (line 429): Reads relation_array[index*48], walks dependency_index chain (max 32 iterations), applies quaternion matrix multiplication using SSE intrinsics. Returns Vector3.

`Transformation::Get_Cached_Data()` (line 404): Reads transform_internal+0x28 (some_ptr+index), then some_ptr+0x18 (relation_array+dependency_index_array). Returns CachedTransformData for later use by Get_Position_From_Cache.

### Key functions

- `Get_HeldWeapon()` (line 798): Decrypts ClActiveItem -> targetUid -> searches belt items by ItemId -> returns HeldEntity. Also searches children list by ownerItemUID. Caches result.
- `ResolvePlayerInventory()` (line 729): Tries decrypt method (0) then component walker (1), validates by checking belt has 0-7 items. Caches working method.
- `GetComponentByName(entity, targetName)` (line 691): Walks GameObject component array, reads Il2CppClass name, matches by strcmp.
- `GetPlayerEyes()`: GetComponentByName("PlayerEyes")
- `IsVisibleRawFlag()`: Reads `offsets::BaseEntity::isVisible` (0x150), 1 byte
- `IsVisibleFiltered()`: Bitmask-based visibility filtering with majority consensus. Modes: 0=raw, 1=consensus, 2=raw AND consensus. Uses pointer-based lazy init for static unordered_map.

### Decrypt functions (namespace decrypt, line 76)

- `Il2cppGetHandle(handle_ptr)` (line 107): Resolves GCHandle to object pointer. Reads page_base+0x20 (type), computes slot, checks bitmap, reads entry.
- All decrypt functions use constants from `OffsetManager::DecryptCfg` (runtime overridable via rust_decrypts.dat)

---

## Overlay (render.hpp)

### Key fixes applied

1. **Mouse position**: `ScreenToClient(window_handle, &p_cursor)` converts screen coords to window-relative
2. **No per-frame HWND_TOPMOST**: Topmost re-asserted every 60 frames only
3. **SWP_FRAMECHANGED**: After WS_EX_TRANSPARENT toggle — ensures click-through change takes effect immediately
4. **Swap chain ResizeBuffers**: When overlay window dimensions change, `ResizeBuffers()` + recreate render target view
5. **ShowWindow(SW_SHOW)**: Not `SW_SHOWDEFAULT` (which uses host process STARTUPINFO)
6. **MonitorFromWindow**: Overlay created on the monitor the game window is on
7. **Fullscreen -> borderless**: If exclusive fullscreen detected, forces game window to `WS_POPUP`
8. **WS_EX_NOACTIVATE**: Window never gets focus. Mouse via `GetAsyncKeyState`, keyboard via `GetAsyncKeyState` + `ToAscii`
9. **WS_EX_TRANSPARENT toggle**: Menu open = remove transparent (capture mouse). Menu closed = add transparent (click-through). `SWP_FRAMECHANGED` for immediate effect.

### Screen dimensions

- `g_ScreenW`/`g_ScreenH` (vectorMath.hpp:488) — updated every frame from `GetClientRect(game_window)` in render.hpp Draw()
- Used by WorldToScreen AND ScreenPointValid (visuals.cpp, Cheat.hpp) — single consistent source
- `io.DisplaySize` from ImGui_ImplWin32_NewFrame matches g_ScreenW/H when overlay is synced to game

### Overlay creation — Hijack() (line 1200)

- Finds game window: `FindWindowA("UnityWndClass", "Rust")` with EnumWindows fallback
- Creates overlay: `WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`, `WS_POPUP`
- `DwmExtendFrameIntoClientArea` with margins=-1
- Loads flavor-specific logo (BeaZt/Bomza/BetterCheats)

---

## WorldToScreen (vectorMath.hpp:491)

```cpp
float w = trans_vec.Dot(pos) + matrix._44;
if (w < 0.098f) return Vector2(-1, -1);  // discard behind-camera entities
```

Returns `(-1, -1)` for entities behind camera or too close to near-clip plane. Prevents behind-camera entities from projecting to extreme screen positions ("ESP in the sky" bug).

---

## Aimbot (aimbot.cpp)

- Runs in render thread (not separate thread) — eliminates race condition crashes
- Writes bodyAngles to `PlayerInput + 0x44`
- Bone selection: 0=Head, 1=Neck, 2=Chest, 3=Pelvis, 4=ClosestToCrosshair, 5=Smart
- **Crouch fix**: Uses `g_LocalNeckPos` from globals. Fallback: `g_LocalPlayerPos + (crouching ? 0.9f : 1.5f)`
- Uses cached entity data (zero IOCTLs for target acquisition) + fresh camera matrix for angle calculation
- **Humanize**: JitterAmount (gaussian noise), OvershootAmount (overshoot for small deltas), SmoothingVariance (randomized smoothing), MissProbability (random skip)
- **Prediction**: Velocity-based with gravity compensation (`predicted.y += 4.905 * dt^2 * PredictionScale`)
- **Startup quarantine**: 10-second delay after LocalPlayer change before any aimbot writes
- **SpinWrites**: 3 passes with Sleep(0) between, re-reads input each pass
- **KeepTarget**: If enabled, holds current target while still valid (within FOV using cached bones)

---

## Visuals (visuals.cpp)

### do_Visuals flow

1. Extract cached data (headPos, feetPos, velocity, bones, name, weaponName)
2. **Head/feet correction**: If diff < 0.5f, treat position as feet, add 1.8f for head
3. **Velocity prediction**: If cacheTick age < 500ms and velocity valid, predict forward. Gravity correction for airborne players (0.5 * 9.81 * dt^2)
4. **Bone offset shifting**: `boneOffset = headPos_predicted - cached.boneAnchorPos`, shift all bones by offset. No additional per-bone velocity prediction (removed double prediction bug).
5. **WorldToScreen**: Project headPos and feetPos
6. **Rendering** (in order): FilledBox, CornerBox, Box, HeadCircle, Name, TeamID, Distance, ShowVisibility, Weapon, Hotbar text/icons, Clothing/ItemList/AmmoBar, OFFArrows, SnapLines, Skeleton (4 polyline chains), MovementTrails

### Skeleton ESP rendering

4 polyline chains via `AddPolyline`:
- Chain 1: head -> neck -> spine1 -> hips -> r_knee -> r_foot
- Chain 2: spine1 -> l_hip -> l_knee -> l_foot
- Chain 3: neck -> r_upperarm -> r_forearm -> r_hand
- Chain 4: neck -> l_upperarm -> l_forearm -> l_hand

### Skeleton ESP fixes applied (this session)

1. **boneAnchorPos timing**: Read player root position BEFORE bones, store as `bc.anchorPos` (with +1.8f). Eliminates race where position thread (1ms) updates headPos between bone reads and anchor capture.
2. **Double velocity prediction removed**: Bones previously got prediction applied twice — once via boneOffset (headPos_predicted - boneAnchorPos), once explicitly per-bone. Removed the explicit per-bone prediction block.
3. **Slow cache headPos consistency**: Slow cache now sets `ec.headPos = pos + 1.8f` and `ec.feetPos = pos`, matching the position thread's convention. Previously slow cache set headPos to raw pos (no offset), causing intermittent 1.8m vertical jumps.

---

## Misc (misc.cpp)

### do_misc() — locked by s_miscMutex, 10-second startup quarantine

1. **FOV/Zoom** (line 293): Hotkeys FovChanger (Numpad1), Zoom (Numpad2). Reads ConVar_Graphics -> encrypts FOV -> writes to fovWrite (0x110). Falls back to camera field_of_view (0x170).
2. **BrightNight** (line 320): Resolves TOD_Sky pointer (static_fields -> instances, tries direct + list + scan 0x00-0x200, 30s staleness check). Writes ambientMultiplier to NightParameters+0x5C, AmbientSaturation to AmbientParameters+0x14, forces recalc (UpdateInterval=0.6, timeSinceAmbientUpdate=999.0).
3. **TimeChanger** (line 361): Writes time value to CycleParameters+0x10 or todSky+0x50.
4. **AdminFlags** (line 387): Hotkey F6. Sets PlayerFlags |= IsAdmin (0x4).
5. **DebugCamera** (line 400): Hotkey F7 (requires AdminFlags). 50-second timer. Enables Flying flag, freezes body movement (target_movement=0). WASD movement, Shift=speed, Space/Ctrl=up/down. Swaps loading screen snapshot to hide camera movement.
6. **Recoil** (line 505): Reads children list, finds recoil pointer (BaseProjectile::recoil at 0x3F0). Caches original values. Writes scaled: `original * (100 - RecoilModifier) / 100`, floored at RecoilFloor. RecoilVariance adds +/-10% random. Throttled to 100ms.
7. **Burst mode** (line 682): Sets isBurstWeapon=true, canChangeFireModes=true. Restores when toggled off.

---

## Cheat (Cheat.hpp)

### ReadFrameCameraMatrix() (line 10)

1. Cache native_cam pointer (refreshed every 200 frames)
2. Chain: `GameAssembly + camera_pointer` -> TypeInfo -> +0xB8 (static_fields) -> +0x38 (wrapper_class) -> +0x10 (entity/native_cam)
3. Validate FOV (1.0-180.0, finite)
4. Read camera world position -> g_CameraWorldPos
5. Read viewMatrix (0x2FC)
6. **View-only detection**: If _14, _24, _34 all ~ 0, it's a view matrix — read projectionMatrix (0x18C), multiply view x proj, transpose
7. `src->PublishMatrix(vpT)`

### Do_Cheat() (line 103)

1. Gate: ShutdownRequested, LocalPlayer valid, BnStableCycles >= 3
2. `ReadFrameCameraMatrix()` — FRESH camera matrix each frame
3. Get frameMatrix from `src->GetMatrixSnapshot()`
4. Get lpPos from `g_LocalPlayerPos` (zero IOCTLs)
5. Hotkey processing: CombatMode (F3), BattleMode (F2)
6. **World ESP**: Copy prefab_List (try_to_lock), WorldToScreen, draw text + distance
7. **Player ESP**: Copy entity_List (try_to_lock), copy g_EspCache (shared_lock try_to_lock). Compute g_HotbarTarget. For each player: velocity prediction, WorldToScreen, `esp->do_Visuals()`
8. **NPC ESP**: Copy npc_List, `esp->do_NPC_Visuals()`
9. **Animal ESP**: Copy animal_List, `esp->do_Animal_Visuals()`
10. **Aimbot**: `aim->do_aimbot()` (in render thread, not separate)

### SafeDoCheat() (line 468)

Wraps `Do_Cheat()` in `__try/__except` — logs crash count, 5 crash limit then skips 60 frames.

---

## VisCheck (VisCheck/VisCheck.hpp)

### Modes (ESP::VisMode)

| Mode | Description |
|------|-------------|
| 0 (Raw, default) | Uses game's BaseEntity::isVisible flag (0x150) directly. No mesh data needed. Detects Unity occlusion culling (large occluders like buildings, terrain). Does NOT detect thin trees, fences, dynamic objects. |
| 1 (Consensus) | Bitmask-based majority vote on raw flag samples. Uses pointer-based lazy init. |
| 2 (Strict) | Raw flag AND consensus must both be true. |

### Mesh data (optional, for raycast-based visibility)

- **PhysX** (primary): Reads UnityPlayer.dll + 0x1C3B3D0 -> PxPhysics -> scenes -> actors -> shapes -> geometry. Generates triangles from box shapes (12 tris) and triangle mesh shapes. Single attempt only.
- **.tri file** (fallback): Binary file of Tri structs (36 bytes each). Loaded from DLL dir, LocalAppData, or C:\. Not included with DLL — users must generate separately.
- **BVH**: Median split on longest axis, leaf nodes <= 8 triangles, stack-based iterative traversal (max 64 depth).

### Production note

VisCheck is OFF by default (`ESP::VisCheck = false`). VisMode=0 (raw flag) is the ONLY viable option for production DLL-only distribution. The .tri file (240MB) is not distributed. PhysX offsets may be stale.

---

## Config (Config.hpp)

### Format

- Plain text, `key=value` per line
- Bools: `0`/`1`, Floats: 2 decimal places, Colors: `R,G,B,A` (0-255)
- Path: `%TEMP%\rustcfg.dat` (BeaZt) / `rustcfg_bomza.dat` (Bomza) / `rustcfg_bc.dat` (Better Cheats)

### ApplyLegitDefaults() (line 915)

Safe defaults: ESP Box+Name+Distance+HealthBar+VisCheck, draw_distance=200, Aimbot OFF, RecoilModifier=0, Radar ON (250px), World ores+hemp+stash+backpack+turrets, NPC enabled, Animal enabled, FOV/Zoom OFF.

---

## OffsetManager (OffsetManager.hpp)

- `DecryptConfig` struct with all 18 decrypt constants (build 24091435 defaults)
- `LoadDecryptConfig()`: Tries DLL dir `rust_decrypts.dat` then `C:\rust_decrypts.dat`. Parses `key=value` lines.
- `Initialize()`: Calls LoadDecryptConfig() then ScanAndUpdate() (probes 3 pointers)
- `rust_decrypts.dat` is locked while game runs — delete only when game is closed

---

## RuntimePaths (RuntimePaths.hpp)

### Flavor detection

- `IsBomza()`: `#ifdef BOMZA` or DLL name contains "bomza"
- `IsBetterCheats()`: `#ifdef BETTERCHEATS` or DLL name contains "bettercheats"
- `CurrentModuleNameLower()`: `GetModuleFileNameA(g_hSelfModule)` with `GetModuleHandleExA` fallback (PE header reading for manually mapped DLLs)

### Paths per flavor

| Path | BeaZt | Bomza | Better Cheats |
|------|-------|-------|---------------|
| Config | `%TEMP%\rustcfg.dat` | `%TEMP%\rustcfg_bomza.dat` | `%TEMP%\rustcfg_bc.dat` |
| Log | `%TEMP%\cheat_debug.log` | `%TEMP%\cheat_debug_bomza.log` | `%TEMP%\cheat_debug_bc.log` |
| Mesh | `{DllDir}\rust_mesh.tri` | same | same |
| Decrypts | `{DllDir}\rust_decrypts.dat` | same | same |

---

## Logger (Logger.hpp)

- `g_UpdateLoggingEnabled`: true if `BEAZT_DEBUG` defined, or `C:\rust_debug_enabled.txt` exists, or DLL name contains `_debug`
- ~50 skip patterns filter verbose messages: `CACHE cycle:`, `POS cycle:`, `RECOIL:`, `TOD_DIAG:`, `FOV:`, `BRIGHTNIGHT:`, `GCHANDLE`, `PE scan:`, `CLA_DIAG[`, `INV_RESOLVE:`, `AIMBOT_WRITE[`, etc.
- Only important messages appear: FAIL, CRASH, SKIP, FATAL, Setup, BN chain, CAM_CHAIN, Do_Cheat, Shutdown

---

## Feature Toggles (offsets.hpp)

### Active MISC toggles (only these are implemented)

- `CheatEnabled`, `ShutdownRequested`
- `ChangeBurst`, `RecoilEnabled`, `RecoilModifier`, `RecoilVariance`, `RecoilFloor`
- `FovChanger`, `FovAmount`, `Zoom`, `ZoomAmount`
- `Timechanger`, `BrightNight`, `lightIntensity`, `ambientMultiplier`, `AmbientSaturation`, `timevalue`
- `DebugCamera`, `DebugCameraTimer`, `AdminFlags`, `CombatMode`
- `CrosshairStyle`, `CrosshairSize`, `CrosshairColor`, `DrawFlags`
- `FireRateScale`

### Removed toggles (do NOT re-add — never implemented, dead code)

All weapon exploits (NoSpread, Automatic, Aimsway, NoPunch, RapidFire, SuperMelee, InstantBow, HitSound, InstantEoka, FastBow, NoPullback, InstantCompound, Longhand, NoHeavySlow, NoAnim, NoBob, NoSway, NoLower), all movement exploits (AlwaysSprint, omniSprint, JumpShot, WalkonWalls, NoFall, ShootInAir, ReducedGravity, MoonJump, Spinbot, Fly, SpeedHack, Spiderman, WalkOnWater, InfJump, etc.), all environment modifiers (Rayleigh, Mie, BrightnessEnv, StarMod, MeshMod, AspectRatio, TodColors, TodSunMoon, TodLight, TodRay, TodSky, TodCloud, TodFog, TodAmbient), RemoveLayers (RemoveTrees/Grass/Water/Sky), SilentShot, SilentAim (AIMBOT::Silent), BulletTracers, OccluderDist, SETTINGS::Vsync/Crosshair/ROTSPEED, DebugCamAdminFlags, ShowServerTime, ShowEspRangeOverlay, UsernameBackgroundColour, DistanceBackgroundColor.

### Active SETTINGS toggles

- `BattleMode`, `AdminFlag`, `MenuOpen`, `Language` (0=English, 1=French)

---

## Hotkeys (Hotkeys.hpp)

- Modes: TOGGLE (rising edge latch), HOLD (key down), ALWAYS (always on), DISABLED
- `g_Hotkeys`: `unordered_map<string, HotkeyBind>` — all bindings
- `UpdateHotkeysFrame()`: Called every frame in render.hpp
- `HotkeyPressed()`: Returns true on rising edge (for toggles)
- `IsHotkeyActive()`: Returns true if hold key is down, or toggle is latched
- Conflict detection: `CanAssignHotkey()`, `HasHotkeyConflict()`
- Critical pairs: DebugCam/AdminFlag can't share keys, global.menu/global.shutdown can't share keys

---

## Debugging workflow

1. Enable verbose logging: set `kCacheVerboseLogs = true` in cache.cpp (around line 46), rebuild debug DLL
2. Check `%TEMP%\cheat_debug.log` for: BN chain results, entity counts, prefab names, AIMBOT_WRITE diagnostics
3. After debugging, set `kCacheVerboseLogs = false` and rebuild
4. Logger filter auto-skips verbose messages — only important messages appear in production log
5. Debug DLL: `Rust Prv Ext_debug.dll` — logging always on, no marker file needed

---

## Constraints

- All game memory reads through kernel driver IOCTL — never direct memory access (unless `g_UseInternalReads` = true)
- `rust_decrypts.dat` is locked while game runs — delete only when game is closed
- DLL output, not EXE — injected via loader
- No tests/typecheck/lint — only CI step is MSBuild compile
- Camera matrix read FRESH in render thread (not cached)
- Aimbot runs in render thread (not separate thread)
- Skeleton thread caps at 5 players per iteration
- Function-local `static std::unordered_map` CRASHES in manually mapped DLLs — use pointer-based lazy init
- ReadMemory_ACE uses `noncachedread=false` (MmCopyVirtualMemory) — safe for freed/invalid pages. Previous `noncachedread=true` (direct physical read) caused BSODs.
- ReadMemory_Raw uses `noncachedread=true` (direct physical read) — used ONLY for PE scan, no fail-counting
- Production builds self-delete the DLL on shutdown via `del.bat` in `%TEMP%`
- Production builds delete ALL trace files on shutdown (logs, configs, markers, decrypts)

---

## Cleanup History

### Session changes (this stable build)

1. **Skeleton ESP fixes**: boneAnchorPos timing (read position before bones), removed double velocity prediction, slow cache headPos consistency (+1.8f)
2. **AnyBrain removed entirely**: Deleted anybrain.hpp, removed ALL toggles/menu items/config save-load. Was dead code (RVAs 0x0, morphine dump doesn't have class, method names obfuscated).
3. **Dead code cleanup**: Deleted Occlusion.hpp (95 lines dead), tracers.hpp (never called). Removed 14 dead functions from sdk.hpp, 12 dead offset constants, 4 dead namespaces, ~80 dead MISC toggles, 158 dead Config.hpp save/load lines. Removed AIMBOT::Silent, ESP::OccluderDist, ESP::BulletTracers, SETTINGS::Vsync/Crosshair/ROTSPEED, g_LocalPlayerEyesPtr, isAabbOccluded field, tempOccluders vector.
4. **Git merge conflict markers**: Resolved 21 conflict markers across 8 code files + AGENTS.md (kept HEAD for all).
5. **sdk.hpp**: Fixed `IsVisibleRawFlag()` to use `offsets::BaseEntity::isVisible` instead of hardcoded `0x150`.
6. **Menu files**: Added `#include "../sdk.hpp"` to all 3 menu files (was transitively included via tracers.hpp which was deleted).
