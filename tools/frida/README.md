# Frida Runtime Dumper

JavaScript Frida scripts for runtime memory inspection of RustClient.exe.

## Scripts

| Script | Purpose |
|--------|---------|
| `dump_full.js` | Full runtime dump — all classes, fields, methods |
| `dump_il2cpp.js` | Il2Cpp-specific dump — focused on encrypted fields |
| `frida_run.py` | Python runner — attaches to RustClient, runs script |
| `frida_dump.bat` | Wrapper — calls frida_run.py |

## Usage

```
frida_dump.bat
```

Or manually:
```
python frida_run.py dump_full.js
```

Output: `frida_dump.txt` (gitignored, regenerable)

## When to Use

- Debug specific field values at runtime
- Verify decrypt function inputs/outputs
- Inspect GCHandle resolution behavior

## Requirements

- Frida installed (`pip install frida frida-tools`)
- Rust running
