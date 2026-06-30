@echo off
title Beazt Master Updater
echo ========================================
echo  Beazt Private – Master Update
echo ========================================
echo.

cd /d "%~dp0"

:: Detect Python command
set "PYTHON_CMD="
for %%p in (python py python3) do (
    %%p --version >nul 2>&1
    if not errorlevel 1 (
        set "PYTHON_CMD=%%p"
        goto :python_found
    )
)
echo [ERROR] Python not found. Tried: python, py, python3
echo         Install Python from https://python.org or add it to PATH.
pause
exit /b 1
:python_found
echo [+] Using Python: %PYTHON_CMD%
echo.

:: Step 1: Fetch
echo [1/8] Fetching Morphine files...
%PYTHON_CMD% fetch_morphine.py
if errorlevel 1 (
    echo [ERROR] Fetch failed.
    pause
    exit /b 1
)
echo.

:: Step 2: Parse
echo [2/8] Parsing offsets and decrypts...
%PYTHON_CMD% parse_all.py
if errorlevel 1 (
    echo [ERROR] Parse failed.
    pause
    exit /b 1
)
echo.

:: Step 3: Analyze Cpp2IL dump for class/type validation
echo [3/8] Analyzing Cpp2IL dump (type validation)...
%PYTHON_CMD% analyze_cpp2il.py
if errorlevel 1 (
    echo [WARN] Cpp2IL analysis failed. Continuing without type validation.
)
echo.

:: Step 4: Compare and generate patches
echo [4/8] Generating diff and patches...
%PYTHON_CMD% compare_and_patch.py
if errorlevel 1 (
    echo [ERROR] Compare failed.
    pause
    exit /b 1
)
echo.

:: Step 5: Generate super prompt for AI
echo [5/8] Generating super prompt for OpenAI/OpenCode...
%PYTHON_CMD% generate_super_prompt.py
if errorlevel 1 (
    echo [ERROR] Super prompt generation failed.
    pause
    exit /b 1
)
echo.

:: Step 6: Analyze handle resolver (optional)
echo [6/8] Analyzing gchandle_get_target (optional)...
%PYTHON_CMD% analyze_handle.py
echo     ^^ This step is informational. If it fails, continue anyway.
echo.

:: Step 7: Apply patches (only if auto_apply=true)
echo [7/8] Applying patches if auto_apply=true...
%PYTHON_CMD% apply_patches.py
if errorlevel 1 (
    echo [ERROR] Apply failed.
    pause
    exit /b 1
)
echo.

:: Step 8: Build
echo [8/8] Building Rust Prv Ext.dll...
%PYTHON_CMD% build_cheat.py
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)
echo.

:: Show summary
echo ========================================
type output\summary.txt
echo ========================================
echo.
echo [OK] Master update complete.
echo.
echo Review these files before injecting:
echo   - output\diff_report.txt
echo   - output\offsets_patch.txt
echo   - output\sdk_decrypt_patch.txt
echo   - output\rust_decrypts.dat
echo   - output\super_prompt.txt (paste into OpenAI/OpenCode)
echo   - output\cpp2il_types.json (Cpp2IL class validation)
echo   - output\handle_draft.cpp (if capstone is installed)
echo   - output\handle_dump.json (for AI analysis)
echo   - output\ask_opencode.txt (ready-to-paste AI prompt)
echo.
pause
