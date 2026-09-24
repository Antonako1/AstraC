@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%..\.."
set "NSIS_DIR=%PROJECT_ROOT%\NSIS"
set "BUILD_DIR=%PROJECT_ROOT%\build"

set "NSIS="
if exist "%ProgramFiles%\NSIS\makensis.exe"                   set "NSIS=%ProgramFiles%\NSIS\makensis.exe"
if exist "%ProgramFiles%\..\NSIS\makensis.exe"                 set "NSIS=%ProgramFiles%\..\NSIS\makensis.exe"
if exist "%ProgramW6432%\NSIS\makensis.exe"                    set "NSIS=%ProgramW6432%\NSIS\makensis.exe"
if exist "%SystemDrive%\Program Files\NSIS\makensis.exe"       set "NSIS=%SystemDrive%\Program Files\NSIS\makensis.exe"
if exist "%SystemDrive%\Program Files (x86)\NSIS\makensis.exe" set "NSIS=%SystemDrive%\Program Files (x86)\NSIS\makensis.exe"
if exist "%ChocolateyInstall%\bin\makensis.exe"                set "NSIS=%ChocolateyInstall%\bin\makensis.exe"
if exist "C:\ProgramData\chocolatey\bin\makensis.exe"          set "NSIS=C:\ProgramData\chocolatey\bin\makensis.exe"
if exist "%LOCALAPPDATA%\Programs\NSIS\makensis.exe"           set "NSIS=%LOCALAPPDATA%\Programs\NSIS\makensis.exe"

if "%NSIS%"=="" (
    where makensis.exe >nul 2>nul
    if %ERRORLEVEL% equ 0 set "NSIS=makensis.exe"
)

if "%NSIS%"=="" (
    echo ERROR: NSIS not found. Install NSIS from https://nsis.sourceforge.io/
    exit /b 1
)

set "NOBUILD="
set "BATCH="
if defined CI set "BATCH=1"

:parse_args
if "%~1"=="" goto done_args
if /i "%~1"=="-nobuild" set "NOBUILD=1"
if /i "%~1"=="-batch"   set "BATCH=1"
if /i "%~1"=="-ci"      set "BATCH=1"
if /i "%~1"=="-noconfirm" set "BATCH=1"
shift
goto parse_args
:done_args

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
        if defined BATCH (
            echo Build configuration failed. Exiting.
            popd & popd & exit /b 1
        )
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

if not defined BATCH (
    echo Run the installer to install AstraC, (Y/N).
    set /p "RUN_INSTALLER=>"
    if /i "!RUN_INSTALLER!"=="Y" (
        start "" "%BUILD_DIR%\AstraC-Setup-%VERSION%.exe"
        echo Installer started. Follow the prompts to complete the installation.
    )
)

popd
