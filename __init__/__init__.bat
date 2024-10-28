@echo off

if /i "%BITTOOLS_PROJECT_ROOT_INIT0_DIR%" == "%~dp0" exit /b 0

set "BITTOOLS_PROJECT_ROOT_INIT0_DIR=%~dp0"

rem cast to integer
set /A NEST_LVL+=0

call "%%~dp0canonical_path_if_ndef.bat" BITTOOLS_PROJECT_ROOT                   "%%~dp0.."
call "%%~dp0canonical_path_if_ndef.bat" BITTOOLS_PROJECT_EXTERNALS_ROOT         "%%BITTOOLS_PROJECT_ROOT%%/_externals"

if not exist "%BITTOOLS_PROJECT_EXTERNALS_ROOT%\*" (
  echo.%~nx0: error: BITTOOLS_PROJECT_EXTERNALS_ROOT directory does not exist: "%BITTOOLS_PROJECT_EXTERNALS_ROOT%".
  exit /b 255
) >&2

call "%%~dp0canonical_path_if_ndef.bat" PROJECT_OUTPUT_ROOT                     "%%BITTOOLS_PROJECT_ROOT%%/_out"
call "%%~dp0canonical_path_if_ndef.bat" PROJECT_LOG_ROOT                        "%%BITTOOLS_PROJECT_ROOT%%/.log"

call "%%~dp0canonical_path_if_ndef.bat" BITTOOLS_PROJECT_INPUT_CONFIG_ROOT      "%%BITTOOLS_PROJECT_ROOT%%/_config"
call "%%~dp0canonical_path_if_ndef.bat" BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT     "%%PROJECT_OUTPUT_ROOT%%/config/bittools"

rem retarget externals of an external project

call "%%~dp0canonical_path_if_ndef.bat" CONTOOLS_PROJECT_EXTERNALS_ROOT         "%%BITTOOLS_PROJECT_EXTERNALS_ROOT%%"

rem init immediate external projects

if exist "%BITTOOLS_PROJECT_EXTERNALS_ROOT%/contools/__init__/__init__.bat" (
  rem disable code page change in nested __init__
  set /A NO_CHCP+=1
  call "%%BITTOOLS_PROJECT_EXTERNALS_ROOT%%/contools/__init__/__init__.bat" || exit /b
  set /A NO_CHCP-=1
)

rem init external projects

if exist "%BITTOOLS_PROJECT_EXTERNALS_ROOT%/tacklelib/__init__/__init__.bat" (
  rem disable code page change in nested __init__
  set /A NO_CHCP+=1
  call "%%BITTOOLS_PROJECT_EXTERNALS_ROOT%%/tacklelib/__init__/__init__.bat" || exit /b
  set /A NO_CHCP-=1
)

call "%%CONTOOLS_ROOT%%/build/mkdir_if_notexist.bat" "%BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT%" || exit /b

if not defined LOAD_CONFIG_VERBOSE if %INIT_VERBOSE%0 NEQ 0 set LOAD_CONFIG_VERBOSE=1

call "%%CONTOOLS_ROOT%%/build/load_config_dir.bat" %%* -gen_user_config "%%BITTOOLS_PROJECT_INPUT_CONFIG_ROOT%%" "%%BITTOOLS_PROJECT_OUTPUT_CONFIG_ROOT%%" || exit /b

call "%%CONTOOLS_ROOT%%/build/mkdir_if_notexist.bat" "%PROJECT_OUTPUT_ROOT%" || exit /b

if defined CHCP call "%%CONTOOLS_ROOT%%/std/chcp.bat" %%CHCP%%

exit /b 0
