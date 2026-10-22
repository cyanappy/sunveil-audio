@echo off
setlocal
if not exist "%~dp0Glassgarden.vst3\Contents\x86_64-win\Glassgarden.vst3" (
 echo Glassgarden.vst3 is missing. Extract the entire ZIP before installing.
 pause
 exit /b 1
)
set "GLASSGARDEN_INSTALL_DIR=%LOCALAPPDATA%\Programs\Common\VST3\Glassgarden.vst3"
echo Installing Sunveil Audio Glassgarden to:
echo %GLASSGARDEN_INSTALL_DIR%
echo Close your audio host before continuing.
pause
robocopy "%~dp0Glassgarden.vst3" "%GLASSGARDEN_INSTALL_DIR%" /E /R:1 /W:1 /NFL /NDL /NJH /NJS
if errorlevel 8 (
 echo Installation failed. See the copy error above.
 pause
 exit /b 1
)
echo Glassgarden installed. Rescan plugins in your audio host.
echo If needed, add %LOCALAPPDATA%\Programs\Common\VST3 to its VST3 search locations.
pause
exit /b 0
