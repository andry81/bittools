@echo off

if defined BITTOOLS_PROJECT_BUILD_ROOT_INIT0_DIR if exist "%BITTOOLS_PROJECT_BUILD_ROOT_INIT0_DIR%\" exit /b 0

call "%%~dp0..\..\__init__\__init__.bat" || exit /b

set "BITTOOLS_PROJECT_BUILD_ROOT_INIT0_DIR=%~dp0"

exit /b 0

:CANONICAL_PATH
setlocal DISABLEDELAYEDEXPANSION
for /F "eol= tokens=* delims=" %%i in ("%~2\.") do set "RETURN_VALUE=%%~fi"
rem set "RETURN_VALUE=%RETURN_VALUE:\=/%"
(
  endlocal
  set "%~1=%RETURN_VALUE%"
)
exit /b 0
