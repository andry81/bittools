@echo off

rem The script for cases where the IDE or downpipe system doesn't have apropriate generator in the cmake.
rem For example, it can be the QtCreator.
rem To bypass the problem of inconvinient usage of environment variables in such circumstances and unability
rem to save them in version control system we have to directly generate cmake include file from a template file
rem and do manually change the values in a template. When the IDE has start analyze the cmake list it would
rem regenerate user cmake from the template and include it loading required set of external variables.
rem To pregenerate this include we use this script.
rem 

setlocal

call "%%~dp0__init__.bat" || goto :EOF

set /A NEST_LVL+=1

pushd "%CMAKE_BUILD_ROOT%" & (
  call :CMD cmake ^
    "-DCONFIGURE_IN_FILE=%%PROJECT_ROOT%%/environment_local.cmake.in" ^
    "-DCONFIGURE_OUT_FILE=%%PROJECT_ROOT%%/environment_local.cmake" ^
    "-D_3DPARTY_ROOT=%%_3DPARTY_ROOT%%" ^
    "-DENV_ROOT=%%ENV_ROOT%%" ^
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=%%CMAKE_BIN_ROOT%%" ^
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=%%CMAKE_LIB_ROOT%%" ^
    "-DCMAKE_INSTALL_PREFIX=%%CMAKE_INSTALL_ROOT%%" ^
    "-DCPACK_OUTPUT_FILE_PREFIX=%%CMAKE_CPACK_ROOT%%" ^
    "-DNSIS_INSTALL_ROOT=%%NSIS_INSTALL_ROOT%%" ^
    -P "%%PROJECT_ROOT%%/cmake/ConfigureFile.cmake" ^
    || ( popd & goto EXIT )
  popd
)

:PROCESS_LINE_END

:EXIT
set /A NEST_LVL-=1

if %NEST_LVL% EQU 0 pause

exit /b

:CMD
echo.^>%*
(%*)
exit /b
