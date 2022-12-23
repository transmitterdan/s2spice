REM Run this batch file in the folder where you cloned s2spice.  For example, if you do this it should work:
REM cd myProjectsFolder
REM git clone https:://github.com/transmitterdan/s2spice
REM .\s2spice\setup_wxWidgets.bat
set "wxVER=3.2.1"
for /f "tokens=*" %%a in ('cd') do (
    set VAR=%%a
)
set "wxWIN=%VAR%\wxWidgets-%wxVER%"
set "wxWidgets_ROOT_DIR=%wxWIN%"
curl -L -o "wxWidgets-Dev-%wxVER%.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_Dev.7z"
7z x "wxWidgets-Dev-%wxVER%.7z" -o %WXWIN%
del "wxWidgets-Dev-%wxVER%.7z"
curl -L -o "wxWidgets-headers.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxWidgets-%wxVER%-headers.7z"
7z x "wxWidgets-headers.7z" -o %WXWIN%
del "wxWidgets-headers.7z"
cd s2spice
for /f "tokens=*" %%a in ('dir /b %wxWidgets_ROOT_DIR%\lib') do (
    set VAR=%%a
)
set "WxWidgets_LIB_DIR=%wxWidgets_ROOT_DIR%\lib\%VAR%"
set "Path=%wxWidgets_LIB_DIR%;%PATH%
cd s2spice
mkdir build && cd build && cmake -A wIN32 .. && cmake --build . --config Release && cmake --build . --config Debug
if %errorlevel% == 0 goto :ok
@echo Error during build proces
exit /b 1
:ok
Release\s2spice
