@echo off
cd %~dp0
cd ..\..

if "%1"=="clean" (
    if exist build (
        rmdir /s /q build
    )
)

if not exist build (
    mkdir build
)

cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    exit /b %errorlevel%
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)