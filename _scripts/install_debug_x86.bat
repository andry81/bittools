@echo off

setlocal

call "%%~dp0__init__.bat" || goto :EOF

set /A NEST_LVL+=1

pushd "%CMAKE_BUILD_ROOT%" && (
  cmake --build . --config Debug --target INSTALL || ( popd & goto EXIT )
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
