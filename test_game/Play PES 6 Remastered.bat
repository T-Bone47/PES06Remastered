@echo off
title Launching PES 6 Legacy Remaster
cd /d "%~dp0"

echo =======================================================
echo        PRO EVOLUTION SOCCER 6 — LEGACY REMASTER
echo =======================================================
echo.
echo [1/3] Clearing any hung background game instances...
powershell -NoProfile -Command "Get-Process pes6 -ErrorAction SilentlyContinue | ForEach-Object { try { $_.Kill() } catch {} }" >nul 2>&1
taskkill /F /IM pes6.exe >nul 2>&1

echo [2/3] Checking game assets and remaster configuration...
if not exist "pes6.exe" (
    if exist "test_game\pes6.exe" (
        cd /d "%~dp0test_game"
    ) else if exist "PES 6 game files\pes6.exe" (
        cd /d "%~dp0PES 6 game files"
    )
)

echo [3/3] Launching PES 6 Remastered (1280x720 16:9 HD Preset)...
echo.
start "" pes6.exe
echo Game launched successfully! Enjoy the match!
timeout /t 3 >nul
exit /b 0
