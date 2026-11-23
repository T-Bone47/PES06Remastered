@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d "%~dp0"

set MH=third_party\MinHook

cl /nologo /W3 /O2 /MT /EHsc /std:c++17 /D_CRT_SECURE_NO_WARNINGS /DREMASTER_TEST ^
   /Fo:tests\ /Fd:tests\ ^
   /I third_party ^
   /I "%MH%\include" ^
   /I "src" ^
   tests\test_core.cpp ^
   core\core.cpp ^
   graphics\graphics.cpp ^
   graphics\hires.cpp ^
   graphics\postprocess.cpp ^
   graphics\modern_hud.cpp ^
   graphics\font_atlas.cpp ^
   analytics\analytics.cpp ^
   telemetry\telemetry.cpp ^
   telemetry\ipc_pipe.cpp ^
   broadcast\director.cpp ^
   gameplay\enhanced.cpp ^
   io\fileio.cpp ^
   assets\assets.cpp ^
   assets\png_loader.cpp ^
   "%MH%\src\hook.c" ^
   "%MH%\src\trampoline.c" ^
   "%MH%\src\buffer.c" ^
   "%MH%\src\hde\hde32.c" ^
   /link /OUT:tests\test_core.exe /INCREMENTAL:NO user32.lib ole32.lib windowscodecs.lib gdi32.lib shell32.lib advapi32.lib

if errorlevel 1 (
  echo.
  echo UNIT TEST BUILD FAILED
  exit /b 1
)
echo.
echo Built tests\test_core.exe successfully.
exit /b 0
