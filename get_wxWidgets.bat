::@echo off
setlocal enabledelayedexpansion

set "wxVER=3.3.1"
if [%1] NEQ [] set "wxVER=%1"

:: get current working direction into variable VAR
for /f "tokens=*" %%a in ('cd') do (
    set VAR=%%a
)
set "wxWIN=%VAR%\wxWidgets-%wxVER%"
set "wxWidgets_ROOT_DIR=%wxWIN%"
set "wxWidgets_LIB_DIR=%wxWidgets_ROOT_DIR%\lib"
if exist "%wxWIN%" goto :gotWX
curl --tls-max 1.2 -L --output "wxWidgets-%wxVER%-Dev.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_x64_Dev.7z"
7z t "wxWidgets-%wxVER%-Dev.7z"
if not %errorlevel%==0 (exit /b 1)
7z x -o%WXWIN% "wxWidgets-%wxVER%-Dev.7z"
if not %errorlevel%==0 (exit /b 1)
del "wxWidgets-%wxVER%-Dev.7z"
curl --tls-max 1.2 -L --output "wxWidgets-%wxVER%-Release.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_x64_ReleaseDLL.7z"
7z t "wxWidgets-%wxVER%-Release.7z"
if not %errorlevel%==0 (exit /b 1)
7z x -y -o%WXWIN% "wxWidgets-%wxVER%-Release.7z"
if not %errorlevel%==0 (exit /b 1)
del "wxWidgets-%wxVER%-Release.7z"
curl --tls-max 1.2 -L --output "wxWidgets-headers.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxWidgets-%wxVER%-headers.7z"
if not %errorlevel%==0 (exit /b 1)
7z t "wxWidgets-headers.7z"
if not %errorlevel%==0 (exit /b 1)
7z x -o%WXWIN% "wxWidgets-headers.7z"
if not %errorlevel%==0 (exit /b 1)
del "wxWidgets-headers.7z"
:gotWX

:: for /f "tokens=*" %a in ('dir /b %wxWidgets_LIB_DIR%') do (set VAR=%a)
for /f "tokens=*" %%a in ('dir /b %wxWidgets_LIB_DIR%') do (set VAR=%%a)
set "_addPath=%wxWidgets_LIB_DIR%\%VAR%"
@echo set "wxWidgets_LIB_DIR=%_addPath%" > ".\configdev.bat"
@echo set "wxWidgets_ROOT_DIR=%wxWidgets_ROOT_DIR%" >> ".\configdev.bat"
@echo path^|find /i "%_addpath%"    ^>nul ^|^| set "path=%path%;%_addpath%" >> ".\configdev.bat"
@echo exit /b 0 >> ".\configdev.bat"
rem @echo goto :EOF>> ".\configdev.bat"
endlocal
.\configdev.bat

exit /b 0
