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

call get_wxWidgets.bat

exit /b 0
