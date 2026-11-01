#!/bin/bash
if [ ! -f "./server" ]; then
    echo "Compiling server for macOS/Linux..."
    clang++ -std=c++17 backend/main.cpp backend/validation.cpp backend/banker.cpp backend/deadlock_detection.cpp backend/prevention_recovery.cpp backend/test_runner.cpp -o server
fi

echo "Starting OS Resource Manager Dashboard on http://localhost:8080..."
open "http://localhost:8080" 2>/dev/null || xdg-open "http://localhost:8080" 2>/dev/null
./server 8080
