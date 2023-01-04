:: Build and upload s2spice artifacts
@echo on

setlocal enabledelayedexpansion

if "%CONFIGURATION%" == "" (set CONFIGURATION=Release)
if "%APPVEYOR_BUILD_FOLDER%" == "" (set "APPVEYOR_BUILD_FOLDER=%~dp0..")

where dumpbin.exe >nul 2>&1
if errorlevel 1 (
  set "VS_BASE=C:\Program Files\Microsoft Visual Studio\2022"
  call "%VS_BASE%\Community\VC\Auxiliary\Build\vcvars32.bat"
)

call %APPVEYOR_BUILD_FOLDER%\github-windeps.bat

where wxmsw32u_qa_vc14x.dll >nul 2>&1
if errorlevel 1 (
  set "PATH=%PATH%;%wxWidgets_LIB_DIR%"
  echo Appending %wxWidgets_LIB_DIR% to PATH
)

cd %APPVEYOR_BUILD_FOLDER%

if exist build (rmdir /q /s build)
mkdir build && cd build

cmake -T v143 ^
    -DCMAKE_GENERATOR_PLATFORM=Win32 ^
    -A Win32 -G "Visual Studio 17 2022" ^
    -DwxWidgets_ROOT_DIR=%wxWidgets_ROOT_DIR% ^
    -DwxWidgets_LIB_DIR=%wxWidgets_LIB_DIR% ^
    -DwxWidgets_CONFIGURATION=mswu ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DOCPN_CI_BUILD=ON ^
    ..

cmake --build . --config %CONFIGURATION%
cmake --build . --config %CONFIGURATION% --target package

REM deploy:
7z a -tzip s2spice.zip *.exe
cloudsmith --help
cloudsmith push raw "%CLOUDSMITH_REPO%" s2spice.zip --version "1.0.1" --summary "s2spice - S-parameter utility" --description "See: https://github.com/transmitterdan/s2spice"
