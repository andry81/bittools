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

set "SYNCSEQ_BYTES=%~4"
set "SYNCSEQ_BIT_SIZE=%~5"
set "SYNCSEQ_REPEAT=%~6"

echo ^>%FILE_NAME_IN%.*%FILE_EXT_IN%
echo.

call :CMD "%%~dp0bitsync.exe" sync "%%BITS_PER_BAUD%%" -s "%%STREAM_BYTE_SIZE%%" -q "%%SYNCSEQ_BIT_SIZE%%" -r "%%SYNCSEQ_REPEAT%%" -k "%%SYNCSEQ_BYTES%%" "%%FILE_IN%%" . && exit /b 0

call :CMD "%%~dp0bitsync.exe" gen "%%BITS_PER_BAUD%%" -s "%%STREAM_BYTE_SIZE%%" "%%FILE_IN%%" . || exit /b 255

set INDEX=1
for /F "usebackq eol= tokens=* delims=" %%i in (`dir /A:-D /B /O:N "%FILE_DIR_IN%%FILE_NAME_IN%.*%FILE_EXT_IN%"`) do (
  call :CMD "%%~dp0bitsync.exe" sync "%%BITS_PER_BAUD%%" -s "%%STREAM_BYTE_SIZE%%" -q "%%SYNCSEQ_BIT_SIZE%%" -r "%%SYNCSEQ_REPEAT%%" -k "%%SYNCSEQ_BYTES%%" "%%i" . ^
  && call :CMD "%%~dp0bitsync.exe" gen "%%BITS_PER_BAUD%%" -n %%INDEX%% "%%FILE_IN%%" .
  set /A INDEX+=1
)

exit /b 0

:CMD
echo.^>%*
(
  %*
)
exit /b
