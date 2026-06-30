@echo off
title Rust Local Server — No EAC (Auto-Admin)
mode con: cols=150 lines=2000
echo =========================================
echo    Rust Local Test Server — No EAC
echo =========================================
echo.
echo    Port: 28015    Size: 2000    Seed: 9999
echo    Auto-admin: ON  (spawn/inventory enabled)
echo =========================================
echo.
echo   HOW TO CONNECT (read before server starts):
echo =========================================
echo.
echo   1. Launch RustClient.exe directly (NO Steam):
echo      "C:\Program Files (x86)\Steam\steamapps\common\Rust\RustClient.exe"
echo.
echo   2. In Rust main menu, press F1 and type:
echo      client.connect 127.0.0.1:28015
echo.
echo   3. Make yourself admin (F1 console):
echo      Find your Steam64 ID in server window (look for "Kicking")
echo      Then type: ownerid YOUR_STEAM64_ID
echo.
echo   3. You are AUTO-ADMIN. Use F1 console:
echo      spawn bear      spawn scientist
echo      entity.spawn minicopter.entity
echo      inventory.give ak47.rifle 1
echo      inventory.give ammo.rifle 500
echo.
echo   4. Inject cheat: x64\Release\Rust Private Lite.dll (or Rust Private.dll)
echo      Press HOME for menu
echo =========================================
echo.
echo Press any key to start the server, or CTRL+C to cancel...
pause >nul

cd /d C:\steamcmd\steamapps\common\rust_dedicated

:: Clean only save data (keep config so ownerid persists)
rmdir /s /q "server\local_test" 2>nul
rmdir /s /q "server\local" 2>nul

:: No hardcoded ownerid — add via F1 console after connecting
:: The server log shows your Steam64 when you try to connect.
:: Look for "Kicking XXXXXXXX / Name" — that's your ID.

echo.
echo Starting server (1-3 minutes)...
echo Look for "Spawning done" in output.
echo =========================================
echo.

RustDedicated.exe -batchmode -nographics +server.secure 0 +server.eac 0 +server.encryption 0 +server.writecfg +server.port 28015 +server.level "Procedural Map" +server.seed 9999 +server.worldsize 2000 +server.maxplayers 10 +server.hostname "Local_Test" +server.identity "local"

echo.
echo =========================================
echo    SERVER STOPPED
echo =========================================
echo.
echo    To reconnect: client.connect 127.0.0.1:28015
echo    Launch: RustClient.exe (no Steam)
echo =========================================
pause
