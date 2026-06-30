# Master Update Pipeline

Automated update system for Rust offsets, decryptions, and encryptions. Fetches data from Morphine (primary) or sha-dumper (fallback), generates patches, and applies them automatically.

## Quick Start

### After a Rust game update:

```
1. Run auto_update.bat    → fetches Morphine + injects sha-dumper + generates patches
2. Check output/diff_report.txt for changes
3. Either:
   a. Run update_now.bat  → auto-applies all patches + builds DLL
   b. Paste output/super_prompt.txt into opencode for manual review
4. Inject the new DLL and test
```

## Scripts

### Entry Points

| Script | Purpose |
|--------|---------|
| `auto_update.bat` | 7-step pipeline: fetch Morphine → parse → sha-dumper → merge → compare → generate prompt → summary |
| `update_now.bat` | Applies generated patches + builds cheat DLL. Auto-restores backup on build failure. |
| `validate_prerequisites.bat` | Checks Python, MSBuild, and Rust installation |

### Core Pipeline Scripts

| Script | Purpose |
|--------|---------|
| `fetch_morphine.py` | Downloads offsets.hpp, offsets.json, materials.hpp from Morphine server |
| `parse_all.py` | Parses Morphine offsets.hpp into master.json (offsets, structs, decrypts) |
| `sha_dumper_inject.py` | Builds + injects sha-dumper DLL into running Rust process |
| `parse_sha_dumper.py` | Parses sha-dumper INI output, merges into master.json (fallback for Morphine) |
| `compare_and_patch.py` | Compares master.json against current code, generates 5 patch files |
| `generate_super_prompt.py` | Generates super_prompt.txt with all changes for AI/manual review |
| `apply_patches.py` | Applies patches to offsets.hpp, sdk.hpp, OffsetManager.hpp (requires auto_apply=true) |
| `build_cheat.py` | Runs MSBuild to compile the cheat DLL |
| `analyze_cpp2il.py` | Validates class structure using Cpp2IL |

### Config

| File | Purpose |
|------|---------|
| `config.json` | Master configuration: URLs, paths, field mappings, decrypt mappings |
| `output/master.json` | Parsed offset data (from Morphine + sha-dumper merge) |
| `output/super_prompt.txt` | Complete update prompt for opencode (5 sections) |
| `output/rust_decrypts.dat` | Runtime decrypt constants (loaded by OffsetManager) |
| `output/diff_report.txt` | Human-readable offset/decrypt diff |

## Build Flavors

| Flavor | TargetName | Output DLL | Config Path |
|--------|-----------|-----------|-------------|
| Private (default) | `Rust Prv Ext` | `Rust Prv Ext.dll` | `C:\rustcfg_private.dat` |
| Vsharp | `vsharp` | `vsharp.dll` | `C:\rustcfg_vsharp.dat` |
| Bomza | `bomzarust` | `bomzarust.dll` | `C:\rustcfg_lite.dat` |

## Morphine Fallback

If Morphine is down, the pipeline automatically falls back to sha-dumper:
- sha-dumper provides ALL offsets, decrypts, TypeInfos, GCHandle RVA
- parse_sha_dumper.py converts to master.json format
- Pipeline works identically whether data came from Morphine or sha-dumper

## super_prompt.txt Sections

1. **Offsets changes** — namespace-scoped OLD/NEW patch lines
2. **Decrypt namespace** — complete sdk.hpp namespace decrypt replacement
3. **Il2cppGetHandle** — GCHandle resolver function for reference
4. **rust_decrypts.dat** — runtime decrypt key file
5. **OffsetManager changes** — DecryptConfig struct + LOAD_DEC + SaveDecryptConfig

## Backup System

- Backups stored in `backups/YYYY-MM-DD_HH-MM-SS/`
- Contains: offsets.hpp, sdk.hpp, OffsetManager.hpp
- update_now.bat auto-restores from latest backup on build failure
- Only the 5 most recent backups are kept (older pruned automatically)
