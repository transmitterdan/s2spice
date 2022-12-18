### S2SPICE README

Version 1, 2023-JAN-2

S2spice is a utility program that reads S-Parameter files in Touchstone format. 
It can convert the data into a Spice .SUBCKT format and write it to a .LIB file. 
The .LIB file can be utilized by Spice simulators such as LTspice or PSpice to 
analyze the AC performance of the circuit described by the S-Parameters.  This 
is useful for analyzing systems with components that only have S-Parameter 
models.

### Usage:

Usage is self explanatory.  There are buttons for the following actions:

Open, Make LIB, Make SYM, Quit

## Open

A file open dialog will appear. Select from the dialog an S-Paramter file 
and press OK.

## Make LIB

Converts the file to Spice .SUBCKT format and writes a file with same 
name as the S-Parameter file but with extension .lib (or .LIB in Windows).

## Make SYM

Creates a generic LTSpice symbol file that can be used to add the model 
to a schematic.  The pins are all on one side (left side) except for 
the ground reference pin which is on the bottom.  You can edit this file 
in the LTspice symbol editor.  The ports are numbered the same as in 
the S-Parameter file.

## Quit

Exit the program.

### Building for Windows
Release build using MSVC:
This program uses DLLs from the Visual C redistributable from Microsoft as well as wxWidgets DLLs.  Below are hints to copy these to the runtime folder.
```
cd build
copy "C:\Program Files (x86)\Windows Kits\10\Redist\ucrt\DLLs\x86\*.*" Release
copy \oa\wxWidgets\lib\vc143_dll\wxmsw32u_core_vc.dll Release
copy \oa\wxWidgets\lib\vc143_dll\wxbase32u_vc.dll Release
del CMakeCache.txt
cmake .. -G "Visual Studio 17 2022" -AWIN32 -DWX_LIB_DIR="%wxWidgets_LIB_DIR%" && cmake --build . --config Release
```
Windows dependencies:
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

