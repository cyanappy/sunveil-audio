@echo off
setlocal
pushd "%~dp0"
Tools\GlassgardenHostTests.exe "Glassgarden.vst3\Contents\x86_64-win\Glassgarden.vst3" > Glassgarden-Windows-check.txt 2>&1
if errorlevel 1 goto failed
Tools\GlassgardenRealtimeTests.exe >> Glassgarden-Windows-check.txt 2>&1
if errorlevel 1 goto failed
Tools\GlassgardenEditorTests.exe >> Glassgarden-Windows-check.txt 2>&1
if errorlevel 1 goto failed
echo All Windows checks passed. See Glassgarden-Windows-check.txt.
popd
pause
exit /b 0
:failed
echo A check failed. See Glassgarden-Windows-check.txt and send that log for troubleshooting.
type Glassgarden-Windows-check.txt
popd
pause
exit /b 1
