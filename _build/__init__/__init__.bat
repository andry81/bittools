@echo off

if defined BITTOOLS_PROJECT_BUILD_ROOT_INIT0_DIR if exist "%BITTOOLS_PROJECT_BUILD_ROOT_INIT0_DIR%\*" exit /b 0

call "%%~dp0..\..\__init__\__init__.bat" || exit /b

set "BITTOOLS_PROJECT_BUILD_ROOT_INIT0_DIR=%~dp0"
