@echo off

rem USAGE:
rem   05_install.bat <build-type> <build-target> <cmake-cmdline>...
rem 
rem Description:
rem   <build-type>: r | d | rd | rm | *
rem     r   - Release
rem     d   - Debug
rem     rd  - ReleaseWidthDebug
rem     rm  - MinSizeRelease
rem     *   - all types from `CMAKE_CONFIG_TYPES` variable

setlocal

call "%%~dp0__init__/script_init.bat" %%0 %%* || exit /b
if %IMPL_MODE%0 EQU 0 exit /b

call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callsub.bat" "%%CONTOOLS_BUILD_TOOLS_ROOT%%/check_config_expiration.bat" ^
  -optional_compare "%%CMAKE_CONFIG_VARS_SYSTEM_FILE_IN%%" "%%CMAKE_CONFIG_VARS_SYSTEM_FILE%%" || exit /b

call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callsub.bat" "%%CONTOOLS_BUILD_TOOLS_ROOT%%/check_config_expiration.bat" ^
  -optional_compare "%%CMAKE_CONFIG_VARS_USER_0_FILE_IN%%" "%%CMAKE_CONFIG_VARS_USER_0_FILE%%" || exit /b

set /A NEST_LVL+=1

call :MAIN %%*
set LAST_ERROR=%ERRORLEVEL%

set /A NEST_LVL-=1

exit /b %LAST_ERROR%

:MAIN
rem CAUTION: an empty value and `*` value has different meanings!
rem
set "CMAKE_BUILD_TYPE=%~1"
rem cmake install does not support particular target installation
set "CMAKE_BUILD_TARGET=%~2"

if not defined CMAKE_BUILD_TYPE (
  echo.%~nx0: error: CMAKE_BUILD_TYPE must be defined.
  exit /b 255
) >&2

rem CAUTION:
rem   This declares only most probable variant (guess) respective to the script extension.
rem   If not then the user have to explicitly pass the target name.
rem
if not defined CMAKE_BUILD_TARGET set "CMAKE_BUILD_TARGET=INSTALL"

rem preload configuration files only to make some checks
call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callln.bat" "%%CONTOOLS_ROOT%%/cmake/set_vars_from_files.bat" ^
  "%%CMAKE_CONFIG_VARS_SYSTEM_FILE:;=\;%%" "WIN" . . . ";" ^
  --exclude_vars_filter "BITTOOLS_PROJECT_ROOT" ^
  --ignore_late_expansion_statements || exit /b 255

rem check if selected generator is a multiconfig generator
call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callln.bat" "%%CONTOOLS_ROOT%%/cmake/get_GENERATOR_IS_MULTI_CONFIG.bat" "%%CMAKE_GENERATOR%%" || exit /b 255

if "%CMAKE_BUILD_TYPE%" == "*" (
  for %%i in (%CMAKE_CONFIG_TYPES:;= %) do (
    set "CMAKE_BUILD_TYPE=%%i"
    call :INSTALL %%* || exit /b
  )
) else (
  call :INSTALL %%*
)

exit /b

:INSTALL
if not defined CMAKE_BUILD_TYPE goto INIT2
if not defined CMAKE_CONFIG_ABBR_TYPES goto INIT2

call "%%CONTOOLS_ROOT%%/cmake/update_build_type.bat" "%%CMAKE_BUILD_TYPE%%" "%%CMAKE_CONFIG_ABBR_TYPES%%" "%%CMAKE_CONFIG_TYPES%%" || exit /b

:INIT2
if %GENERATOR_IS_MULTI_CONFIG%0 EQU 0 (
  call "%%CONTOOLS_ROOT%%/cmake/check_build_type.bat" "%%CMAKE_BUILD_TYPE%%" "%%CMAKE_CONFIG_TYPES%%" || exit /b
)

setlocal

rem load configuration files again unconditionally
set "CMAKE_BUILD_TYPE_ARG=%CMAKE_BUILD_TYPE%"
if not defined CMAKE_BUILD_TYPE_ARG set "CMAKE_BUILD_TYPE_ARG=."
rem escape all values for `--make_vars`
set "PROJECT_ROOT_ESCAPED=%BITTOOLS_PROJECT_ROOT:\=/%"
set "PROJECT_ROOT_ESCAPED=%PROJECT_ROOT_ESCAPED:;=\;%"
call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callln.bat" "%%CONTOOLS_ROOT%%/cmake/set_vars_from_files.bat" ^
  "%%CMAKE_CONFIG_VARS_SYSTEM_FILE:;=\;%%;%%CMAKE_CONFIG_VARS_USER_0_FILE:;=\;%%" "WIN" . "%%CMAKE_BUILD_TYPE_ARG%%" . ";" ^
  --make_vars ^
  "CMAKE_CURRENT_PACKAGE_NEST_LVL;CMAKE_CURRENT_PACKAGE_NEST_LVL_PREFIX;CMAKE_CURRENT_PACKAGE_NAME;CMAKE_CURRENT_PACKAGE_SOURCE_DIR;CMAKE_TOP_PACKAGE_NAME;CMAKE_TOP_PACKAGE_SOURCE_DIR" ^
  "0;00;%%PROJECT_NAME%%;%%PROJECT_ROOT_ESCAPED%%;%%PROJECT_NAME%%;%%PROJECT_ROOT_ESCAPED%%" ^
  --ignore_statement_if_no_filter --ignore_late_expansion_statements || exit /b

call "%%CONTOOLS_ROOT%%/cmake/make_output_directories.bat" "%%CMAKE_BUILD_TYPE%%" "%%GENERATOR_IS_MULTI_CONFIG%%" || exit /b

set "CMDLINE_FILE_IN=%BITTOOLS_PROJECT_INPUT_CONFIG_ROOT%\_build\%?~n0%\cmdline%?~x0%.in"

rem for safe parse
setlocal ENABLEDELAYEDEXPANSION

rem load command line from file with expansion
for /F "usebackq eol=# tokens=* delims=" %%i in (%CMDLINE_FILE_IN%) do call set "CMAKE_CMD_LINE=!CMAKE_CMD_LINE! %%i"

rem safe variable return over endlocal with delayed expansion
for /F "tokens=* delims="eol^= %%i in ("!CMAKE_CMD_LINE!") do endlocal & set "CMAKE_CMD_LINE=%%i"

call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callln.bat" pushd "%%CMAKE_BUILD_DIR%%" || goto INSTALL_END

call "%%CONTOOLS_BUILD_TOOLS_ROOT%%/callln.bat" cmake%%CMAKE_CMD_LINE%%

popd

:INSTALL_END
exit /b
