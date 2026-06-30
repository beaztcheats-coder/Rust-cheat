# BEAZT — Rust External Cheat

External Rust cheat DLL (x64, C++20, VS2022) injected via loader into Rust (Steam) Unity 6 Il2Cpp. ImGui D3D11 overlay menu. Reads/writes game memory through a kernel driver (`\\.\Bfo64` IOCTL).

---

## Quick Start

```
1.  .\build_both.bat          # Build everything (both DLLs + injector + spoofer)
2.  .\inject_lite.bat          # Inject Lite variant  (or inject_private.bat)
3.  Press INSERT to open menu  # Configure features
```

---

## Architecture

| File | Role |
|------|------|
| `Rust Prv Ext.cpp` | `DllMain`, `MainThread`, init sequence, export functions |
| `Driver.hpp` | `DriverInterface`, `read<T>`/`write<T>` templates, process/module finder |
| `sdk.hpp` | Decrypt functions, SSE transform chain, `Get_Position`, `Get_HeldItem`, visibility pipeline |
| `offsets.hpp` | Game offsets, feature toggle globals, color presets |
| `OffsetManager.hpp` | Decrypt config struct, `LoadDecryptConfig`/`SaveDecryptConfig`, embedded defaults |
| `Config.hpp` | Save/Load config to `C:\rustcfg_lite.dat` / `C:\rustcfg_private.dat` (key=value) |
| `Logger.hpp` | File logging to `C:\cheat_debug_lite.log` / `C:\cheat_debug_private.log` |
| `Hotkeys.hpp` | Hotkey binding system (toggle/hold/always modes) |
| `RuntimePaths.hpp` | Detects Lite/Private variant by DLL filename, returns config/log paths |
| `Hacks/Cache/cache.cpp` | BN entity chain walk, entity classification, skeleton/ESP cache |
| `Hacks/Aimbot/aimbot.cpp` | Memory aimbot (writes `bodyAngles` to `PlayerInput+0x44`) |
| `Hacks/Visuals/visuals.cpp` | Box/Skeleton/Name/Distance/Health/Head ESP rendering |
| `Hacks/Misc/misc.cpp` | Debug cam, NoRecoil, weapon features, FOV changer, BrightNight |
| `Hacks/Misc/spoofer.cpp` | Embedded HWID spoofer (runs via `\\.\Bfo64` IOCTL before cheat starts) |
| `Hacks/Cheat/Cheat.hpp` | `Do_Cheat()` main loop — iterates entity lists, dispatches to visuals/aimbot/misc |
| `Render/render.hpp` | ImGui D3D11 overlay: `Hijack()`, `Draw()`, `Menu()` (10-page UI) |

---

## Full Feature List

### Dashboard
- FPS counter, active config name, dirty/saved/loaded indicators
- Language selector (English / Francais)
- Panic button (disables all features, closes menu)

---

### Combat (Aimbot)

**Memory Aimbot:**
- Memory Aimbot toggle (writes aim angles — HIGH EAC risk)
- Silent Aim (writes bodyRotation quaternion — HIGH EAC risk)
- Aim Key bind (hold/toggle/always modes)
- Smoothing (1.0 – 20.0)
- FOV Size (1 – 2000)
- Keep Target
- Target Filter (Vis) — only aim at visible players
  - Strict Visibility — require raw flag + filtered consensus
- Ignore Team, Ignore Wounded, Ignore Sleepers
- Weapon Check — only aim at weapon/melee holders
- Max Aim Distance (0 – 500m)
- Bone Priority: Head, Neck, Chest, Pelvis, Closest to Crosshair, Smart
- Aim Bone: Head (53), Neck (52), Spine (23), Pelvis (0)
- Prediction (0x – 2.0x scale, with gravity compensation)
- Spin Writes (multiple angle writes per frame — HIGH EAC risk)

**Visual:**
- FOV Circle (color configurable)
- FOV Outline
- Prediction Indicator
- Target Line (color + thickness)
- Target Icon (color)

**Recoil Mod:**
- Recoil Mod toggle (writes recoil properties — HIGH EAC risk)
- Recoil Mod % (0 – 100%)

---

### Player ESP

**Master Toggle:** Enable Player ESP

**Visuals:**
- Box (color, outline toggle, thickness 1–5)
- Corner Box
- Filled Box (color + alpha)
- Skeleton (color, outline, thickness 1–5)
- Head Circle (color)
- Snaplines (color, outline, thickness 1–5, mode: Off/Bottom/Middle/Top)
- OFF Arrows

**Info:**
- Name (color)
- Distance (color, bracket style: 0/1/2/3)
- Held Item (color)
- Team ID
- Hotbar Text

**Visibility Check:**
- Visibility Check toggle
- Vis Mode: Flag / Hybrid / Strict
- Vis Samples (1 – 10)
- Vis Hold (0 – 500ms)
- Visible/Invisible colors (box, skeleton visible/invisible)

**Filters:**
- Ignore Team, Ignore Wounded, Ignore Sleepers
- Remove Wounded, Remove Sleepers, Remove Team (hide from ESP)
- Highlight Wounded, Highlight Sleepers (color override)
- Advanced Distances (per-feature distance caps: Box, Skeleton, Name, Weapon, HealthBar, Snapline, Head Circle, OFF Arrow)
- Draw Distance (global, 0 – 2000m)

**Style:**
- ESP Font Size (0.5 – 2.0)
- Movement Trails
- Bullet Tracers (color, thickness, outline)
- Text Background

---

### NPC ESP

**Master Toggle:** Enable NPC ESP

- Box, Skeleton, Name, Distance, Health, Held Item, Snaplines
- Snapline Mode (Off/Bottom/Middle/Top)
- Global draw distance + per-feature advanced distances
- NPC type filters: Scientists, Tunnel Dweller, Underwater Dweller

---

### Animal ESP

**Master Toggle:** Enable Animal ESP

- Box, Name, Distance, Health, Snaplines
- Snapline Mode (Off/Bottom/Middle/Top)
- Global draw distance + per-feature advanced distances
- Animal type filters: Bear, Wolf, Boar, Stag, Chicken, Horse, Polar Bear, Panther, Tiger, Snake

---

### World ESP

- Ore resources: Stone, Metal, Sulfur (with pickup indicators)
- Collectibles: Hemp, Wood
- Deployables: Stash, Body Bag, Backpack, Dropped Items
- Turrets: Auto Turret, Shotgun Trap, Flame Turret, SAM Site
- Vehicles: MiniCopter, Bradley APC, Rowboat, RHIB, Kayak, Tugboat, Submarine, Transport Heli, Attack Heli, Balloon, Motorbike, Motorbike Sidecar, Trike, Bicycle, Snowmobile
- Traps: Bear Trap, Landmine
- Base objects: Lockers, Bags, Beds, TC, Vending Machine, Workbench, Large Storage, Ladder, Generator, Battery
- Loot crates: Normal, Military, Elite, Locked, Medical, Food
- Barrels: Beige, Blue, Fuel
- World objects: Cargo Ship, Shark
- Global draw distance (200 – 5000m) + per-item draw distances
- Show Distance toggle

---

### Radar

- Enable Radar toggle
- Size (50 – 500), Scale (0.25 – 5.0), Dot Size (1 – 10)
- Show Players, Show NPCs, Show Animals
- Hide Sleepers
- Position presets (0–5)
- Show Grid toggle
- Rotate with player toggle
- Background opacity (0 – 1.0)
- Colors: Players, NPCs, Animals, Border

---

### Visuals

**Camera:**
- FOV Changer (30 – 150), Zoom (configurable amount)
- Crosshair (style: 0–4, size, color)

**Screen:**
- Screen Info (server time display)
- Draw Flags

**Environment:**
- Bright Night toggle
- Ambient Multiplier (0 – 20)
- Ambient Saturation (0 – 5)

**TOD Ambient:**
- TOD Color Changer (Sun/Moon, Light, Ray, Sky, Cloud, Fog, Ambient)
- Rayleigh modifier, Mie modifier, Brightness, Star modifier, Mesh modifier

**World Removal (Experimental):**
- Remove Trees, Remove Grass, Remove Water, Remove Sky
- Removal Layers Active toggle
- Aspect Ratio modifier

---

### Utility

- Admin Flags (writes `IsAdmin` flag — HIGH EAC risk)
- Battle Mode (one-key combat profile with filter settings)
- Battle Mode Filters: Players ESP, Aimbot, World ESP, NPC ESP, Animal ESP, Radar
- Debug Camera (50s timeout, requires Admin Flags, bind key in Settings > Keybinds)
- Debug Camera HUD countdown timer
- Force Admin Flags during Debug Camera option

---

### Settings

**Configs:**
- Saved configs dropdown (auto-scans `C:\` for `.dat` config files)
- Config name input + Save/Load named config
- Quick Actions: Save Current, Load Default, Reset All
- Active config path display
- Config dropdown auto-refreshes after Save/Load

**Keybinds:**
- Per-feature key bind system (toggle/hold/always/disabled)
- Search binds by name
- Page filter dropdown
- Conflict detection (red highlight)
- Clear All / Reset to defaults
- Columns: Feature Name, Page, Key, Mode, Delete

**Appearance:**
- UI Scale (50 – 200)
- Menu Opacity (0.50 – 1.00)
- Border Glow, Animation Speed
- Theme selector (0–3), Accent color
- Compact Mode, Advanced Mode, Tooltips toggles
- Watermark, Show FPS, Hide Menu on Panic
- Performance Mode, Disable Animations
- Background FX: Animated BG, Matrix Particles, Lightning FX
- Matrix Density, Background Opacity, FX Intensity

---

## Build Pipeline

| Script | Output |
|--------|--------|
| `build_both.bat` | Full pipeline: Lite DLL → Private DLL → Injector |
| `build_vsharp.bat` | `x64\Release\vsharp.dll` (Vsharp brand) |
| `build_lite.bat` | `x64\Release\Rust Private Lite.dll` |
| `build_private.bat` | `x64\Release\Rust Private.dll` |
| `build_injector.bat` | `x64\Release\injector.exe` |

Requirements:
- Visual Studio 2022 Community (v143 toolchain)
- DirectX SDK (included in repo: `Microsoft DirectX SDK/`)
- Windows 10/11 x64

---

## Configuration

### User Settings
| Path | DLL Variant |
|------|-------------|
| `C:\rustcfg_vsharp.dat` | Vsharp |
| `C:\rustcfg_lite.dat` | Lite |
| `C:\rustcfg_private.dat` | Private |
| `C:\rustcfg_<name>.dat` | Named configs |

### Decrypt Constants
| Path | Purpose |
|------|---------|
| `C:\rust_decrypts.dat` | Decrypt constants (auto-generated from embedded defaults if missing) |

### Logs
| Path | DLL Variant |
|------|-------------|
| `C:\cheat_debug_vsharp.log` | Vsharp |
| `C:\cheat_debug_lite.log` | Lite |
| `C:\cheat_debug_private.log` | Private |

---

## Updating for New Game Patches

When Rust updates, use the master update pipeline to automatically fetch new offsets and decrypts:

### Quick Update (automated)
```
1. cd tools\masterupdate
2. auto_update.bat     # Fetches Morphine + sha-dumper, generates patches
3. update_now.bat      # Applies patches + builds DLL
4. Inject and test
```

### Manual Update (via opencode)
```
1. cd tools\masterupdate
2. auto_update.bat
3. Paste output\super_prompt.txt into opencode
4. opencode applies all changes
5. Build with build_vsharp.bat (or build_lite.bat / build_private.bat)
```

See `tools/masterupdate/README.md` for full pipeline documentation.

---

## Dumper Tools

### Il2CppDumper (tools/dumper/)
Static IL2CPP offset dumper. Works offline against dumped GameAssembly.dll + global-metadata.dat.

```
Usage:
  Il2CppDumper.exe <GameAssembly.dll> <global-metadata.dat> <output_dir>
  Or just run dump_offsets.bat from repo root.

Output:
  dump.cs        — C# class/field dumps with offsets
  script.json    — JSON metadata (methods, strings, addresses)
  il2cpp.h       — C header with struct definitions
  DummyDll/      — restored .NET DLL stubs

Scripts (tools/dumper/scripts/):
  ida.py, ida_py3.py                    — IDA Pro method/string labeling
  ida_with_struct.py, ida_with_struct_py3.py — IDA Pro with type signatures
  ghidra.py, ghidra_wasm.py             — Ghidra method/string labeling
  ghidra_with_struct.py                 — Ghidra with C type signatures
  il2cpp_header_to_ghidra.py            — Convert il2cpp.h to Ghidra-compatible format
```

### Frida IL2CPP Bridge (tools/frida/)
Runtime field dumper. Attaches to live RustClient.exe process.

```
Prerequisites:
  - RustClient.exe running WITHOUT EAC
  - Python 3.14 with frida pip package
  - frida-il2cpp-bridge at C:\frida-il2cpp\
  - Run: .\tools\frida\frida_dump.bat

Output: F:\github\rust\frida_dump.txt (all assemblies, classes, fields)

Scripts:
  dump_full.js      — Full IL2CPP bridge dump (ES module)
  dump_il2cpp.js   — Raw memory field dumper (no bridge, reads Il2CppClass directly)
  frida_run.py     — Python Frida host (attaches, loads bridge, captures output)
```

---

## Local Dev Server

Launch a Rust dedicated server locally for testing (no EAC, no bans).

```
# 1. Install/update server (one-time)
.\tools\server\update_server.bat

# 2. Launch server
.\tools\server\launch_server.bat
# Or with progress bar: tools\server\launch_server_progress.ps1

# 3. Connect from Rust client
#    Press F1 in-game, type:
#    client.connect localhost:28015
```

Server config: seed 9999, worldsize 2000, EAC disabled, port 28015.
Server install path: `C:\steamcmd\steamapps\common\rust_dedicated\`

---

## HWID Spoofer

The embedded spoofer runs before cheat initialization. It prompts the user via MessageBox and, if confirmed, executes HWID spoof operations via the loaded kernel driver (`\\.\Bfo64`).

### What it spoofs (11 operations)
| # | Operation | Description |
|---|-----------|-------------|
| 1 | Disk Serials | Spoofs disk drive serial numbers |
| 2 | MAC Addresses | Spoofs network adapter MAC addresses |
| 3 | GPU HWID | Spoofs GPU hardware identifiers |
| 4 | SMBIOS Data | Spoofs system BIOS/DMI data |
| 5 | Boot Config | Spoofs boot configuration data |
| 6 | Firmware Entries | Spoofs firmware table entries |
| 7 | ARP Cache | Clears/spoofs ARP cache entries |
| 8 | SMBIOS Nulls | Nullifies SMBIOS null entries |
| 9 | TPM Identifiers | Nullifies TPM hardware identifiers |
| 10 | Monitor Serials | Spoofs monitor EDID serials |
| 11 | Registry HWID | Spoofs registry-based HWID keys |

### Status reporting
Console prints summary after spoof:
```
Spoofer status: 8/11 successful (11 attempted, seed: set)
```
Fields: `okCount/total (attemptedCount, seed: set/failed, mismatch: true/false)`

### Cleanup (post-spoof)
- TPM key wiping (registry deletions)
- DNS cache flush
- Winsock reset
- WMI service restart

### Standalone spoofer
The embedded spoofer runs automatically before cheat initialization (see above). No standalone EXE needed.

---

## Injector

The injector (`injector.exe`) prompts for:
1. Variant selection: Lite or Private DLL
2. HWID spoofer: Yes/No (runs `spoofer.exe` before injection)
3. Command-line flags: `private` for Private, `nospoof` to skip spoof prompt

```
Usage:
  injector.exe              # Interactive: select variant + spoof option
  injector.exe private      # Direct: inject Private DLL
  injector.exe nospoof      # Skip spoof prompt
  injector.exe private nospoof  # Private DLL, no spoof
```

---

## Important Notes

- **External cheat**: all game memory reads/writes go through kernel driver IOCTL — never direct memory access.
- Feature toggles are `inline` globals in `offsets.hpp` — shared thread, no sync needed.
- BN chain fails with `wrapper_class_ptr=0` for ~30s after pressing HOME — this is normal while joining a server.
- `C:\rust_decrypts.dat` is locked while game runs — delete only when game is closed.
- ImGui `Checkbox` must NOT be wrapped in style push/pop lambdas — this corrupts the style stack in child windows.

---

## TODO / Known Issues

- [ ] PhysX-based line-of-sight raycasting (currently uses `PlayerModel + 0xBC` flag + temporal filter)
- [ ] Admin flag command execution via ConsoleSystem (offset unknown for current patch)
- [ ] Magic bullet / silent shot reliability improvements
- [ ] Water/fog removal stability on newer Unity builds
- [ ] Time changer daylight cycle sync
- [ ] Spoofer driver compatibility detection (Bfo64 build mismatch warning already in-place)
- [ ] Server-side health bar (not readable from client memory in current Rust builds)

---

## Changelog

### 2026-06-16
- **Visibility pipeline**: Added Flag/Hybrid/Strict modes with temporal smoothing and hold time
- **Config dropdown**: Saved configs auto-discovered and shown in Settings dropdown
- **Spoofer rewrite**: Embedded spoofer now uses correct `REQUEST_DATA` struct layout (matches driver)
- **Spoofer diagnostics**: Per-command IOCTL diag output, mismatch detection, non-blocking cleanup
- **Menu cleanup**: Removed top search bar, removed Clothing/ItemList/ESP Range Overlay/Reload Bar toggles
- **Backspace fix**: `io.AddKeyEvent` for control keys fixes InputText backspace/delete
- **Offset correction**: `PlayerModel::SkinnedMultiMesh` corrected to 0x428
- **NPC fix**: Removed `is_npc` gate on NPC ESP so safe-zone guards render
- **Debug camera**: 50s timer with HUD countdown, keybind in Settings
- **Bright Night**: TOD ambient static-chain resolver, Ambient Saturation control
- **Repo cleanup**: Removed 8 dead/empty stub files, moved dumpers to `tools/`, comprehensive README

### 2026-06-10
- Initial v39/v40 dump integration
- BN chain entity classification
- ImGui D3D11 overlay menu (10 pages)
- Config save/load, decrypt constant system
- Embedded spoofer implementation
