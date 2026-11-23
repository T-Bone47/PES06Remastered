@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "C:\Users\olive\Downloads\PES6-Remaster\tools\rea\dx8"
cl /nologo /W3 /O2 /EHa /Fe:probe_matrix.exe probe_matrix.cpp /link user32.lib
if errorlevel 1 exit /b 1
echo ==== slot 13 : createdevice, MS layout, HAL ====
probe_matrix.exe 13 createdevice
echo ==== slot 13 : createdevice, MS layout, REF ====
set USE_REF=1
probe_matrix.exe 13 createdevice
set USE_REF=
echo ==== slot 13 : createdevice, MIN layout, REF ====
set USE_REF=1
set USE_MIN=1
probe_matrix.exe 13 createdevice
set USE_REF=
set USE_MIN=
echo ==== slot 13 : hmodule (RegisterSoftwareDevice style) ====
probe_matrix.exe 13 hmodule
echo ==== slot 13 : modecount (UINT style) ====
probe_matrix.exe 13 modecount
echo ==== slot 14 : hmodule ====
probe_matrix.exe 14 hmodule
echo ==== slot 14 : modecount ====
probe_matrix.exe 14 modecount
exit /b 0

