@echo off
setlocal EnableDelayedExpansion
:: ============================================================
:: auto_update.bat — Fully automated post-game-update workflow
::
:: Usage:
::   tools\auto_update.bat                     (looks for dump on Desktop)
::   tools\auto_update.bat "C:\path\to\file.h" (specify dump path)
::
:: What this does:
::   1. Converts morphine-dumper2 output → rust_decrypts.dat + updated_offsets.txt
::   2. Auto-patches offsets.hpp + Driver.hpp with new offset values
::   3. Copies rust_decrypts.dat to DLL output directory
::   4. Rebuilds all 4 DLL flavors
::   5. Reports results
::
:: Prerequisites:
::   - morphine-dumper2 DLL already injected and output file generated
::   - Python 3.x installed
::   - MSBuild (VS2022) installed
:: ============================================================

set "DUMP_FILE=%~1"
set "SCRIPT_DIR=%~dp0"
set "CHEAT_DIR=%SCRIPT_DIR%.."

:: Find dump file
if "%DUMP_FILE%"=="" (
    for /f "usebackq tokens=2,*" %%A in (`reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Desktop 2^>nul`) do set "DESKTOP=%%B"
    if exist "!DESKTOP!\morphine-dumper_output.h" (
        set "DUMP_FILE=!DESKTOP!\morphine-dumper_output.h"
    ) else (
        echo [ERROR] morphine-dumper_output.h not found on Desktop.
        echo.
        echo Steps before running this script:
        echo   1. Start Rust and load into a server
        echo   2. Inject tools\morphine-dumper2\morphine-dumper\build\Release\morphine-dumper.dll
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

echo ============================================================
echo  Rust Cheat — Fully Automated Update
echo ============================================================
echo.
echo  Dump file: %DUMP_FILE%
echo  Cheat dir: %CHEAT_DIR%
echo.

:: Step 1: Convert + auto-patch
echo [1/4] Converting dumper output + patching offsets.hpp + Driver.hpp...
python "%SCRIPT_DIR%morphine_to_cheat.py" "%DUMP_FILE%" --output-dir "%SCRIPT_DIR%" --apply --cheat-dir "%CHEAT_DIR%"
if !ERRORLEVEL! neq 0 (
    echo.
    echo [ERROR] Conversion/patching failed.
    echo   Backups saved as offsets.hpp.bak and Driver.hpp.bak
    exit /b 1
)
echo.

:: Step 2: Copy rust_decrypts.dat to DLL output directory
set "DLL_DIR=%CHEAT_DIR%\x64\Release"
if not exist "%DLL_DIR%" mkdir "%DLL_DIR%"

if exist "%SCRIPT_DIR%rust_decrypts.dat" (
    echo [2/4] Copying rust_decrypts.dat to %DLL_DIR%
    copy /Y "%SCRIPT_DIR%rust_decrypts.dat" "%DLL_DIR%\rust_decrypts.dat" >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo   Done
    ) else (
        echo   [WARNING] Copy failed — copy manually
    )
) else (
    echo [2/4] No rust_decrypts.dat to copy
)
echo.

:: Step 3: Rebuild all 4 DLLs
echo [3/4] Rebuilding all 4 DLL flavors...
set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
set "VCPROJ=%CHEAT_DIR%\Rust Prv Ext.vcxproj"
set /a BUILD_OK=0
set /a BUILD_FAIL=0

for %%F in ("Rust Prv Ext_debug" "Rust Prv Ext" "bomzarust" "bettercheats") do (
    set "TARGET=%%~F"
    echo   Building !TARGET!...
    "%MSBUILD%" "%VCPROJ%" /p:Configuration=Release /p:Platform=x64 /p:TargetName="!TARGET!" /m /v:minimal >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo     OK
        set /a BUILD_OK+=1
    ) else (
        echo     FAILED
        set /a BUILD_FAIL+=1
    )
)
echo.
echo   Build results: !BUILD_OK! succeeded, !BUILD_FAIL! failed
echo.

:: Step 4: Summary
echo [4/4] Summary
echo   rust_decrypts.dat    — Copied to %DLL_DIR%
echo   offsets.hpp          — Auto-patched (backup: offsets.hpp.bak)
echo   Driver.hpp           — Auto-patched (backup: Driver.hpp.bak)
echo   updated_offsets.txt  — Full offset report at %SCRIPT_DIR%updated_offsets.txt
echo   DLLs rebuilt         — !BUILD_OK!/4 succeeded
echo.
if !BUILD_FAIL! neq 0 (
    echo [WARNING] !BUILD_FAIL! DLL(s) failed to build!
    echo   Check the build output above or rebuild manually:
    echo   "%MSBUILD%" "%VCPROJ%" /p:Configuration=Release /p:Platform=x64 /p:TargetName=^<name^> /m
    echo.
    echo   To restore backups:
    echo   copy "%CHEAT_DIR%\offsets.hpp.bak" "%CHEAT_DIR%\offsets.hpp"
    echo   copy "%CHEAT_DIR%\Driver.hpp.bak" "%CHEAT_DIR%\Driver.hpp"
)
echo ============================================================
echo  Done
echo ============================================================

endlocal
