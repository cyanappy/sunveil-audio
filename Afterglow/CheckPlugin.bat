@echo off
setlocal
pushd "%~dp0"
Tools\AfterglowHostTests.exe "Afterglow.vst3\Contents\x86_64-win\Afterglow.vst3" > Afterglow-Windows-check.txt 2>&1
if errorlevel 1 goto failed
Tools\AfterglowRealtimeTests.exe >> Afterglow-Windows-check.txt 2>&1
if errorlevel 1 goto failed
Tools\AfterglowEditorTests.exe >> Afterglow-Windows-check.txt 2>&1
if errorlevel 1 goto failed
echo All Windows checks passed. See Afterglow-Windows-check.txt.
popd
pause
exit /b 0
:failed
echo A check failed. See Afterglow-Windows-check.txt and send that log for troubleshooting.
type Afterglow-Windows-check.txt
popd
pause
exit /b 1
