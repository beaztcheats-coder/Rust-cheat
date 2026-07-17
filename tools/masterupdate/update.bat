@echo off
setlocal EnableDelayedExpansion
:: ============================================================
:: update.bat — Post-game-update workflow for Rust cheat
::
:: Usage:
::   update.bat                     (looks for morphine-dumper_output.h on Desktop)
::   update.bat "C:\path\to\file.h"  (specify custom path)
::
:: Prerequisites:
::   1. Inject morphine-dumper2 DLL into Rust (tools\morphine-dumper2\morphine-dumper\build\Release\morphine-dumper.dll)
::   2. Wait for dump to complete, press END to unload
::   3. Run this script
:: ============================================================

set "DUMP_FILE=%~1"
if "%DUMP_FILE%"=="" (
    :: Try Desktop
    for /f "usebackq tokens=2,*" %%A in (`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Desktop 2^>nul`) do set "DESKTOP=%%B"
    if exist "!DESKTOP!\morphine-dumper_output.h" (
        set "DUMP_FILE=!DESKTOP!\morphine-dumper_output.h"
    ) else (
        echo [ERROR] morphine-dumper_output.h not found on Desktop.
        echo Usage: update.bat "C:\path\to\morphine-dumper_output.h"
        echo.
        echo Steps:
        echo   1. Start Rust and load into a server
        echo   2. Inject tools\morphine-dumper2\build\Release\morphine-dumper.dll
        echo   3. Wait ~30 seconds for dump to complete
        echo   4. Press END to unload the dumper
        echo   5. Run this script again
        exit /b 1
    )
)

if not exist "%DUMP_FILE%" (
    echo [ERROR] File not found: %DUMP_FILE%
    exit /b 1
)

set "SCRIPT_DIR=%~dp0"
set "CHEAT_DIR=%SCRIPT_DIR%.."
set "OUTPUT_DIR=%SCRIPT_DIR%"

echo ============================================================
echo  Rust Cheat — Post-Update Workflow
echo ============================================================
echo.
echo  Dump file: %DUMP_FILE%
echo.

:: Step 1: Run conversion script
echo [1/3] Running morphine_to_cheat.py...
python "%SCRIPT_DIR%morphine_to_cheat.py" "%DUMP_FILE%" --output-dir "%OUTPUT_DIR%"
if !ERRORLEVEL! neq 0 (
    echo [ERROR] Conversion script failed.
    exit /b 1
)
echo.

:: Step 2: Copy rust_decrypts.dat to DLL output directory
set "DLL_DIR=%CHEAT_DIR%\x64\Release"
if not exist "%DLL_DIR%" set "DLL_DIR=%CHEAT_DIR%\x64"

if exist "%OUTPUT_DIR%rust_decrypts.dat" (
    echo [2/3] Copying rust_decrypts.dat to %DLL_DIR%
    copy /Y "%OUTPUT_DIR%rust_decrypts.dat" "%DLL_DIR%\rust_decrypts.dat" >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo   Done — decrypts will be loaded at runtime (no recompile needed)
    ) else (
        echo   [WARNING] Could not copy to %DLL_DIR% — copy manually
    )
) else (
    echo [2/3] No rust_decrypts.dat generated (decrypt functions not found in dump)
)
echo.

:: Step 3: Show summary
echo [3/3] Summary
echo   rust_decrypts.dat    — Copy to DLL directory (runtime decrypt override)
echo   updated_offsets.txt  — Review and update offsets.hpp manually
echo.
echo   Next steps:
echo     1. Check updated_offsets.txt for changed static pointers / field offsets
echo     2. Update offsets.hpp with any changed values
echo     3. Update GameAssembly timestamp in Driver.hpp if changed
echo     4. Rebuild: MSBuild "Rust Prv Ext.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:TargetName=^<name^>
echo.
echo ============================================================
echo  Done
echo ============================================================

endlocal
