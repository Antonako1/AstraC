@echo off
REM USAGE: COMPILE.BAT <source_file> <output_file>

where /q cl.exe >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Microsoft Visual C++ Build Tools not found. Please install them to compile the project.
    exit /b 1
)

cl.exe /nologo /W3 /O2 /D_CRT_SECURE_NO_WARNINGS /Fe%2 %1
if errorlevel 1 (
    echo [ERROR] Compilation failed.
    exit /b 1
)