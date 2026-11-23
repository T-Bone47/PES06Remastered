@echo off
rem Builds the 32-bit d3d8 proxy (static CRT, no external dependencies).
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "C:\Users\olive\Downloads\PES6-Remaster\tools\rea\dx8"
cl /nologo /W3 /O2 /MT /GS- /LD /D_CRT_SECURE_NO_WARNINGS d3d8.cpp /link /DEF:d3d8.def /OUT:d3d8.dll user32.lib
if errorlevel 1 exit /b 1
echo built d3d8.dll
dumpbin /nologo /exports d3d8.dll
exit /b 0

