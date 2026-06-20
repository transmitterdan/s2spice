@echo off
setlocal enabledelayedexpansion

echo Fetching latest CMake version...
for /f "tokens=2 delims=:," %%A in ('
  curl -s https://api.github.com/repos/Kitware/CMake/releases/latest ^
  ^| findstr /i "tag_name"
') do set TAG=%%~A

set TAG=%TAG:"=%
echo Latest CMake tag: %TAG%

set CMAKE_ZIP=cmake-%TAG%-windows-x86_64.zip
set CMAKE_URL=https://github.com/Kitware/CMake/releases/download/%TAG%/%CMAKE_ZIP%

echo Downloading %CMAKE_ZIP%...
curl -L -o %CMAKE_ZIP% %CMAKE_URL%

echo Extracting...
tar -xf %CMAKE_ZIP%

echo Adding CMake to PATH...
set CMAKE_DIR=%CD%\cmake-%TAG%-windows-x86_64\bin
set PATH=%CMAKE_DIR%;%PATH%

echo Installed CMake version:
cmake --version

endlocal
