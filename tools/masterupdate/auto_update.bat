@echo off
setlocal
title Rust Cheat Auto-Update Scanner
cd /d "%~dp0"

if not exist "output" mkdir output
if exist "output\master.json" del "output\master.json"
call :log "[OK] Cleared stale master.json (prevents wrong nk2 values from previous runs)"
set "LOG=output\auto_update.log"
set "STEPLOG=%TEMP%\auto_update_step.log"
if exist "%LOG%" del "%LOG%"

call :log "================================================================"
call :log "  RUST CHEAT AUTO-UPDATE SCANNER"
call :log "  Scans Morphine + Il2CppInspector + sha-dumper + Frida + current code"
call :log "  Outputs to: output\"
call :log "================================================================"
call :log "Working dir: %CD%"
call :log "Log file:    %CD%\%LOG%"

REM === Detect Python ===
set PYTHON_CMD=
for %%p in (python py python3) do (
    %%p --version >nul 2>&1
    if not errorlevel 1 (
        set PYTHON_CMD=%%p
        goto found_python
    )
)
call :log "[ERROR] Python not found (tried python, py, python3)."
echo.
echo ================================================================
echo   PYTHON NOT FOUND.
echo   Install Python 3 from https://python.org and add to PATH.
echo   Full log: %CD%\%LOG%
echo ================================================================
pause
exit /b 1
:found_python
call :log "[OK] Python: %PYTHON_CMD%"

REM === Step 1: Fetch Morphine offsets ===
call :log "[1/12] Fetching Morphine offsets..."
%PYTHON_CMD% fetch_morphine.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
set MORPHINE_OK=1

if "%RC%"=="0" (
    call :log "[OK] Morphine fetch succeeded."
    goto step2
)

if "%RC%"=="2" (
    call :log "[OFFLINE] Morphine appears to be OFFLINE (all URLs unreachable)."
    echo.
    echo ================================================================
    echo   MORPHINE APPEARS TO BE OFFLINE
    echo   All Morphine URLs are unreachable.
    echo.
    echo   The sha-dumper can provide ALL offsets, decrypts, and mesh data.
    echo   Known limitation: some decrypt constants may be incorrect
    echo   for duplicate cipher sections ^(cl_active_item, eyes, inventory^).
    echo.
    echo   You can verify these manually after the update.
    echo ================================================================
    echo.
    choice /C YN /T 15 /D Y /M "Skip Morphine and continue with sha-dumper only"
    if errorlevel 2 goto abort_morphine
    call :log "[USER] Skipping Morphine - using sha-dumper only"
    set MORPHINE_OK=0
    goto step3
)

REM RC=1 - partial failure
if not "%RC%"=="0" (
    set MORPHINE_OK=0
    call :log "[WARN] Morphine fetch partially failed (RC=%RC%) - will use sha-dumper/Frida as fallback."
    call :log "       If Rust is running, sha-dumper + Frida can provide ALL offsets + decrypts."
)

:step2
REM === Step 2: Parse Morphine into master.json ===
if not "%MORPHINE_OK%"=="1" call :log "[2/11] Skipping Morphine parse (no data)"
if not "%MORPHINE_OK%"=="1" goto step3
call :log "[2/12] Parsing Morphine offsets and decrypts..."
%PYTHON_CMD% parse_all.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if "%RC%"=="0" goto step3
call :log "[WARN] Parse failed (RC=%RC%) - will rely on sha-dumper/Frida."
set MORPHINE_OK=0
:step3
REM === Enable logging marker for the injected DLL (after Morphine prompt) ===
echo enabled > "C:\rust_debug_enabled.txt"
call :log "[OK] Debug logging marker created (C:\rust_debug_enabled.txt)"

REM === Step 3: Run Il2CppInspectorPro (offline, no game needed) ===
call :log "[3/12] Running Il2CppInspectorPro (offline IL2CPP metadata dump)..."
set "DOTNET_ROOT=C:\Users\ludwi\AppData\Local\Temp\opencode\dotnet10"
set "DOTNET_MULTILEVEL_LOOKUP=0"
set "IL2CPP_OUT=output\il2cppinspector"
if not exist "%IL2CPP_OUT%" mkdir "%IL2CPP_OUT%"
set "IL2CPP_EXE=E:\github\Il2CppInspectorPro-master\Il2CppInspector.CLI\bin\Release\net10.0\win-x64\Il2CppInspector.exe"
set "IL2CPP_GA=C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll"
set "IL2CPP_META=C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat"
if not exist "%IL2CPP_GA%" (call :log "[WARN] GameAssembly.dll not found at: %IL2CPP_GA% - skipping Il2CppInspector" & goto :skip_il2cpp)
"%IL2CPP_EXE%" -i "%IL2CPP_GA%" -m "%IL2CPP_META%" --select-outputs -o "%IL2CPP_OUT%\metadata.json" -h "%IL2CPP_OUT%\cpp" > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Il2CppInspector had issues (RC=%RC%) - continuing with sha-dumper/Frida."
if exist "%IL2CPP_OUT%\metadata.json" call :log "[OK] Il2CppInspector metadata.json generated"
if exist "%IL2CPP_OUT%\cpp\appdata\il2cpp-types-ptr.h" call :log "[OK] Il2CppInspector il2cpp-types-ptr.h generated"

REM Step 3b: Run DumpOffsets (queries Camera/BasePlayer/etc via Il2CppInspector.Common library API)
set "DUMPOFFSETS_EXE=E:\github\rust\tools\masterupdate\il2cpp_dumper\bin\Release\net10.0\DumpOffsets.exe"
set "DUMPOFFSETS_DIR=E:\github\rust\tools\masterupdate\il2cpp_dumper\bin\Release\net10.0"
if not exist "%DUMPOFFSETS_DIR%\plugins" mkdir "%DUMPOFFSETS_DIR%\plugins"
if exist "%DUMPOFFSETS_EXE%" (
    call :log "[3b/12] Running DumpOffsets (field offset extraction)..."
    "%DUMPOFFSETS_EXE%" > "%STEPLOG%" 2>&1
    set "RC=%errorlevel%"
    if exist "%STEPLOG%" type "%STEPLOG%"
    if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
    if exist "%IL2CPP_OUT%\offsets.json" call :log "[OK] DumpOffsets offsets.json generated"
    if not "%RC%"=="0" call :log "[WARN] DumpOffsets had issues (RC=%RC%) - continuing."
)
:skip_il2cpp

:step4
REM === Step 4: Build + inject sha-dumper ===
call :log "[4/12] Building and injecting sha-dumper..."
%PYTHON_CMD% sha_dumper_inject.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] sha-dumper step had issues (RC=%RC%) - continuing with Morphine/Frida data only."

REM === Check for mesh dump output ===
call :check_mesh

REM === Step 5: Parse sha-dumper output ===
call :log "[5/12] Parsing sha-dumper output..."
%PYTHON_CMD% parse_sha_dumper.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if "%RC%"=="0" goto step5_done
call :log "[WARN] sha-dumper parse failed (RC=%RC%) - continuing with Morphine data only."
:step5_done

REM === Step 5.5: Parse Il2CppInspector output ===
call :log "[5.5/12] Parsing Il2CppInspector output..."
%PYTHON_CMD% parse_il2cppinspector.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Il2CppInspector parse had issues (RC=%RC%) - continuing."

REM === Step 6: Run Frida validation (100% accurate runtime dump) ===
call :log "[6/12] Running Frida runtime dumper..."
%PYTHON_CMD% run_frida_validation.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Frida dump had issues (RC=%RC%) - sha-dumper/Morphine data still available."

REM === Step 7: Compare and generate patches ===
call :log "[7/12] Comparing offsets and generating patches..."
if not exist "output\master.json" (
    call :log "[ERROR] master.json not found - Morphine and sha-dumper both failed. Cannot generate patches."
    call :log "       Super prompt will still be generated with available data (Frida dump)."
    goto step10
)
for %%A in ("output\master.json") do set "MJ_SIZE=%%~zA"
if "%MJ_SIZE%"=="0" (
    call :log "[ERROR] master.json is empty - no data to patch. Cannot generate patches."
    goto ValidateOutputs
)
call :log "  master.json size: %MJ_SIZE% bytes"
%PYTHON_CMD% compare_and_patch.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Compare had issues (RC=%RC%) - continuing anyway."

REM === Step 8: Cross-validate offsets (Frida/il2cpp-dumper override Morphine) ===
call :log "[8/12] Cross-validating offsets against Frida/Il2CppInspector..."
%PYTHON_CMD% cross_validate_offsets.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Cross-validation had issues (RC=%RC%) - Morphine offsets used as-is."

REM === Step 9: Analyze GCHandle disassembly ===
call :log "[9/12] Analyzing GCHandle resolver (analyze_handle.py)..."
%PYTHON_CMD% analyze_handle.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] analyze_handle had issues (RC=%RC%) - handle_dump.json may be missing."

REM === Step 10: Generate opencode prompts ===
:step10
call :log "[10/12] Generating opencode prompts..."
%PYTHON_CMD% generate_super_prompt.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] super_prompt generation had issues (RC=%RC%)."
%PYTHON_CMD% generate_opencode_update_prompt.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] opencode_update_prompt generation had issues (RC=%RC%)."

REM === Step 11: Summary ===
call :log "[11/12] Validating outputs and summary..."
call :ValidateOutputs
call :log "================================================================"
call :log "  OUTPUT FILES READY IN: output\"
call :log "================================================================"
echo.
echo ================================================================
echo   OUTPUT FILES READY IN: output\
echo ================================================================
echo.
if exist "output\summary.txt" type "output\summary.txt"
if exist "output\summary.txt" type "output\summary.txt" >> "%LOG%"
echo.
echo Next steps:
echo   1. Review: output\diff_report.txt
echo   2. Review: output\summary.txt
echo   3. Either:
echo      a. Run update_now.bat to auto-apply + build
echo      b. Paste output\super_prompt.txt into opencode for full feature restore
echo.
echo Full log: %CD%\%LOG%
echo.
pause
exit /b 0

:abort_morphine
call :log "[USER] User chose to abort - Morphine required."
echo Aborting.
pause
exit /b 1

:check_mesh
set "MESH_LOG=%USERPROFILE%\Desktop\rust_mesh_dump.log"
if not exist "%MESH_LOG%" (
    call :log "[MESH] No mesh dump log found - VisCheck will use fallback (PlayerModel._visible)"
    goto :eof
)
call :log "[MESH] Mesh dump log found - checking status..."
findstr /C:"SUCCESS" "%MESH_LOG%" >nul 2>&1
if not errorlevel 1 (
    for /f "tokens=*" %%M in ('findstr /C:"SUCCESS" "%MESH_LOG%"') do call :log "[MESH] %%M"
    call :log "[MESH] Mesh dump succeeded."
    goto :eof
)
findstr /C:"ABORT" "%MESH_LOG%" >nul 2>&1
if not errorlevel 1 (
    for /f "tokens=*" %%M in ('findstr /C:"ABORT" "%MESH_LOG%"') do call :log "[MESH] %%M"
    goto :eof
)
call :log "[MESH] Mesh dump status unclear - check rust_mesh_dump.log"
goto :eof

:ValidateOutputs
set MISSING=0
if not exist "output\offsets_patch.txt" set /a MISSING+=1
if not exist "output\sdk_decrypt_patch.txt" set /a MISSING+=1
if not exist "output\offsetmanager_patch.txt" set /a MISSING+=1
if not exist "output\rust_decrypts.dat" set /a MISSING+=1
if not exist "output\super_prompt.txt" set /a MISSING+=1
if %MISSING% GTR 0 call :log "[WARN] %MISSING% required output file(s) missing - review steps above."
if %MISSING%==0 call :log "[OK] All 5 required output files present."
if exist "output\frida_dump.txt" call :log "[OK] Frida dump available (frida_dump.txt)."
if not exist "output\frida_dump.txt" call :log "[WARN] No Frida dump - manual offsets verification will be limited."
if exist "output\il2cppinspector\offsets.json" call :log "[OK] Il2CppInspector offsets available (il2cppinspector/offsets.json)."
if not exist "output\il2cppinspector\offsets.json" call :log "[WARN] No Il2CppInspector dump - offline offset validation not available."
if exist "output\il2cpp_cross_validation.txt" call :log "[OK] Il2CppInspector cross-validation report available."
if exist "output\handle_dump.json" call :log "[OK] Handle dump available (handle_dump.json)."
if not exist "output\handle_dump.json" call :log "[WARN] No handle dump - Il2cppGetHandle verification will be limited."
if exist "output\rust_mesh.tri" (
    for %%A in ("output\rust_mesh.tri") do call :log "[OK] Mesh data available (rust_mesh.tri, %%~zA bytes) - VisCheck ready for packaging."
) else (
    call :log "[WARN] No mesh data (rust_mesh.tri) - VisCheck will use PlayerModel._visible fallback."
)
goto :eof

:log
echo %~1
echo [%date% %time%] %~1>>"%LOG%"
goto :eof
