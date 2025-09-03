:: Build and upload s2spice artifacts
setlocal enabledelayedexpansion
@echo off

if "%CONFIGURATION%" == "" (set CONFIGURATION=Release)
if "%APPVEYOR_BUILD_FOLDER%" == "" (set "APPVEYOR_BUILD_FOLDER=%~dp0..")

where dumpbin.exe >nul 2>&1
if errorlevel 1 (
  set "VS_BASE=C:\Program Files\Microsoft Visual Studio\2022"
  call "%VS_BASE%\Community\VC\Auxiliary\Build\vcvars32.bat"
)

echo "APPVEYOR_BUILD_FOLDER=%APPVEYOR_BUILD_FOLDER%"
call "%APPVEYOR_BUILD_FOLDER%\github-windeps.bat"

cd %APPVEYOR_BUILD_FOLDER%
cd
dir
if exist build (rmdir /q /s build)
mkdir build && cd build
cd
dir
@echo "Configuring:"
cmake -T v143 ^
    -A x64 -G "Visual Studio 17 2022" ^
    -DwxWidgets_ROOT_DIR=%wxWidgets_ROOT_DIR% ^
    -DwxWidgets_LIB_DIR=%wxWidgets_LIB_DIR% ^
    -DwxWidgets_CONFIGURATION=mswu ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    ..

call %APPVEYOR_BUILD_FOLDER%\version.bat

@echo "Building:"
cmake --build . --config %CONFIGURATION% --target package

@echo "Deploying to Cloudsmith:"
7z a -tzip s2spice.zip *.exe
cloudsmith push raw "%CLOUDSMITH_REPO%" s2spice.zip -k "%CLOUDSMITH_API_KEY%" --version "%VERSION_STRING%" --summary "s2spice - S-parameter utility" --description "See: https://github.com/transmitterdan/s2spice"
