@echo off
set CONFIG=Release

if "%~1"=="-debug" (
    set CONFIG=Debug
    echo Building in Debug mode...
) else (
    echo Building in Release mode...
)

cmake -B build -S .
cmake --build build --config %CONFIG%
