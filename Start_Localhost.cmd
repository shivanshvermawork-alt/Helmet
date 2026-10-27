@echo off
title AegisHelm Localhost Dashboard Gateway
echo ==================================================================
echo   Starting AegisHelm Localhost Web Server & Serial Gateway
echo ==================================================================
echo.
echo 1. Starting Python Server on http://localhost:8000 ...
start "AegisHelm Server" python "%~dp0cockpit_server.py"
timeout /t 2 /nobreak >nul
echo 2. Opening http://localhost:8000 in your browser...
start http://localhost:8000
echo.
echo Gateway is running! Leave the server window open while testing.
echo ==================================================================
pause
