@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\.."
set "NSIS_DIR=%PROJECT_ROOT%\NSIS"
set "BUILD_DIR=%PROJECT_ROOT%\build"

set "NSIS="
if exist "%ProgramFiles%\NSIS\makensis.exe"                 set "NSIS=%ProgramFiles%\NSIS\makensis.exe"
if exist "%ProgramFiles%\..\NSIS\makensis.exe"               set "NSIS=%ProgramFiles%\..\NSIS\makensis.exe"
if exist "%ProgramW6432%\NSIS\makensis.exe"                  set "NSIS=%ProgramW6432%\NSIS\makensis.exe"
if exist "%SystemDrive%\Program Files\NSIS\makensis.exe"     set "NSIS=%SystemDrive%\Program Files\NSIS\makensis.exe"
if exist "%SystemDrive%\Program Files (x86)\NSIS\makensis.exe" set "NSIS=%SystemDrive%\Program Files (x86)\NSIS\makensis.exe"

if "%NSIS%"=="" (
    echo ERROR: NSIS not found. Install NSIS from https://nsis.sourceforge.io/
    exit /b 1
)

set "NOBUILD="
if /i "%~1"=="-nobuild" set "NOBUILD=1"

pushd "%PROJECT_ROOT%"

echo ============================================
echo AstraC NSIS Installer Builder
echo ============================================
echo.

if defined NOBUILD (
    echo [1/3] Skipping project build (-nobuild)
) else (
    echo [1/3] Building AstraC project...
    if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
    pushd "%BUILD_DIR%"
    cmake -G "Visual Studio 17 2022" -A x64 ..
    if %ERRORLEVEL% neq 0 ( 
        set /p "answ=Delete the build directory and try again? (Y/N): "
        if /i "!answ!"=="Y" (
            popd
            rmdir /s /q "%BUILD_DIR%"
            mkdir "%BUILD_DIR%"
            pushd "%BUILD_DIR%"
            cmake -G "Visual Studio 17 2022" -A x64 ..
            if %ERRORLEVEL% neq 0 ( popd & popd & exit /b 1 )
        ) else (
            echo Build failed. Exiting.
            popd & popd & exit /b 1
        )
    )
    cmake --build . --config Release
    if %ERRORLEVEL% neq 0 ( popd & popd & exit /b 1 )
    popd
    echo Build completed.
)

if not exist "%BUILD_DIR%\Release\AstraC.exe" (
    echo ERROR: %BUILD_DIR%\Release\AstraC.exe not found. Build the project first.
    popd & exit /b 1
)

echo.
echo [2/3] Building NSIS installer...
pushd "%NSIS_DIR%"
"%NSIS%" INSTALLER.NSI
if %ERRORLEVEL% neq 0 ( popd & popd & exit /b %ERRORLEVEL% )
popd

echo.
echo ============================================
echo [3/3] Done!

REM Read version from VERSION.txt
set "VERSION_FILE=%PROJECT_ROOT%\VERSION\VERSION.txt"


for /f "usebackq delims=" %%v in ("%VERSION_FILE%") do set "VERSION=%%v"
echo   %BUILD_DIR%\AstraC-Setup-%VERSION%.exe
echo ============================================

echo Run the installer to install AstraC, (Y/N).

set /p "RUN_INSTALLER=>"
if /i "%RUN_INSTALLER%"=="Y" (
    start "" "%BUILD_DIR%\AstraC-Setup-%VERSION%.exe"
    echo Installer started. Follow the prompts to complete the installation.
)

popd
