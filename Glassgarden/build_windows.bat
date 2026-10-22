@echo off
setlocal
pushd "%~dp0"
where cmake >nul 2>&1
if errorlevel 1 (
 echo CMake was not found. Use the Visual Studio 2022 developer command prompt
 echo with CMake tools, Desktop development with C++, and the Windows SDK installed.
 goto failed
)
cmake -S . -B Build -G "Visual Studio 17 2022" -A x64 -DGLASSGARDEN_BUILD_TESTS=ON
if errorlevel 1 goto failed
cmake --build Build --config Release --parallel
if errorlevel 1 goto failed
ctest --test-dir Build -C Release --output-on-failure
if errorlevel 1 goto failed
echo Build and tests passed: Build\Glassgarden.vst3
popd
pause
exit /b 0
:failed
echo Build or tests failed. Read the error above.
popd
pause
exit /b 1
