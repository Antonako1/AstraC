@echo off
cd %~dp0
cd ..\..
@REM build\Release\AstraC.exe asm .\AS\MAIN.AS verbose warn err verbose
@REM build\Release\AstraC.exe comp .\AC\MAIN.AC verbose warn err verbose
build\Release\AstraC.exe comp .\AC\TEST.AC warn err