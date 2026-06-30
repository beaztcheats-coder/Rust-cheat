# Offsets & Auto-Update Guide

## Overview

The cheat has a partial auto-offset system. After a Rust update:

- **80% of offsets** (field offsets within classes) are **hardcoded** in `offsets.hpp` — they rarely change but need manual update when they do.
- **15% of offsets** (TypeInfo static pointers) are **hardcoded** in `offsets.hpp` but **validated at startup** — the cheat checks if they're still valid and logs failures.
- **5% of offsets** (decrypt constants) are **loaded from `C:\rust_decrypts.dat`** — you can edit this file and reinject the DLL **without recompiling**.

The cheat logs everything to `C:\cheat_debug.log`. Always check this file first after a patch.

---

## File Map — Where Every Offset Lives

| File | What It Stores | Examples | Recompile After Change? |
|------|---------------|----------|------------------------|
| `offsets.hpp` lines 8-11 | 4 TypeInfo pointers (RVA from GameAssembly.dll base) | `basenetworkable_pointer = 0xE2D5ED8` | **Yes** |
| `offsets.hpp` lines 13-72 | Il2Cpp class field offsets & chain offsets | `PlayerFlags = 0x670`, `static_fields = 0xB8` | **Yes** |
| `offsets.hpp` lines 102-125 | Weapon/recoil `#define` offsets | `oRecoilYawMin = 0x18`, `oAimCone = 0x3E0` | **Yes** |
| `OffsetManager.hpp` lines 10-37 | Embedded decrypt constant defaults (fallback) | `nk_rol = 0x2` | **Yes** |
| `C:\rust_decrypts.dat` | **Live** decrypt constants (loaded at runtime) | `nk_rol=0x2` | **NO** — edit file, reinject |
| `sdk.hpp` | Decrypt function logic (uses constants from config) | `OffsetManager::DecryptCfg.nk_rol` | **No** |
| `misc.cpp` line 24 | FOV TypeInfo pointer (`ConVar_Graphics`) | `0xE251748` | **Yes** |

### What NEVER needs updating
- The menu UI (`Render/render.hpp`)
- ESP rendering (`Hacks/Visuals/visuals.cpp`)
- Aimbot logic (`Hacks/Aimbot/aimbot.cpp`)
- Misc features logic (`Hacks/Misc/misc.cpp`) — just the encrypt constants
- Driver code (`Driver.hpp`)
- Overlay code (`Overlay.hpp`)
- Config system

---

## Step-by-Step Post-Update Workflow

### STEP 1 — Inject the DLL as normal

Inject the DLL into Rust. The cheat auto-starts. Go in-game (main menu is fine).

### STEP 2 — Check the cheat log

Open `C:\cheat_debug.log`. Find the `[OffsetMgr]` section near the top. You'll see something like:

```
=== OffsetManager Initializing ===
No C:\rust_decrypts.dat found. Using embedded decrypt defaults.
[OffsetMgr] Il2CppGetHandle resolved: 0xE6A13E0
[OffsetMgr] il2cpp_class_from_name found at: 0x7FFA1B003400
[OffsetMgr] BaseNetworkable validated at RVA 0xE2D5ED8 -> 0x1F3A8B000
[OffsetMgr] BaseCamera validated at RVA 0xE2C22B0 -> 0x1F3A7C000
[OffsetMgr] TOD_Sky validated at RVA 0xE28EB78 -> 0x1F3A54000
[OffsetMgr] Initialization complete. 4 pointers validated.
```

**Good**: All 4 pointers say "validated" → TypeInfo pointers are fine, skip to STEP 5.

**Bad**: Any pointer says "FAILED" → go to STEP 3.

### STEP 3 — Fix broken TypeInfo pointers

If you see `[OffsetMgr] BaseNetworkable pointer validation FAILED.`:

1. **Get the new RVA value** from UnknownCheats Rust dump thread, or run Il2CppDumper (see section below).

2. Open `offsets.hpp` and update the failed pointer:
   ```cpp
   // offsets.hpp lines 8-11
   namespace offsets {
       inline uint64_t basenetworkable_pointer = 0xE2D5ED8;  // <-- update this
       inline uint64_t camera_pointer = 0xE2C22B0;           // <-- update this
       inline uint64_t Il2cppGetHandle = 0xE6A13E0;          // <-- update this
       inline uint64_t TOD_Sky_TypeInfo = 0xE28EB78;         // <-- update this
   ```

3. **Rebuild the DLL** — you need MSBuild:
   ```
   "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "F:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /m
   ```
   Output: `F:\github\rust\x64\Release\Rust Prv Ext.dll`

4. Reinject and check the log again.

### STEP 4 — Check if field offsets are broken

Go in-game and test these features. If any fail:

| Feature broken | Offset to check | Class to search in dump.cs |
|---------------|-----------------|---------------------------|
| No player names on ESP | `DisplayName = 0x330` | BasePlayer |
| Health bar shows wrong values | `Health = 0x29c`, `MaxHealth = 0x2a0` | BaseCombatEntity |
| Team ID shows wrong team | `CurrentTeam = 0x4f0` | BasePlayer |
| Held item/weapon name fails | `ClActiveItem = 0x520` | BasePlayer |
| No entities in list | `Entity chain offsets` | BaseNetworkable |
| Aimbot doesn't write angles | `PlayerInput = 0x2e8` | BasePlayer |
| Debug camera doesn't work | `BaseMovement = 0x540`, `PlayerFlags = 0x670` | BasePlayer |

If any offset shifted:

1. Run Il2CppDumper (see below) to get new values.
2. Update `offsets.hpp` with new values.
3. Rebuild the DLL.

### STEP 5 — Check if decrypt constants are broken

In-game, test:
- **Held Item in ESP** — if weapon name shows "Empty" or wrong: `cla_*` constants broken
- **Entity list is empty** — no players/NPCs visible: `nk_*` or `nk2_*` constants broken
- **PlayerInventory doesn't work**: `inv_*` broken
- **OFFArrows broken**: `ey_*` broken
- **FOV changer doesn't work**: `fov_*` broken

If any are broken:

1. Get new decrypt constants (UnknownCheats or manual RE).
2. Edit `C:\rust_decrypts.dat` with new values.
3. **No recompile needed** — just reinject the DLL.

### STEP 6 — Test everything

In-game checklist:
- [ ] ESP boxes/skeletons render on players
- [ ] ESP renders on NPCs (scientists)
- [ ] ESP renders on animals
- [ ] World ESP shows resources (hemp, ore)
- [ ] Health bars show correctly
- [ ] Player names show correctly
- [ ] Held weapon names show
- [ ] Team IDs show correctly
- [ ] Aimbot activates (crosshair moves to target)
- [ ] FOV changer works
- [ ] No recoil works (hold fire, crosshair stays still)
- [ ] No spread works (bullets go center)
- [ ] Debug camera works (F5, noclip fly)
- [ ] Radar shows player positions
- [ ] Time changer works
- [ ] Bright Night works

---

## Running Il2CppDumper — Full Walkthrough

### What is Il2CppDumper?

A tool that reads Unity's Il2Cpp metadata and outputs every class, field, method, and their memory offsets. It produces a `dump.cs` file you can open in any text editor.

### Step-by-step

**1. Find your Rust game files**

Steam default:
```
GameAssembly.dll:
  C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll

global-metadata.dat:
  C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat
```

If you installed Rust elsewhere, search for `RustClient.exe` and look in that folder.

**2. Open Command Prompt**

Press `Win+R`, type `cmd`, press Enter. Then navigate:
```
cd F:\github\rust\dumper
```

**3. Run Il2CppDumper**

```
Il2CppDumper.exe "C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll" "C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat" "C:\rust_dump\"
```

(If `C:\rust_dump\` doesn't exist, create it first: `mkdir C:\rust_dump`)

**4. Wait for it to finish**

It will print progress. Takes 30-60 seconds for Rust. When done, `C:\rust_dump\` contains:
- `dump.cs` — C# stubs with all class/field offsets

**5. Open dump.cs**

Open in Notepad or VS Code. It's a large file (~50MB). Use Ctrl+F to search.

**6. Find TypeInfo pointers**

Search for a class name, e.g. `BaseNetworkable`. You'll see something like:
```csharp
// Namespace: 
public class BaseNetworkable : BaseMonoBehaviour // TypeDefIndex: 1234
{
    // Fields
    ...
}
```

The `TypeDefIndex` is NOT the RVA. The TypeInfo RVA (what goes in `offsets.hpp` line 8) is **not directly shown** in dump.cs. You need:

- **Option A**: Look at the UnknownCheats Rust dump thread — they post the exact RVA values.
- **Option B**: Use `script.json` instead of `dump.cs` and search for the TypeInfo address.
- **Option C**: The cheat validates pointers at startup. If the old value still works, keep it. If the log says "FAILED", get the new value from UnknownCheats.

**7. Find field offsets**

In `dump.cs`, search for the class name. Each field shows its hex offset:

Example — searching for `BasePlayer`:
```csharp
public class BasePlayer : BaseCombatEntity
{
    public PlayerEyes eyes; // 0x3E0
    public PlayerInventory inventory; // 0x3A8
    public PlayerInput input; // 0x2E8
    public float playerFlags; // 0x670
    public uint clActiveItem; // 0x520
    public float currentTeam; // 0x4F0
    public string _displayName; // 0x330
    ...
}
```

**Copy the hex values** (e.g., `0x670`) into `offsets.hpp`:
```cpp
namespace BasePlayer {
    inline uint64_t PlayerFlags = 0x670;  // <-- update this
}
```

**8. Find weapon/recoil offsets**

Search for `RecoilProperties`:
```csharp
public class RecoilProperties
{
    public float recoilYawMin; // 0x18
    public float recoilYawMax; // 0x1C
    public float recoilPitchMin; // 0x20
    public float recoilPitchMax; // 0x24
    public float aimconeCurveScale; // 0x60
    ...
}
```

Search for `BaseProjectile`:
```csharp
public class BaseProjectile : AttackEntity
{
    public float aimCone; // 0x3E0
    public float hipAimCone; // 0x3E4
    public float aimconePenalty; // 0x3EC
    public float stancePenalty; // 0x3F8
    public float sightAimConeScale; // 0x43C
    ...
}
```

**9. After updating offsets.hpp — REBUILD THE DLL**

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" "F:\github\rust\Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /m
```

---

## Decrypt Constants — Format & Update

### File: `C:\rust_decrypts.dat`

This file is **auto-created** on the first injection. It contains 20 decrypt/encrypt constants in key=value format.

### Current values (embedded defaults in OffsetManager.hpp):

```
nk_rol=0x2
nk_xor=0x111B9118
nk_add=0x79300E2E
nk2_rol=0x6
nk2_xor=0xC5D748E1
nk2_add=0x48498B34
cla_add=0x9420FF13
cla_rol=0x10
cla_add_2=0xEDC489FD
cla_rol_2=0x6
fov_rol=0x1F
fov_sub=0x270C775
fov_xor=0x93DAED4D
inv_rol=0x19
inv_sub=0x249D878C
inv_xor=0x58D82066
inv_add=0x7CD2A7CE
ey_sub=0xC26F5B3
ey_xor=0x6EC84F5D
ey_rol=0xD
```

### Which constant affects which feature

| Constant prefix | Game system | What breaks if wrong |
|----------------|-------------|---------------------|
| `nk_*`         | BaseNetworkable chain decrypt | **No entities at all** — ESP blank, aimbot no targets |
| `nk2_*`        | Parent class decrypt | Same as above |
| `cla_*`        | ClActiveItem UID decrypt | Held item name shows "Empty" or wrong |
| `ey_*`         | PlayerEyes pointer decrypt | OFFArrows, some bone lookups fail |
| `inv_*`        | PlayerInventory pointer decrypt | Hotbar/Belt inventory not readable |
| `fov_*`        | FOV value encryption | FOV changer & Zoom don't work |

### How to update decrypt constants

1. Get new values from UnknownCheats (Rust update thread) or manual RE.
2. Open `C:\rust_decrypts.dat` in Notepad.
3. Change the values. The ROL values are **rotation bit counts** (e.g., `nk_rol=29` means rotate-left by 29 bits).
4. Save the file.
5. **Reinject the DLL** — no recompile needed.
6. Check the log: `[OffsetMgr] Decrypt config loaded from C:\rust_decrypts.dat`

### If decrypt constants are ALSO in OffsetManager.hpp

The embedded defaults in `OffsetManager.hpp` lines 10-37 are the **fallback** when `C:\rust_decrypts.dat` doesn't exist. If you want the defaults to match, update both:
1. `OffsetManager.hpp` lines 10-37 (recompile needed)
2. `C:\rust_decrypts.dat` (no recompile needed)

But normally you only need to edit `C:\rust_decrypts.dat` and reinject.

---

## Reading the Cheat Log

### Example 1: Everything healthy

```
=== OffsetManager Initializing ===
Decrypt config loaded from C:\rust_decrypts.dat
[OffsetMgr] Il2CppGetHandle resolved: 0xE6A13E0
[OffsetMgr] il2cpp_class_from_name found at: 0x7FFA1B003400
[OffsetMgr] BaseNetworkable validated at RVA 0xE2D5ED8 -> 0x1F3A8B000
[OffsetMgr] BaseCamera validated at RVA 0xE2C22B0 -> 0x1F3A7C000
[OffsetMgr] TOD_Sky validated at RVA 0xE28EB78 -> 0x1F3A54000
[OffsetMgr] Initialization complete. 4 pointers validated.
[OffsetMgr] Decrypt config saved to C:\rust_decrypts.dat
```

**Status**: All good. No action needed.

### Example 2: Pointer validation failed

```
=== OffsetManager Initializing ===
No C:\rust_decrypts.dat found. Using embedded decrypt defaults.
[OffsetMgr] Il2CppGetHandle resolved: 0xE6A13E0
[OffsetMgr] il2cpp_class_from_name NOT FOUND.
[OffsetMgr] BaseNetworkable pointer validation FAILED.
[OffsetMgr] BaseCamera pointer validation FAILED.
[OffsetMgr] TOD_Sky pointer validation FAILED.
[OffsetMgr] Initialization complete. 1 pointers validated.
```

**Status**: ALL TypeInfo pointers are stale. Game updated. You need to:
1. Get new RVA values from UnknownCheats or Il2CppDumper
2. Update `offsets.hpp` lines 8-11
3. Rebuild DLL

### Example 3: Decrypt loaded from file

```
=== OffsetManager Initializing ===
Decrypt config loaded from C:\rust_decrypts.dat
...
```

Good — decrypt constants were read from the file. If the file exists, its values override the embedded defaults.

---

## Troubleshooting

| Problem | Likely Cause | Fix |
|---------|-------------|-----|
| **No entities at all** | `nk_*` or `nk2_*` decrypt constants wrong | Update `C:\rust_decrypts.dat`, reinject |
| **ESP boxes wrong position** | Bone/transform offsets shifted | Run Il2CppDumper, update field offsets |
| **Weapon name shows "Empty"** | `cla_*` decrypt constants wrong OR `Get_HeldItem()` chain broken | Update decrypt constants first; if still broken, update `ClActiveItem` offset |
| **FOV changer does nothing** | `fov_*` encrypt constants OR `ConVar_Graphics` TypeInfo pointer wrong | Update `C:\rust_decrypts.dat`; if still broken, update `misc.cpp` line 24 |
| **Il2CppDumper says "ERROR: metadata not found"** | `global-metadata.dat` path wrong | Check the path — new Rust versions may move it |
| **Il2CppDumper crashes** | GameAssembly.dll is LZ4-compressed | Use `Cpp2IL` instead (https://github.com/SamboyCoding/Cpp2IL) |
| **Cheat crashes on injection** | Driver not loaded or offsets completely wrong | Check driver is running; verify offsets against dump |
| **"No C:\rust_decrypts.dat found" in log** | First run, file not created yet | The cheat creates it on first injection. If it doesn't, the auto-save may have failed — copy the values from OffsetManager.hpp manually |

---

## File Layout Reference

```
F:\github\rust\
├── offsets.hpp              ← ALL field offsets + TypeInfo pointers (recompile)
├── OffsetManager.hpp        ← Decrypt config defaults + auto-detection
├── PatternScanner.hpp       ← Byte pattern scanner for Il2CppGetHandle
├── sdk.hpp                  ← Decrypt functions (uses DecryptCfg)
├── Hacks/Misc/misc.cpp      ← FOV encrypt function + misc features
├── Hacks/Aimbot/aimbot.cpp  ← Aimbot logic
├── Hacks/Visuals/visuals.cpp← ESP rendering
├── Hacks/Cheat/Cheat.hpp    ← Main loop (ESP, aimbot, misc dispatch)
├── Hacks/Cache/cache.cpp    ← Entity list population
├── Config.hpp               ← Settings save/load
├── Logger.hpp               ← Logging to C:\cheat_debug.log
├── Rust Prv Ext.cpp         ← Entry point, initializes OffsetManager
│
├── dumper/
│   ├── Il2CppDumper.exe     ← Runs against GameAssembly.dll
│   ├── README.txt           ← Quick start for Il2CppDumper
│   └── *.dll, *.py          ← Dependencies
│
├── offsets/
│   └── README.md            ← THIS FILE
│
├── x64\Release\
│   └── Rust Prv Ext.dll     ← Built DLL output
│
└── Runtime files:
    C:\rust_decrypts.dat     ← Decrypt constants (edit, no recompile)
    C:\rustcfg.dat           ← User settings (config)
    C:\cheat_debug.log       ← Runtime diagnostic log
    C:\beazt_bg.png          ← Menu background image
```
