@echo off
setlocal

rem Always use the SmartPlant project directory containing this file.
cd /d "%~dp0"

if not exist "backend\" (
    echo [ERROR] The backend folder was not found.
    echo Place this file in the SmartPlant_ESP32 project root.
    pause
    exit /b 1
)

if not exist "backend\.venv\Scripts\python.exe" (
    echo [ERROR] The backend Python environment was not found.
    echo Expected: backend\.venv\Scripts\python.exe
    pause
    exit /b 1
)

echo Starting the SmartPlant backend...
echo Local test:    http://127.0.0.1:8000/health
echo Stop server:   Ctrl+C
echo.

"backend\.venv\Scripts\python.exe" -m uvicorn backend.app:app --app-dir . --host 0.0.0.0 --port 8000

echo.
echo The SmartPlant backend has stopped.
pause

