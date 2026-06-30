@echo off
title Frida IL2CPP Full Bridge Dumper
echo =============================================
echo  Frida IL2CPP Full Bridge Dumper for Rust
echo =============================================
echo.
echo PREREQUISITE: RustClient.exe running WITHOUT EAC
echo  (launch it directly, NOT through Steam)
echo =============================================
echo.

echo Attaching Frida via Python host...
echo Output will appear below. Wait for "DUMP COMPLETE".
echo =============================================
echo.

"C:\Users\ludwi\AppData\Local\Python\pythoncore-3.14-64\python.exe" "E:\github\rust\tools\frida\frida_run.py"

echo.
echo =============================================
echo DUMP COMPLETE
echo Results: E:\github\rust\frida_dump.txt
echo =============================================
start notepad "E:\github\rust\frida_dump.txt"
pause
