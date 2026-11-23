@echo off
rem Builds remaster.asi - the PES 6 Legacy Remaster.
rem
rem   src/       upstream PESMod gameplay sources (GPL-3.0-or-later)
rem   core/      manifest, config, logger, hook registry, build gate
rem   graphics/  resolution / window hooks
rem   assets/    AFS asset replacement
rem
rem MinHook is compiled from vendored upstream source, so the ASI has no
rem external DLL dependencies.
rem
rem Bisect switches (optional):
rem   set DX8_NO_UPSTREAM=1   exclude the upstream gameplay layer
rem   set DX8_NO_ASSETS=1     exclude the assets layer

setlocal EnableDelayedExpansion
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
)
cd /d "%~dp0"

if not exist build mkdir build
del /q build\*.obj 2>nul

set MH=third_party\MinHook

rem ---- collect the upstream gameplay sources (its own DllMain is replaced) ----
set SOURCES=
if not "%DX8_NO_UPSTREAM%"=="1" for /r "src" %%F in (*.cpp) do call :addsrc "%%F"

rem ---- optional layers ----
set ASSETS=assets\assets.cpp assets\png_loader.cpp
if "%DX8_NO_ASSETS%"=="1" set ASSETS=
set EXTRA=
if "%DX8_NO_UPSTREAM%"=="1" set EXTRA=/DREMASTER_NO_UPSTREAM

cl /nologo /W3 /O2 /MT /EHsc /GS- /LD /std:c++17 /D_CRT_SECURE_NO_WARNINGS !EXTRA! ^
   /Fo:build\ /Fd:build\ ^
   /I third_party ^
   /I "%MH%\include" ^
   /I "src" ^
   remaster.cpp ^
   core\core.cpp ^
   graphics\graphics.cpp ^
   graphics\d3d8.cpp ^
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
   !ASSETS! ^
   !SOURCES! ^
   "%MH%\src\hook.c" ^
   "%MH%\src\trampoline.c" ^
   "%MH%\src\buffer.c" ^
   "%MH%\src\hde\hde32.c" ^
   /link /OUT:build\remaster.asi /INCREMENTAL:NO user32.lib ole32.lib windowscodecs.lib gdi32.lib shell32.lib advapi32.lib

if errorlevel 1 (
  echo.
  echo BUILD FAILED
  exit /b 1
)
echo.
echo built build\remaster.asi
dir /-c build\remaster.asi
exit /b 0

:addsrc
set NAME=%~nx1
if /I "%~1"=="src\dllmain.cpp" goto :eof
set SOURCES=!SOURCES! "%~1"
goto :eof


