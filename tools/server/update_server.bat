@echo off
title Rust Server Updater
echo =========================================
echo    Rust Dedicated Server Updater
echo =========================================
echo.
echo This downloads/updates the Rust Dedicated
echo Server to the latest version.
echo.
echo Size: ~5 GB  |  Time: 5-15 min
echo =========================================
echo.

set STEAMCMD=C:\steamcmd\steamcmd.exe
set SERVER_DIR=C:\steamcmd\steamapps\common\rust_dedicated

echo [%time%] Starting update...

if not exist "%STEAMCMD%" (
    echo [%time%] SteamCMD not found. Downloading...
    mkdir C:\steamcmd
    powershell -Command "& { Invoke-WebRequest -Uri 'https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip' -OutFile 'C:\steamcmd\steamcmd.zip' }"
    powershell -Command "& { Expand-Archive 'C:\steamcmd\steamcmd.zip' -DestinationPath 'C:\steamcmd' -Force }"
    echo [%time%] SteamCMD downloaded.
)

echo [%time%] Running SteamCMD (app_update 258550)...
cd C:\steamcmd

"%STEAMCMD%" +login anonymous +app_update 258550 validate +quit

echo.
echo =========================================
echo [%time%] Update complete!
echo Server files: %SERVER_DIR%
echo =========================================
echo.
echo Run launch_rust_server.bat to start the server.
pause
