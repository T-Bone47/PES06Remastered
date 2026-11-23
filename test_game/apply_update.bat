@echo off
echo Waiting for pes6.exe to close...
:loop
tasklist /fi "imagename eq pes6.exe" | find /i "pes6.exe" >nul
if not errorlevel 1 (
    timeout /t 1 /nobreak >nul
    goto loop
)
if exist remaster.asi.new (
    move /y remaster.asi.new remaster.asi >nul
    echo Remaster binary updated successfully!
)
