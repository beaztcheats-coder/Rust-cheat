@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl.exe /O2 /EHsc /Fe:"E:\github\rust\x64\Release\injector.exe" "E:\github\rust\injector.cpp" /link user32.lib
del injector.obj
