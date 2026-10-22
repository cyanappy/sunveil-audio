@echo off
setlocal
if not exist "%~dp0Afterglow.vst3\Contents\x86_64-win\Afterglow.vst3" (
 echo Afterglow.vst3 is missing. Extract the entire ZIP before installing.
 pause
 exit /b 1
)
set "AFTERGLOW_INSTALL_DIR=%LOCALAPPDATA%\Programs\Common\VST3\Afterglow.vst3"
echo Installing Sunveil Audio Afterglow to:
echo %AFTERGLOW_INSTALL_DIR%
echo Close your audio host before continuing.
pause
robocopy "%~dp0Afterglow.vst3" "%AFTERGLOW_INSTALL_DIR%" /E /R:1 /W:1 /NFL /NDL /NJH /NJS
if errorlevel 8 (
 echo Installation failed. See the copy error above.
 pause
 exit /b 1
)
echo Afterglow installed. Rescan plugins in your audio host.
echo If needed, add %LOCALAPPDATA%\Programs\Common\VST3 to its VST3 search locations.
pause
exit /b 0
