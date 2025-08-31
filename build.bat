@echo off
REM Build script for Search Engine project (Windows)

echo Search Engine Build Script
echo ==========================

REM Check if CMake is available
where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: CMake is not installed or not in PATH!
    echo.
    echo Please install CMake from:
    echo https://cmake.org/download/
    echo.
    echo Remember to:
    echo 1. Check "Add CMake to the system PATH" during installation
    echo 2. Restart your command prompt after installation
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Run cmake
echo Running CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo.
echo Executables created:
echo   - build\Release\spider.exe (Web crawler)
echo   - build\Release\search_server.exe (Search web server)
echo.
echo To run:
echo   build\Release\spider.exe config\config.ini
echo   build\Release\search_server.exe config\config.ini

pause