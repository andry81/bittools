@echo off

if /i "%BITTOOLS_PROJECT_ROOT_INIT0_DIR%" == "%~dp0" exit /b 0

set "BITTOOLS_PROJECT_ROOT_INIT0_DIR=%~dp0"

if not defined NEST_LVL set NEST_LVL=0

if not defined BITTOOLS_PROJECT_ROOT                        call "%%~dp0canonical_path.bat" BITTOOLS_PROJECT_ROOT                 "%%~dp0.."
if not defined BITTOOLS_PROJECT_EXTERNALS_ROOT              call "%%~dp0canonical_path.bat" BITTOOLS_PROJECT_EXTERNALS_ROOT       "%%BITTOOLS_PROJECT_ROOT%%/_externals"

if not exist "%BITTOOLS_PROJECT_EXTERNALS_ROOT%\*" (
  echo.%~nx0: error: BITTOOLS_PROJECT_EXTERNALS_ROOT directory does not exist: "%BITTOOLS_PROJECT_EXTERNALS_ROOT%".
  exit /b 255
) >&2

if not defined PROJECT_OUTPUT_ROOT                          call "%%~dp0canonical_path.bat" PROJECT_OUTPUT_ROOT                   "%%BITTOOLS_PROJECT_ROOT%%/_out"
if not defined PROJECT_LOG_ROOT                             call "%%~dp0canonical_path.bat" PROJECT_LOG_ROOT                      "%%BITTOOLS_PROJECT_ROOT%%/.log"

if not defined BITTOOLS_PROJECT_INPUT_CONFIG_ROOT           call "%%~dp0canonical_path.bat" BITTOOLS_PROJECT_INPUT_CONFIG_ROOT    "%%BITTOOLS_PROJECT_ROOT%%/_config"
if not defined BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT          call "%%~dp0canonical_path.bat" BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT   "%%PROJECT_OUTPUT_ROOT%%/config/bittools"

rem retarget externals of an external project

if not defined CONTOOLS_PROJECT_EXTERNALS_ROOT              call "%%~dp0canonical_path.bat" CONTOOLS_PROJECT_EXTERNALS_ROOT       "%%BITTOOLS_PROJECT_EXTERNALS_ROOT%%"

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

call "%%CONTOOLS_ROOT%%/build/load_config_dir.bat" %%* -gen_user_config "%%BITTOOLS_PROJECT_INPUT_CONFIG_ROOT%%" "%%BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT%%" || exit /b

if not exist "%PROJECT_OUTPUT_ROOT%\" ( mkdir "%PROJECT_OUTPUT_ROOT%" || exit /b 11 )
if not exist "%PROJECT_LOG_ROOT%\" ( mkdir "%PROJECT_LOG_ROOT%" || exit /b 12 )

if defined CHCP call "%%CONTOOLS_ROOT%%/std/chcp.bat" %%CHCP%%

exit /b 0
