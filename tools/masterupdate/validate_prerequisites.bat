@echo off
title Validate Prerequisites
echo [VALIDATE] Checking prerequisites...

:: Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [FAIL] Python not found. Install from https://python.org
    exit /b 1
)
echo [OK] Python found

:: Check MSBuild
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    echo [OK] MSBuild found
) else (
    echo [WARN] MSBuild not found at expected location
)

:: Check Rust paths
if exist "C:\Program Files (x86)\Steam\steamapps\common\Rust\GameAssembly.dll" (
    echo [OK] GameAssembly.dll found
) else (
    echo [WARN] GameAssembly.dll not found at default Steam path
)

if exist "C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient_Data\il2cpp_data\Metadata\global-metadata.dat" (
    echo [OK] global-metadata.dat found
) else (
    echo [WARN] global-metadata.dat not found at default Steam path
)

:: Check il2cpp_dumper
if exist "tools\dumper\il2cpp_dumper.exe" (
    echo [OK] il2cpp_dumper.exe found
) else (
    echo [WARN] il2cpp_dumper.exe not found
)

:: Check Frida
python -c "import frida" >nul 2>&1
if errorlevel 1 (
    echo [WARN] Frida Python module not found. Install: pip install frida
) else (
    echo [OK] Frida Python module found
)

echo [OK] Prerequisites check complete
exit /b 0
