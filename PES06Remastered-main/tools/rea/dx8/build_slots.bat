@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "C:\Users\olive\Downloads\PES6-Remaster\tools\rea\dx8"
cl /nologo /W3 /O2 /Fe:probe_slot.exe probe_slot.cpp /link user32.lib
if errorlevel 1 exit /b 1
for %%L in (ms min) do (
  for %%S in (13 14) do (
    echo -------- slot %%S layout %%L --------
    probe_slot.exe %%S %%L
    echo   exitcode=%errorlevel%
  )
)
exit /b 0

