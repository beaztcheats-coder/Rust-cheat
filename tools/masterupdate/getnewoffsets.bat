@echo off
setlocal
title Rust Cheat Update Scanner
cd /d "%~dp0"

if not exist "output" mkdir output
if exist "output\master.json" del "output\master.json"
set "LOG=output\getnewoffsets.log"
set "STEPLOG=%TEMP%\getnewoffsets_step.log"
if exist "%LOG%" del "%LOG%"
call :log "[OK] Cleared stale master.json (prevents wrong values from previous runs)"

call :log "================================================================"
call :log "  RUST CHEAT UPDATE SCANNER"
call :log "  Primary: morphine-dumper (injected DLL, runtime-verified)"
call :log "  Decrypt: disasm_decrypts.py (capstone, offline, single source of truth)"
call :log "  Optional: sha-dumper, Frida, Il2CppInspector (cross-validation)"
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

REM === Enable logging marker for the injected DLL ===
echo enabled > "C:\rust_debug_enabled.txt"
call :log "[OK] Debug logging marker created (C:\rust_debug_enabled.txt)"

REM === Step 1: Build + inject morphine-dumper (PRIMARY SOURCE) ===
call :log "[1/12] Building and injecting morphine-dumper (primary source)..."
echo.
echo ================================================================
echo   STEP 1: MORPHINE-DUMPER INJECTION
echo   This is the PRIMARY source for offsets, decrypts, and encrypts.
echo   Requires: Rust running + joined a server.
echo ================================================================
echo.
%PYTHON_CMD% morphine_dumper_inject.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" (
    call :log "[WARN] morphine-dumper step had issues (RC=%RC%) - pipeline will continue with fallbacks."
    call :log "       disasm_decrypts.py can use standalone cipher scan (no dumper needed)."
)

REM === Step 2: Parse morphine-dumper output into master.json ===
call :log "[2/12] Parsing morphine-dumper output into master.json..."
if exist "output\morphine-dump.h" (
    %PYTHON_CMD% parse_morphine_dump.py --input "output\morphine-dump.h" > "%STEPLOG%" 2>&1
    set "RC=%errorlevel%"
) else if exist "output\morphine-dump.json" (
    %PYTHON_CMD% parse_morphine_dump.py --input "output\morphine-dump.json" > "%STEPLOG%" 2>&1
    set "RC=%errorlevel%"
) else (
    call :log "[WARN] No morphine-dumper output found - creating empty master.json"
    %PYTHON_CMD% -c "import json; json.dump({'offsets':{'static_pointers':{},'structs':{}},'klass_rvas':{},'namespaces':{},'decrypt_functions':{},'encrypt_functions':{}}, open('output/master.json','w'), indent=2)"
    set "RC=0"
)
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if "%RC%"=="0" (
    call :log "[OK] master.json generated from morphine-dumper output"
) else (
    call :log "[WARN] Parse failed (RC=%RC%) - continuing with empty master.json"
)

REM === Step 3: Run capstone decrypt disassembler (THE AUTHORITY for decrypts) ===
call :log "[3/12] Running capstone decrypt disassembler (offline, single source of truth)..."
call :log "       Reads fn_rvas from master.json (morphine-dumper) or sha-dumper output"
call :log "       Falls back to standalone cipher scan if no fn_rvas available"
call :log "       Auto-generates encrypt functions (inverse of decrypt)"
%PYTHON_CMD% disasm_decrypts.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if exist "output\decrypt_algorithms.json" call :log "[OK] Decrypt + encrypt algorithms extracted (output\decrypt_algorithms.json)"
if not "%RC%"=="0" call :log "[WARN] Capstone decrypt scan had issues (RC=%RC%) - fallback decrypts will be used."

REM === Step 3.5: Signature-based static pointer scan (100% OFFLINE, no game needed) ===
call :log "[3.5/12] Running signature-based static pointer scanner (offline)..."
%PYTHON_CMD% sig_scanner.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Sig scanner had issues (RC=%RC%) - morphine-dumper static pointers will be used."

REM === Step 4: Optional sha-dumper (cross-validation) ===
call :log "[4/12] Building and injecting sha-dumper (optional cross-validation)..."
%PYTHON_CMD% sha_dumper_inject.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] sha-dumper step had issues (RC=%RC%) - morphine-dumper data is sufficient."

REM === Check for mesh dump output ===
call :check_mesh

REM === Step 5: Parse sha-dumper output (if available) ===
call :log "[5/12] Parsing sha-dumper output (if available)..."
if not exist "output\sha-dumper-output.txt" (
    call :log "[INFO] No sha-dumper output - skipping (morphine-dumper is primary source)"
    goto step5_done
)
%PYTHON_CMD% parse_sha_dumper.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] sha-dumper parse failed (RC=%RC%) - continuing with morphine-dumper data."
:step5_done

REM === Step 5.6: Parse Il2CppInspector output (if available) ===
call :log "[5.6/12] Parsing Il2CppInspector output (if available)..."
%PYTHON_CMD% parse_il2cppinspector.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Il2CppInspector parse had issues (RC=%RC%) - continuing."

REM === Step 6: Run Frida validation (optional, 100% accurate runtime dump) ===
call :log "[6/12] Running Frida runtime dumper (optional cross-validation)..."
%PYTHON_CMD% run_frida_validation.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Frida dump had issues (RC=%RC%) - morphine-dumper data still available."

REM === Step 6b: Run Frida decrypt scanner (extract decrypt algorithms from game memory) ===
call :log "[6b/12] Running Frida decrypt algorithm scanner..."
%PYTHON_CMD% frida_decrypt_scan.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if exist "output\frida_decrypt_algorithms.json" call :log "[OK] Frida decrypt algorithms extracted"
if not "%RC%"=="0" call :log "[WARN] Frida decrypt scan had issues (RC=%RC%) - capstone decrypts still available."

REM === Step 6c: Generate offset patches from Frida data (100% accurate field offsets) ===
call :log "[6c/12] Generating Frida offset patches..."
%PYTHON_CMD% apply_frida_offsets.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if exist "output\frida_offset_patches.txt" call :log "[OK] Frida offset patches generated"
if not "%RC%"=="0" call :log "[WARN] Frida offset patch generation had issues (RC=%RC%)."

REM === Step 6d: Apply hash mapping (Morphine-independent field resolution) ===
call :log "[6d/12] Applying Frida hash mapping..."
if not exist "output\field_hash_mapping.json" (
    call :log "[INFO] No hash mapping yet -- building from current offsets.hpp + Frida dump..."
    %PYTHON_CMD% frida_hash_mapper.py --build > "%STEPLOG%" 2>&1
    if exist "%STEPLOG%" type "%STEPLOG%"
    if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
) else (
    call :log "[OK] Hash mapping exists -- applying to generate patches..."
    %PYTHON_CMD% frida_hash_mapper.py --apply > "%STEPLOG%" 2>&1
    if exist "%STEPLOG%" type "%STEPLOG%"
    if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
)
if exist "output\field_hash_mapping.json" call :log "[OK] Hash mapping active"

REM === Step 7: Compare and generate patches ===
call :log "[7/12] Comparing offsets and generating patches..."
if not exist "output\master.json" (
    call :log "[ERROR] master.json not found - all dumpers failed. Cannot generate patches."
    call :log "       Super prompt will still be generated with available data."
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

REM === Step 7.5: Generate verification report ===
call :log "[7.5/12] Generating verification report..."
%PYTHON_CMD% generate_verification_report.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if exist "output\verification_report.txt" call :log "[OK] Verification report generated (output\verification_report.txt)"
if not "%RC%"=="0" call :log "[WARN] Verification report had issues (RC=%RC%)."

REM === Step 7.6: Generate consolidated offsets dump ===
call :log "[7.6/12] Generating consolidated offsets dump..."
%PYTHON_CMD% generate_offsets_dump.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if exist "output\offsets_dump.hpp" call :log "[OK] Consolidated offsets dump generated (output\offsets_dump.hpp)"
if not "%RC%"=="0" call :log "[WARN] Offsets dump generation had issues (RC=%RC%)."

REM === Step 8: Cross-validate offsets (Frida/il2cpp-dumper override morphine-dumper) ===
call :log "[8/12] Cross-validating offsets against Frida/Il2CppInspector..."
%PYTHON_CMD% cross_validate_offsets.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Cross-validation had issues (RC=%RC%) - morphine-dumper offsets used as-is."

REM === Step 8.5: Run end-to-end pipeline verification ===
call :log "[8.5/12] Running end-to-end pipeline verification..."
%PYTHON_CMD% verify_pipeline.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if exist "output\pipeline_verification.txt" call :log "[OK] Pipeline verification report generated (output\pipeline_verification.txt)"
if not "%RC%"=="0" call :log "[WARN] Pipeline verification had issues (RC=%RC%)."

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
echo   2. Review: output\pipeline_verification.txt
echo   3. Review: output\summary.txt
echo   4. Either:
echo      a. Run update_now.bat to auto-apply + build
echo      b. Paste output\super_prompt.txt into opencode for full feature restore
echo.
echo Full log: %CD%\%LOG%
echo.
pause
exit /b 0

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
if exist "output\morphine-dump.json" call :log "[OK] morphine-dumper JSON available (morphine-dump.json)."
if not exist "output\morphine-dump.json" call :log "[WARN] No morphine-dumper JSON - morphine-dumper may have failed."
if exist "output\decrypt_algorithms.json" call :log "[OK] Capstone decrypt algorithms available (decrypt_algorithms.json)."
if exist "output\frida_dump.txt" call :log "[OK] Frida dump available (frida_dump.txt)."
if not exist "output\frida_dump.txt" call :log "[INFO] No Frida dump - morphine-dumper + capstone are sufficient."
if exist "output\handle_dump.json" call :log "[OK] Handle dump available (handle_dump.json)."
if not exist "output\handle_dump.json" call :log "[WARN] No handle dump - Il2cppGetHandle verification will be limited."
if exist "output\rust_mesh.tri" (
    for %%A in ("output\rust_mesh.tri") do call :log "[OK] Mesh data available (rust_mesh.tri, %%~zA bytes) - VisCheck ready for packaging."
) else (
    call :log "[INFO] No mesh data (rust_mesh.tri) - VisCheck will use isVisible flag fallback."
)
goto :eof

:log
echo %~1
echo [%date% %time%] %~1>>"%LOG%"
goto :eof
