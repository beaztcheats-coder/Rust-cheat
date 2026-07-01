@echo off
setlocal
title Rust Cheat Update Applier
cd /d "%~dp0"

if not exist "output" mkdir output
set "LOG=output\update_now.log"
set "STEPLOG=%TEMP%\update_now_step.log"
if exist "%LOG%" del "%LOG%"

call :log "================================================================"
call :log "  RUST CHEAT UPDATE APPLIER"
call :log "  Auto-applies patches + builds cheat"
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

REM === Check that auto_update.bat was run first ===
set MISSING_FILES=0
if not exist "output\offsets_patch.txt" set /a MISSING_FILES+=1
if not exist "output\sdk_decrypt_patch.txt" set /a MISSING_FILES+=1
if not exist "output\offsetmanager_patch.txt" set /a MISSING_FILES+=1
if not exist "output\rust_decrypts.dat" set /a MISSING_FILES+=1
if not exist "output\super_prompt.txt" set /a MISSING_FILES+=1
if not exist "output\frida_patches.txt" set /a MISSING_FILES+=1
if %MISSING_FILES% GTR 0 goto patches_missing
call :log "[OK] All 6 patch files present."
goto start_steps

:patches_missing
call :log "[ERROR] %MISSING_FILES% patch file(s) missing in output."
call :log "[ERROR] You MUST run getnewoffsets.bat FIRST to generate all patches."
echo.
echo ================================================================
echo   PATCH FILES MISSING (%MISSING_FILES% of 6).
echo   Run getnewoffsets.bat FIRST to generate all patches.
echo   Full log: %CD%\%LOG%
echo ================================================================
pause
exit /b 1

:start_steps
REM === Step 1: Enable auto_apply temporarily ===
call :log "[1/4] Enabling auto-apply in config.json..."
%PYTHON_CMD% -c "import json; c=json.load(open('config.json')); c['auto_apply']=True; json.dump(c, open('config.json','w'), indent=2)" > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" goto step1_failed

REM === Step 2: Apply patches ===
call :log "[2/4] Applying patches..."
%PYTHON_CMD% apply_patches.py > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] apply_patches.py returned RC=%RC% (continuing; build may fail)."

REM === Step 3: Disable auto_apply ===
call :log "[3/4] Disabling auto-apply in config.json..."
%PYTHON_CMD% -c "import json; c=json.load(open('config.json')); c['auto_apply']=False; json.dump(c, open('config.json','w'), indent=2)" > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" call :log "[WARN] Could not disable auto_apply (RC=%RC%). Edit config.json manually."

REM === Step 4: Build all flavors ===
call :log "[4/4] Building all 3 cheat flavors (this may take a while)..."
%PYTHON_CMD% build_cheat.py "Rust Prv Ext" "Rust Prv Ext_debug" "bomzarust" > "%STEPLOG%" 2>&1
set "RC=%errorlevel%"
if exist "%STEPLOG%" type "%STEPLOG%"
if exist "%STEPLOG%" type "%STEPLOG%" >> "%LOG%"
if not "%RC%"=="0" goto build_failed

call :log "[OK] All builds succeeded."
if exist "output\build_log_Rust Prv Ext.txt" type "output\build_log_Rust Prv Ext.txt" >> "%LOG%"
if exist "output\build_log_bomzarust.txt" type "output\build_log_bomzarust.txt" >> "%LOG%"

REM === Copy mesh data for distribution ===
if exist "output\rust_mesh.tri" (
    copy /Y "output\rust_mesh.tri" "%~dp0..\..\rust_mesh.tri" >nul 2>&1
    for %%A in ("output\rust_mesh.tri") do call :log "[OK] Mesh data copied for distribution (%%~zA bytes)"
) else (
    call :log "[INFO] No mesh data generated (VisCheck is currently disabled)"
)

call :log "================================================================"
call :log "  UPDATE COMPLETE!"
call :log "================================================================"
echo.
echo ================================================================
echo   UPDATE COMPLETE!
echo   Cheat DLLs:
echo     x64\Release\Rust Prv Ext.dll (production)
echo     x64\Release\Rust Prv Ext_debug.dll (debug - auto-logging)
echo     x64\Release\bomzarust.dll (bomza)
if exist "output\rust_mesh.tri" (
    echo   Mesh data:
echo     rust_mesh.tri ^(copy alongside DLL for VisCheck^)
) else (
    echo   Mesh data: NOT GENERATED ^(VisCheck is disabled^)
)
echo   Full log:  %CD%\%LOG%
echo ================================================================
echo.
echo Test checklist:
echo   [ ] Inject and verify ESP renders
echo   [ ] Verify aimbot works
echo   [ ] Verify recoil works
echo   [ ] No crashes
echo   [ ] When done debugging: delete C:\rust_debug_enabled.txt for stealth mode
echo.
pause
exit /b 0

:step1_failed
call :log "[ERROR] Step 1 failed (RC=%RC%). config.json may be invalid or locked."
echo.
echo ================================================================
echo   STEP 1 FAILED: cannot enable auto_apply.
echo   Check config.json is valid JSON and not open in another program.
echo   Full log: %CD%\%LOG%
echo ================================================================
pause
exit /b 1

:build_failed
call :log "[ERROR] build_cheat.py FAILED (RC=%RC%). Restoring backup..."
if exist "output\build_log.txt" call :log "--- output\build_log.txt ---"
if exist "output\build_log.txt" type "output\build_log.txt" >> "%LOG%"
echo.
echo ================================================================
echo   BUILD FAILED - restoring backup
echo ================================================================
for /f "delims=" %%d in ('dir /b /ad /o-n backups\') do (
    call :log "[RESTORE] Restoring from backups\%%d"
    copy /Y "backups\%%d\offsets.hpp" "%~dp0..\..\offsets.hpp" >nul 2>&1
    copy /Y "backups\%%d\sdk.hpp" "%~dp0..\..\sdk.hpp" >nul 2>&1
    copy /Y "backups\%%d\OffsetManager.hpp" "%~dp0..\..\OffsetManager.hpp" >nul 2>&1
    goto restored
)
call :log "[ERROR] No backup found - manual fix required."
echo No backup found - manual fix required.
:restored
call :log "[ERROR] Update FAILED. Paste output\opencode_update_prompt.txt into opencode for manual fix."
echo.
echo ================================================================
echo   BUILD FAILED. Backup restored (if any).
echo   Paste output\opencode_update_prompt.txt into opencode for a manual fix.
echo   Full log: %CD%\%LOG%
echo ================================================================
pause
exit /b 1

:log
echo %~1
echo [%date% %time%] %~1>>"%LOG%"
goto :eof
