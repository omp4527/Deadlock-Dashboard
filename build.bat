@echo off
echo ========================================================
echo   Compiling OS Resource Manager Native REST Backend
echo ========================================================
g++ -std=c++17 backend/main.cpp backend/validation.cpp backend/banker.cpp backend/deadlock_detection.cpp backend/prevention_recovery.cpp backend/test_runner.cpp -lws2_32 -o server.exe

if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed! Check g++ installation.
    pause
    exit /b %errorlevel%
)

echo [SUCCESS] server.exe built successfully!
echo Run 'run.bat' to start the application.
pause
