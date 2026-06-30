============================================================
 Il2CppDumper v6.7.46 — Quick Reference
============================================================

TOOL:  Il2CppDumper.exe
FROM:  https://github.com/Perfare/Il2CppDumper
VERS:  v6.7.46 (net6)

============================================================
 QUICK START
============================================================

1. Find your Rust game files (Steam default):
   GameAssembly.dll:        C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll
   global-metadata.dat:     C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat

2. Open Command Prompt in this folder (dumper\)

3. Run:
   Il2CppDumper.exe "C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll" "C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat" "C:\rust_dump\"

   (Create C:\rust_dump\ first if it doesn't exist)

4. Output files in C:\rust_dump\:
   dump.cs      → C# class stubs with ALL field offsets (open in Notepad)
   script.json  → JSON metadata with every class/field/method

============================================================
 WHAT TO LOOK FOR
============================================================

FIND TYPEINFO POINTERS (offsets.hpp lines 8-11):
   Open dump.cs, search for "BaseNetworkable" class.
   Look for a comment like: // TypeDefIndex: 1234
   The pointer is at the address shown BEFORE the class name.
   (In practice: use UnknownCheats or the cheat's existing values)

FIND FIELD OFFSETS (offsets.hpp lines 13-72):
   Open dump.cs, search for "BasePlayer" class.
   Find fields like:
       public float playerFlags; // 0x670
   Copy the hex value (e.g., 0x670) into offsets.hpp.

   Common classes to search for:
   - BasePlayer          → all player-related offsets
   - BaseCombatEntity    → health, maxHealth
   - BaseProjectile      → weapon offsets (recoil, spread, etc.)
   - RecoilProperties    → recoil override values
   - PlayerInventory     → belt, wear, main containers
   - BaseMovement        → movement component

============================================================
 NOTES
============================================================
- Requires .NET 6 Runtime installed on your PC
- If "global-metadata.dat not found" — the path changed. Search for it.
- GameAssembly.dll may be LZ4-compressed in newer Rust versions.
  If Il2CppDumper fails, try using Cpp2IL instead.
- All output is plain text. Use Notepad or VS Code to read dump.cs.
