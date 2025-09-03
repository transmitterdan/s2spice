@echo off
REM Run this batch file in the folder where you cloned s2spice.  For example, if you do this it should work:
REM cd myProjectsFolder
REM git clone https:://github.com/transmitterdan/s2spice
REM cd s2spice
REM .\setup_wxWidgets.bat
REM
REM To bypass downloading the wxWidgets libraries do this:
REM .\setup_wxWidgets.bat build

call get_wxWidgets.bat

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
exit /b 0
