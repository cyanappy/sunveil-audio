@echo off
setlocal
pushd "%~dp0Tools"
SundriveHostTests.exe "%~dp0Sundrive.vst3\Contents\x86_64-win\Sundrive.vst3"
set "SUNDRIVE_TEST_EXIT=%ERRORLEVEL%"
popd
echo.
if "%SUNDRIVE_TEST_EXIT%"=="0" (
    echo All built-in checks passed.
) else (
    echo A check failed. Keep this window open to review the output above.
)
pause
exit /b %SUNDRIVE_TEST_EXIT%
