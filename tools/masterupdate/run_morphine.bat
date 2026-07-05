@echo off
setlocal EnableDelayedExpansion
title Morphine Master Dumper Pipeline
color 0A

echo ====================================================================
echo  MORPHINE MASTER DUMPER PIPELINE
echo  One-shot dump: inject DLL ^> parse output ^> apply patches ^> build
echo ====================================================================
echo.

set "SCRIPT_DIR=%~dp0"
set "PYTHON_CMD=python"
set "DUMPER_DLL=%SCRIPT_DIR%..\morphine-dumper\build\Release\morphine-dumper.dll"
set "OUTPUT_DIR=%SCRIPT_DIR%output"
set "DUMP_PATH=%USERPROFILE%\Desktop\morphine-dumper_output.h"

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo [STEP 1] Morphine Dumper DLL Check
echo ...............................................................
if not exist "%DUMPER_DLL%" (
    echo [ERROR] Morphine dumper DLL not found: %DUMPER_DLL%
    echo         Build it first: morphine-dumper\morphine-dumper\morphine-dumper.vcxproj
    pause
    exit /b 1
)
echo [OK] Dumper DLL: %DUMPER_DLL%
echo.

echo [STEP 2] Inject Morphine Dumper into Rust
echo ...............................................................
echo.
echo  Make sure Rust is running and you are on a server!
echo  The dumper will be injected automatically.
echo.
pause

REM Inject using the same CreateRemoteThread method as sha-dumper
set "INJECT_PS1=%SCRIPT_DIR%..\build\Release\inject.ps1"
if not exist "%INJECT_PS1%" (
    echo [ERROR] inject.ps1 not found: %INJECT_PS1%
    pause
    exit /b 1
)
echo [INFO] Injecting morphine-dumper.dll into RustClient.exe...
powershell -ExecutionPolicy Bypass -File "%INJECT_PS1%" "%DUMPER_DLL%"
if errorlevel 1 (
    echo [ERROR] Injection failed. Make sure Rust is running.
    pause
    exit /b 1
)
echo [OK] Dumper injected
echo.

echo [STEP 3] Wait for Dump Output
echo ...............................................................
echo  Waiting for: %DUMP_PATH%
echo  The dumper writes to Desktop\morphine-dumper_output.h
echo  Press END in the dumper console when it finishes.
echo.

set "WAIT_COUNT=0"
:wait_for_dump
if exist "%DUMP_PATH%" (
    echo [OK] Dump file found!
    goto :dump_found
)
set /a WAIT_COUNT+=1
if !WAIT_COUNT! geq 120 (
    echo [ERROR] Timeout waiting for dump file (2 minutes)
    echo         Make sure the dumper was injected and ran successfully.
    pause
    exit /b 1
)
timeout /t 5 /nobreak >nul
echo  Waiting... (!WAIT_COUNT!/120)
goto :wait_for_dump

:dump_found
REM Copy dump to output directory
copy /Y "%DUMP_PATH%" "%OUTPUT_DIR%\morphine-dump.h" >nul
echo [OK] Dump copied to %OUTPUT_DIR%\morphine-dump.h

REM Copy rust_mesh.tri if it exists (PhysX collision mesh for VisCheck)
set "TRI_PATH=%USERPROFILE%\Desktop\rust_mesh.tri"
if exist "%TRI_PATH%" (
    copy /Y "%TRI_PATH%" "%OUTPUT_DIR%\rust_mesh.tri" >nul
    echo [OK] rust_mesh.tri copied to %OUTPUT_DIR%\rust_mesh.tri
) else (
    echo [INFO] No rust_mesh.tri found on Desktop - VisCheck mesh not dumped
)
echo.

echo [STEP 4] Parse Morphine Dump
echo ...............................................................
%PYTHON_CMD% "%SCRIPT_DIR%parse_morphine_dump.py" --input "%OUTPUT_DIR%\morphine-dump.h" --output-dir "%OUTPUT_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to parse morphine dump
    pause
    exit /b 1
)
echo [OK] Parse complete
echo.

echo [STEP 5] Generate Patches
echo ...............................................................
%PYTHON_CMD% "%SCRIPT_DIR%compare_and_patch.py"
if errorlevel 1 (
    echo [WARN] Patch generation had issues — check output
)
echo [OK] Patches generated
echo.

echo [STEP 6] Apply Patches
echo ...............................................................
%PYTHON_CMD% "%SCRIPT_DIR%apply_patches.py"
if errorlevel 1 (
    echo [WARN] Patch application had issues — check output
)
echo [OK] Patches applied
echo.

echo [STEP 7] Build All 4 DLLs
echo ...............................................................
echo  Building debug flavor first...
set "MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
set "VCPROJ=%SCRIPT_DIR%..\Rust Prv Ext.vcxproj"

if exist "%MSBUILD%" (
    "%MSBUILD%" "%VCPROJ%" /p:Configuration=Release /p:Platform=x64 /p:TargetName="Rust Prv Ext_debug" /m /v:minimal 2>&1
    echo  Building BeaZt production...
    "%MSBUILD%" "%VCPROJ%" /p:Configuration=Release /p:Platform=x64 /p:TargetName="Rust Prv Ext" /m /v:minimal 2>&1
    echo  Building Bomza production...
    "%MSBUILD%" "%VCPROJ%" /p:Configuration=Release /p:Platform=x64 /p:TargetName=bomzarust /m /v:minimal 2>&1
    echo  Building Better Cheats production...
    "%MSBUILD%" "%VCPROJ%" /p:Configuration=Release /p:Platform=x64 /p:TargetName=bettercheats /m /v:minimal 2>&1
    echo [OK] All 4 DLLs built
) else (
    echo [WARN] MSBuild not found at expected path. Build manually.
)
echo.

echo ====================================================================
echo  MORPHINE PIPELINE COMPLETE
echo ====================================================================
echo.
echo  Output files:
echo    %OUTPUT_DIR%\master.json          (all offsets + decrypts)
echo    %OUTPUT_DIR%\rust_decrypts.dat     (decrypt constants)
echo    %OUTPUT_DIR%\morphine-dump.h       (raw dump)
echo    %SCRIPT_DIR%..\x64\Release\*.dll   (4 built DLLs)
echo.
echo  Old tools (sha-dumper, capstone, sig-scanner, Frida) are optional.
echo  Run getnewoffsets.bat for the old multi-step pipeline.
echo.
pause
