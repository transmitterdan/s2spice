### S2SPICE README

Version 1, 2023-JAN-2

S2spice is a utility program that reads S-Parameter files in Touchstone format. It can convert the data into a Spice .SUBCKT format and write it to a .LIB file. The .LIB file can be utilized by Spice simulators such as LTspice or PSpice to analyze the AC performance of the circuit described by the S-Parameters.  This is useful for analyzing systems with components that only have S-Parameter models.

Another feature is the ability to create LTspice symbol (.ASY) files making it easier to add the new library file to a schematic.

### Usage:

Usage is mostly self explanatory.  There are buttons for the following actions:

Open, Make LIB, Make SYM, Quit

## Open

A file open dialog will appear. Select from the dialog any S-Paramter file and press OK.

## Make LIB

Converts the file to Spice .SUBCKT format and writes a file with same name as the S-Parameter file but with extension .lib (or .LIB in Windows).

## Make SYM

Creates a generic LTSpice symbol file that can be used to add the model to a schematic. The pins are evenly divided between left and right sides except for the reference node pin which is on the bottom. You can edit this file in the LTspice symbol editor. The ports are numbered the same as in the S-Parameter file ports with on extra port for the reference node.  Most users will connect that pin to GND (node 0).

__DO NOT FORGET TO HOOK UP THE REFERENCE NODE PIN!__

## Quit

Exit the program.

### Examples provided

## Windows 
```
C:\>cd s2spice\Test & rem go to the test folder under s2spice
C:\s2spice\Test>..\s2spice
```
When s2spice starts the window looks like this:
![Screenshot 1](Test/Screenshot%201.png)

Click the "Open" button (or press ctrl-O, or use menu File->Open)
In the file dialog select one of the S-parameter files supplied with s2spice and click Open button.  Here we selected: AD6PS-1+___+25.S7P 
![Screenshot 2](Test/Screenshot%202.png)

Now simply press the 'Make LIB' and/or 'Make SYM' buttons to create Spice library file.  The symbol created is for LTspice.

You can open additional files and create more library files and symbols.

### Building for Linux
Release build on debian like (e.g. Ubuntu):
```
sudo apt update
sudo apt install libwxbase3.0-dev  ; or whatever is latest version of wxWidgets
git clone https://github.com/transmitterdan/s2spice.git
git clone https://github.com/libigl/eigen.git
cd s2spice
mkdir build
cd build
cmake ..
cmake --build .
```

### Building for Windows
Release build using MSVC:
This program uses DLLs from the Visual C redistributable from Microsoft as well as wxWidgets DLLs.  Here are some hints to copy these files to the Release folder where s2spice.exe is found. You can learn more at
https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
https://www.wxwidgets.org/downloads/

Install the Microsoft C runtime and wxWidgets libraries from above web sites. Make note where you put the files.

```
cd build
mkdir Release
copy "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\*.*" Release
copy \oa\wxWidgets\lib\vc14x_dll\wxmsw32u_core_vc.dll Release
copy \oa\wxWidgets\lib\vc14x_dll\wxbase32u_vc.dll Release
del CMakeCache.txt
cmake .. -G "Visual Studio 17 2022" -AWIN32 -DWX_LIB_DIR="\oa\wxWidgets\lib"
cmake --build . --config Release
cd ..\Test
..\build\Release\s2spice.exe
```

Windows dependencies:
```
    wxbase32u_vc.dll
    wxmsw32u_core_vc.dll
    MSVCP140.dll
    VCRUNTIME140.dll
    api-ms-win-crt-runtime-l1-1-0.dll
    api-ms-win-crt-stdio-l1-1-0.dll
    api-ms-win-crt-filesystem-l1-1-0.dll
    api-ms-win-crt-heap-l1-1-0.dll
    api-ms-win-crt-convert-l1-1-0.dll
    api-ms-win-crt-math-l1-1-0.dll
    api-ms-win-crt-locale-l1-1-0.dll
```
