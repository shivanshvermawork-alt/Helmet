@echo off
title AegisHelm Stitch IoT Full-Stack System
echo ========================================================
echo   Launching AegisHelm Stitch Full-Stack IoT System
echo ========================================================
echo.
echo Starting Backend & Serial Gateway on http://localhost:8080 ...
start "AegisHelm Server" cmd /k "cd server && npm start"
timeout /t 3 /nobreak >nul
echo.
echo Opening Live Cockpit Dashboard in your browser...
start http://localhost:8080
echo.
echo System is running!
pause
