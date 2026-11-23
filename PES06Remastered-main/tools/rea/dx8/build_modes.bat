@echo off
rem Builds the three bisect variants of the d3d8 proxy.
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "C:\Users\olive\Downloads\PES6-Remaster\tools\rea\dx8"
for %%M in (1 2 3) do (
  cl /nologo /W3 /O2 /MT /GS- /LD /D_CRT_SECURE_NO_WARNINGS /DDX8_MODE=%%M d3d8.cpp /link /DEF:d3d8.def /OUT:d3d8_mode%%M.dll user32.lib
  if errorlevel 1 exit /b 1
)
echo built mode1 mode2 mode3
exit /b 0

