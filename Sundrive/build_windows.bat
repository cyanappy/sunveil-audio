@echo off
setlocal
where cmake >nul 2>&1
if errorlevel 1 (
    echo Install CMake 3.20 or newer, then run this file again.
    echo https://cmake.org/download/
    exit /b 1
)
cmake -S "%~dp0." -B "%~dp0Build" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build "%~dp0Build" --config Release
if errorlevel 1 exit /b 1
echo.
echo Built: %~dp0Build\Sundrive.vst3
echo Run tests with: ctest --test-dir "%~dp0Build" -C Release --output-on-failure
endlocal
