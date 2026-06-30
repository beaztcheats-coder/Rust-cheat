@echo off
setlocal
echo ========================================
echo   BEAZT Full Build Pipeline
echo ========================================
echo.

echo [1/4] Building Rust Private Lite DLL...
call "F:\github\rust\build_lite.bat"
if errorlevel 1 (
    echo [FAIL] Lite DLL build failed
    exit /b 1
)
echo.

echo [2/4] Building Rust Private DLL...
call "F:\github\rust\build_private.bat"
if errorlevel 1 (
    echo [FAIL] Private DLL build failed
    exit /b 1
)
echo.

echo [3/4] Building Injector...
call "F:\github\rust\build_injector.bat"
if errorlevel 1 (
    echo [FAIL] Injector build failed
    exit /b 1
)
echo.

echo [4/4] Building Spoofer (Rust)...
set "PATH=%USERPROFILE%\.cargo\bin;%PATH%"
cd /d "F:\github\spoofer"
cargo build --release 2>nul
if errorlevel 1 (
    echo [FAIL] Spoofer build failed - check Rust toolchain
) else (
    copy /y "F:\github\spoofer\target\release\spoofer_exe.exe" "F:\github\rust\x64\Release\spoofer.exe" >nul
    echo [OK] Spoofer built
)

echo.
echo ========================================
echo   BUILD COMPLETE
echo ========================================
echo.
echo  Outputs:
echo    x64\Release\Rust Private Lite.dll
echo    x64\Release\Rust Private.dll
echo    x64\Release\injector.exe
echo    x64\Release\spoofer.exe
echo.
echo  Run x64\Release\injector.exe to launch.
echo ========================================
endlocal
