#!/usr/bin/env bash

CONFIG="Release"

if [ "$1" = "-debug" ]; then
    CONFIG="Debug"
    echo "Building in Debug mode..."
else
    echo "Building in Release mode..."
fi

cmake -B build -S .
cmake --build build --config $CONFIG
