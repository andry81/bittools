@echo off

setlocal

call "%%~dp0__init__.bat" || goto :EOF

set /A NEST_LVL+=1

pushd "%CMAKE_BUILD_ROOT%" & (
  call :CMD cmake ^
    "%%PROJECT_ROOT%%" ^
    -G "%%CMAKE_GENERATOR_TOOLSET%%" ^
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=%%CMAKE_BIN_ROOT%%" ^
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=%%CMAKE_LIB_ROOT%%" ^
    "-DCMAKE_INSTALL_PREFIX=%%CMAKE_INSTALL_ROOT%%" ^
    "-DCPACK_OUTPUT_FILE_PREFIX=%%CMAKE_CPACK_ROOT%%" ^
    || ( popd & goto EXIT )
  popd
)

:EXIT
set /A NEST_LVL-=1

if %NEST_LVL% EQU 0 pause

exit /b

:CMD
echo.^>%*
(%*)
exit /b
