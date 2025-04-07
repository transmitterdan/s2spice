@echo off
REM Run this batch file in the folder where you cloned s2spice.  For example, if you do this it should work:
REM cd myProjectsFolder
REM git clone https:://github.com/transmitterdan/s2spice
REM cd s2spice
REM .\setup_wxWidgets.bat
REM
REM To bypass downloading the wxWidgets libraries do this:
REM .\setup_wxWidgets.bat build

set "wxVER=3.2.7"
for /f "tokens=*" %%a in ('cd') do (
    set VAR=%%a
)
set "wxWIN=%VAR%\wxWidgets-%wxVER%"
set "wxWidgets_ROOT_DIR=%wxWIN%"
if "%1" == "build" goto :build
if exist "%wxWIN%" (rmdir /s /q "%wxWIN%")
curl --tls-max 1.2 -L --output "wxWidgets-%wxVER%-Dev.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_x64_Dev.7z"
if not %errorlevel%==0 (exit /b 1)
7z x -o%WXWIN% "wxWidgets-%wxVER%-Dev.7z"
if not %errorlevel%==0 (exit /b 1)
del "wxWidgets-%wxVER%-Dev.7z"
curl --tls-max 1.2 -L --output "wxWidgets-%wxVER%-Release.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_x64_ReleaseDLL.7z"
if not %errorlevel%==0 (exit /b 1)
7z x -y -o%WXWIN% "wxWidgets-%wxVER%-Release.7z"
if not %errorlevel%==0 (exit /b 1)
del "wxWidgets-%wxVER%-Release.7z"
curl --tls-max 1.2 -L --output "wxWidgets-headers.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxWidgets-%wxVER%-headers.7z"
if not %errorlevel%==0 (exit /b 1)
7z x -o%WXWIN% "wxWidgets-headers.7z"
if not %errorlevel%==0 (exit /b 1)
del "wxWidgets-headers.7z"
:build
for /f "tokens=*" %%a in ('dir /b %wxWidgets_ROOT_DIR%\lib') do (
    set VAR=%%a
)
set "WxWidgets_LIB_DIR=%wxWidgets_ROOT_DIR%\lib\%VAR%"
set "Path=%wxWidgets_LIB_DIR%;%PATH%

REM ***************************************************************************
REM * Now we can build s2spice from sources.  We create both a release and a  *
REM * debug version.  The debug version has better error messages if an error *
REM * should happen.                                                          *
REM ***************************************************************************
if exist .\build rmdir /s /q .\build
mkdir build && cd build && cmake -A x64 .. && cmake --build . --config Release --target package && cmake --build . --config Debug
if %errorlevel% == 0 goto :ok
@echo Error during build proces
exit /b 1
:ok
REM ***************************************************************************
REM * Everything went ok without an error so we can try to run s2spice.       *
REM ***************************************************************************
.\Debug\s2spice
if not %errorlevel%==0 (exit /b 1)
