# Il2CppDumper

Offline .NET tool that extracts class/field/method offsets from `GameAssembly.dll` + `global-metadata.dat`. No game injection required.

## Usage

```
Il2CppDumper.exe "C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll" "C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat"
```

Output: `dump.cs` (C# stubs with all offsets), `script.json` (IDA/Ghidra symbols)

## Files

| File | Purpose |
|------|---------|
| `Il2CppDumper.exe` | Main dumper binary (.NET 6) |
| `config.json` | Dumper configuration |
| `scripts/` | IDA Pro + Ghidra export scripts |

## When to Use

- Verify offsets against live sha-dumper data
- Generate IDA/Ghidra symbol scripts for reverse engineering
- Check field types and inheritance hierarchy

## Alternative

For runtime verification (more accurate for encrypted fields), use sha-dumper instead.
