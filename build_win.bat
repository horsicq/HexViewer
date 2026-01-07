@echo off
setlocal enabledelayedexpansion
cls
set "VSVARS_PATH="

for /f "tokens=*" %%a in ('
    reg query "HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall" /s /f "Visual Studio" ^|
    findstr "HKEY"
') do (
    for /f "tokens=2*" %%b in ('
        reg query "%%a" /v InstallLocation 2^>nul ^|
        findstr /i "InstallLocation"
    ') do (
        if not "%%c"=="" (
            if exist "%%c\VC\Auxiliary\Build\vcvars64.bat" (
                set "VSVARS_PATH=%%c\VC\Auxiliary\Build\vcvars64.bat"
                goto :found_vs
            )
        )
    )
)

:found_vs
if not defined VSVARS_PATH (
    echo ERROR: Visual Studio not found.
    exit /b 1
)

echo Using VS environment: %VSVARS_PATH%
call "%VSVARS_PATH%"

if "%~1"=="clean" (
    echo Cleaning build directory...
    rmdir /s /q build
	rmdir /s /q obj 2>nul
    exit /b 0
)

if not exist build mkdir build
cd build

echo Running CMake configuration...
cmake -S .. -B . -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)

echo Building with Ninja...
ninja -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Output: build\HexViewer.exe

cd ..
pause
