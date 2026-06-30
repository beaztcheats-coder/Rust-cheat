@echo off
title Rust Static Offset Dumper — wklin8607 Il2CppDumper
echo =============================================
echo  Rust Il2Cpp Static Offset Dumper
echo  Uses wklin8607 Il2CppDumper v6.7.48
echo =============================================
echo.
echo This reads GameAssembly.dll + global-metadata.dat
echo DIRECTLY from disk. No game injection needed!
echo Rust does NOT need to be running.
echo =============================================
echo.
set DUMPER=C:\rust_dump_wklin\Il2CppDumper.exe
set GAMEASM=C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll
set METADATA=C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat
set OUTDIR=C:\rust_dump_latest\

echo [1/3] Creating output directory...
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

echo [2/3] Running Il2CppDumper...
echo   GameAssembly: %GAMEASM%
echo   Metadata:     %METADATA%
echo   Output:       %OUTDIR%
echo.
"%DUMPER%" "%GAMEASM%" "%METADATA%" "%OUTDIR%"

echo.
echo [3/3] Done!
echo.
echo Output files:
echo   %OUTDIR%dump.cs          — all class/field offsets
echo   %OUTDIR%DummyDll\        — restored DLLs
echo   %OUTDIR%script.json      — method metadata
echo.
echo =============================================
echo  To find Item/PlayerInventory offsets:
echo    1. Open %OUTDIR%dump.cs
echo    2. Search for "ItemContainer" or "PlayerInventory"
echo    3. Note all field offsets (0xXX values)
echo    4. Copy correct values to offsets.hpp
echo =============================================
echo.
echo Opening dump.cs...
start notepad "%OUTDIR%dump.cs"
pause
