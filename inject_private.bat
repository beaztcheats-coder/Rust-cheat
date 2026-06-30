@echo off
setlocal
:: Copy mesh + decrypts next to the DLL (cheat finds them via DLL directory path)
if exist "%~dp0rust_mesh.tri" (
    copy /Y "%~dp0rust_mesh.tri" "%~dp0x64\Release\rust_mesh.tri" >nul 2>&1
)
if exist "%~dp0rust_decrypts.dat" (
    copy /Y "%~dp0rust_decrypts.dat" "%~dp0x64\Release\rust_decrypts.dat" >nul 2>&1
)
"%~dp0x64\Release\injector.exe" private
endlocal
