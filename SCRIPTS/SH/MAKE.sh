#!/bin/sh
# Build AstraC using CMake
# Uses Ninja if available, otherwise Unix Makefiles

cd "$(dirname "$0")/../.." || exit 1

if [ "$1" = "clean" ]; then
    rm -rf build
fi

if [ ! -d build ]; then
    mkdir build
fi

cd build || exit 1

if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
else
    GENERATOR="Unix Makefiles"
fi

cmake -G "$GENERATOR" ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    read -p "Delete build directory and try again? (y/n): " answer
    if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then
        cd ..
        rm -rf build
        mkdir build
        cd build || exit 1
        cmake -G "$GENERATOR" ..
        if [ $? -ne 0 ]; then
            echo "CMake configuration failed again. Exiting."
            exit 1
        fi
    else
        exit 1
    fi
fi

cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi
