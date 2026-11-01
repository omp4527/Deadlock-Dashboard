@echo off
echo ========================================================
echo   Launching OS Resource Manager Dashboard
echo ========================================================
if not exist server.exe (
    echo server.exe not found! Compiling backend first...
    call build.bat
)

echo Starting Native REST Backend on http://localhost:8080...
start "" "http://localhost:8080"
.\server.exe 8080
