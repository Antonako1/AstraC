@echo off
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File BUILD.PS1
if %errorlevel% neq 0 exit /b %errorlevel%
