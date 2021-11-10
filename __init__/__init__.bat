@echo off

if /i "%BITTOOLS_PROJECT_ROOT_INIT0_DIR%" == "%~dp0" exit /b 0

set "BITTOOLS_PROJECT_ROOT_INIT0_DIR=%~dp0"

if not defined NEST_LVL set NEST_LVL=0

if not defined BITTOOLS_PROJECT_ROOT                        call :CANONICAL_PATH BITTOOLS_PROJECT_ROOT                  "%%~dp0.."
if not defined BITTOOLS_PROJECT_EXTERNALS_ROOT              call :CANONICAL_PATH BITTOOLS_PROJECT_EXTERNALS_ROOT        "%%BITTOOLS_PROJECT_ROOT%%/_externals"

if not defined PROJECT_OUTPUT_ROOT                          call :CANONICAL_PATH PROJECT_OUTPUT_ROOT                    "%%BITTOOLS_PROJECT_ROOT%%/_out"
if not defined PROJECT_LOG_ROOT                             call :CANONICAL_PATH PROJECT_LOG_ROOT                       "%%BITTOOLS_PROJECT_ROOT%%/.log"

if not defined BITTOOLS_PROJECT_INPUT_CONFIG_ROOT           call :CANONICAL_PATH BITTOOLS_PROJECT_INPUT_CONFIG_ROOT     "%%BITTOOLS_PROJECT_ROOT%%/_config"
if not defined BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT          call :CANONICAL_PATH BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT    "%%PROJECT_OUTPUT_ROOT%%/config/bittools"

rem init immediate external projects

if exist "%BITTOOLS_PROJECT_EXTERNALS_ROOT%/contools/__init__/__init__.bat" (
  call "%%BITTOOLS_PROJECT_EXTERNALS_ROOT%%/contools/__init__/__init__.bat" || exit /b
)

rem init external projects

if exist "%BITTOOLS_PROJECT_EXTERNALS_ROOT%/tacklelib/__init__/__init__.bat" (
  call "%%BITTOOLS_PROJECT_EXTERNALS_ROOT%%/tacklelib/__init__/__init__.bat" || exit /b
)

if not exist "%BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT%\" ( mkdir "%BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT%" || exit /b 10 )

if not defined LOAD_CONFIG_VERBOSE if %INIT_VERBOSE%0 NEQ 0 set LOAD_CONFIG_VERBOSE=1

call "%%CONTOOLS_ROOT%%/build/load_config_dir.bat" -gen_user_config "%%BITTOOLS_PROJECT_INPUT_CONFIG_ROOT%%" "%%BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT%%" || exit /b

if not exist "%PROJECT_OUTPUT_ROOT%\" ( mkdir "%PROJECT_OUTPUT_ROOT%" || exit /b 11 )
if not exist "%PROJECT_LOG_ROOT%\" ( mkdir "%PROJECT_LOG_ROOT%" || exit /b 12 )

if defined CHCP call "%%CONTOOLS_ROOT%%/std/chcp.bat" %%CHCP%%

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
