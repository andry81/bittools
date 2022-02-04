@echo off

setlocal

set "FILE_IN=%~1"

if "%FILE_IN%" == "" (
  echo.%~nx0: error: FILE_IN is not defined.
  exit /b 255
) >&2

set "FILE_IN=%~f1"
set "FILE_NAME_IN=%~n1"
set "FILE_EXT_IN=%~x1"
set "FILE_DIR_IN=%~dp1"

set "BITS_PER_BAUD=%~2"
set "STREAM_BYTE_SIZE=%~3"

set "SYNCSEQ_INT32=%~4"
set "SYNCSEQ_BIT_SIZE=%~5"
set "SYNCSEQ_REPEAT=%~6"

set "BARE_FLAGS="
:FLAGS_LOOP
if "%~7" == "" goto FLAGS_LOOP_END
set BARE_FLAGS=%BARE_FLAGS% %7
shift
goto FLAGS_LOOP

:FLAGS_LOOP_END
echo ^>%FILE_NAME_IN%.*%FILE_EXT_IN%
echo.

call :CMD "%%~dp0bitsync.exe" /s "%%STREAM_BYTE_SIZE%%" gen "%%BITS_PER_BAUD%%" "%%FILE_IN%%" . || exit /b 255

for /F "usebackq eol= tokens=* delims=" %%i in (`dir /A:-D /B /O:N "%FILE_DIR_IN%%FILE_NAME_IN%.*%FILE_EXT_IN%"`) do (
  set "FILE_NAME=%%~ni"
  call :CMD "%%~dp0bitsync.exe"%%BARE_FLAGS%% /disable-calc-autocorr-mean /autocorr-min 0.99 /s "%%STREAM_BYTE_SIZE%%" /r "%%SYNCSEQ_REPEAT%%" /q "%%SYNCSEQ_BIT_SIZE%%" /k "%%SYNCSEQ_INT32%%" sync "%%i" . ^
  && call :CMD "%%~dp0bitsync.exe" /g "%%FILE_NAME:*.=%%" gen "%%BITS_PER_BAUD%%" "%%FILE_IN%%" .
)

exit /b 0

:CMD
echo.^>%*
(
  %*
)
exit /b
