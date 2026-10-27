@echo off
title AegisHelm Stitch IoT Full-Stack System
cd /d "%~dp0"

echo ========================================================
echo   Launching AegisHelm Hardware & Dashboard System
echo ========================================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "arduino\tools\Hardware_Setup_Wizard.ps1"

pause
