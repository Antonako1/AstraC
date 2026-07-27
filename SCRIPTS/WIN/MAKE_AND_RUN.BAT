@echo off
cd %~dp0
call MAKE.BAT %*

cd %~dp0
call RUN.BAT %*