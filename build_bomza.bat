@echo off
setlocal

set MSBUILD="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
set PROJ="%~dp0Rust Prv Ext.vcxproj"

echo Building Bomza DLL...
%MSBUILD% %PROJ% /p:Configuration=Release /p:Platform=x64 /p:TargetName="bomzarust" /m
if errorlevel 1 exit /b 1

echo.
echo Output:
echo   %~dp0x64\Release\bomzarust.dll
echo.
endlocal
