:: Build and upload s2spice artifacts
setlocal enabledelayedexpansion
@echo off

if "%CONFIGURATION%" == "" (set CONFIGURATION=Release)

where dumpbin.exe >nul 2>&1
if errorlevel 1 (
  call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars32.bat"
)

call %GITHUB_WORKSPACE%\github-windeps.bat

where wxmsw32u_qa_vc14x.dll >nul 2>&1
if errorlevel 1 (
  set "PATH=%PATH%;%wxWidgets_LIB_DIR%"
  echo Appending %wxWidgets_LIB_DIR% to PATH
)

pwd
echo GITHUB_WORKSPACE="%GITHUB_WORKSPACE%"
cd "%GITHUB_WORKSPACE%"


if exist build (rmdir /q /s build)
mkdir build && cd build

@echo "Configuring:"
cmake -T v143 ^
    -DCMAKE_GENERATOR_PLATFORM=Win32 ^
    -A Win32 -G "Visual Studio 17 2022" ^
    -DwxWidgets_ROOT_DIR=%wxWidgets_ROOT_DIR% ^
    -DwxWidgets_LIB_DIR=%wxWidgets_LIB_DIR% ^
    -DwxWidgets_CONFIGURATION=mswu ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    ..

call %GITHUB_WORKSPACE%\version.bat

@echo "Building:"
cmake --build . --config %CONFIGURATION% --target package

