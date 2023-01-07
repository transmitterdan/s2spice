@echo off
::
:: Install build dependencies. Requires a working choco installation,
:: see https://docs.chocolatey.org/en-us/choco/setup.
::
:: Initial run will do choco installs requiring administrative
:: privileges.
::

:: Install choco cmake and add it's persistent user path element
::
:: set CMAKE_HOME=C:\Program Files\CMake
:: if not exist "%CMAKE_HOME%\bin\cmake.exe" choco install --no-progress -y cmake

:: Update required python stuff
::
python --version > nul 2>&1 && python -m ensurepip > nul 2>&1
if errorlevel 1 choco install --no-progress -y python
python --version
python -m ensurepip
python -m pip install --upgrade pip
python -m pip install -q setuptools wheel
python -m pip install -q --upgrade cloudsmith-cli
python -m pip install -q cryptography

:: Install 7z if needed
::
7z i > nul 2>&1 || choco install -y 7zip

:: Install pre-compiled wxWidgets and other DLL; add required paths.
::
set "wxVER=3.2.1"
for /f "tokens=*" %%a in ('cd') do (
    set VAR=%%a
)
set "wxWIN=%VAR%\wxWidgets-%wxVER%"
set "wxWidgets_ROOT_DIR=%wxWIN%"
if exist "%wxWIN%" (rmdir /s /q "%wxWIN%")
curl --tls-max 1.2 -L --output "wxWidgets-%wxVER%-Dev.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_Dev.7z"
7z x -o%WXWIN% "wxWidgets-%wxVER%-Dev.7z"
del "wxWidgets-%wxVER%-Dev.7z"
curl --tls-max 1.2 -L --output "wxWidgets-%wxVER%-Release.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxMSW-%wxVER%_vc14x_ReleaseDLL.7z"
7z x -y -o%WXWIN% "wxWidgets-%wxVER%-Release.7z"
del "wxWidgets-%wxVER%-Release.7z"
curl --tls-max 1.2 -L --output "wxWidgets-headers.7z" "https://github.com/wxWidgets/wxWidgets/releases/download/v%wxVER%/wxWidgets-%wxVER%-headers.7z"
7z x -o%WXWIN% "wxWidgets-headers.7z"
del "wxWidgets-headers.7z"
for /f "tokens=*" %%a in ('dir /b %wxWidgets_ROOT_DIR%\lib') do (
    set VAR=%%a
)
set "WxWidgets_LIB_DIR=%wxWidgets_ROOT_DIR%\lib\%VAR%"
set "Path=%wxWidgets_LIB_DIR%;%PATH%

exit /b 0
