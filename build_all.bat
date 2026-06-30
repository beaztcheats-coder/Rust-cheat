@echo off
setlocal enabledelayedexpansion

set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
set PROJ="%~dp0Rust Prv Ext.vcxproj"

echo ================================================================
echo   BUILDING ALL 2 DLL FLAVORS
echo ================================================================

set ERRORS=0

echo.
echo [1/2] Building BeaZt DLL (full rebuild)...
%MSBUILD% %PROJ% /p:Configuration=Release /p:Platform=x64 /t:Rebuild /m /v:minimal
if errorlevel 1 (
    echo [FAIL] BeaZt DLL build failed
    set /a ERRORS+=1
) else (
    echo [OK] Rust Prv Ext.dll
)

echo.
echo [2/2] Building Bomza DLL (incremental)...
%MSBUILD% %PROJ% /p:Configuration=Release /p:Platform=x64 /p:TargetName="bomzarust" /m /v:minimal
if errorlevel 1 (
    echo [FAIL] Bomza DLL build failed
    set /a ERRORS+=1
) else (
    echo [OK] bomzarust.dll
)

echo.
echo ================================================================
if %ERRORS% EQU 0 (
    echo   ALL 2 BUILDS SUCCESSFUL
    echo.
    echo   x64\Release\Rust Prv Ext.dll    ^(BeaZt^)
    echo   x64\Release\bomzarust.dll       ^(Bomza^)
) else (
    echo   %ERRORS% BUILD(S) FAILED
)
echo ================================================================
echo.
pause
endlocal
