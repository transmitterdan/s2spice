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

## Typical Usage

![LTspice schematic](Test/Screenshot%205.png)

![LTspice schematic](Test/Screenshot%206.png)

## Installation (Windows)
Download the .zip file https://drive.google.com/file/d/1B4nMLFFSjssAPiJLtuuhsTYb2SGO_Ivc/view

Unzip the file into a folder called s2spice.

## Windows Usage Example 
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

___THE NEXT PART IS FOR EXPERIENCED DEVELOPERS ONLY___

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
Install 7zip: https://www.7-zip.org/download.html
Install MS Visual Studio 2022 Community edition: https://visualstudio.microsoft.com/vs/community/
Open a "x86 Native Tools Command Prompt for VS 2022" window and issue these commands
```
REM setup a project folder
mkdir myProjects
cd myProjects
git clone https://github.com/transmitterdan/s2spice.git
.\s2spice\setup
If all goes well you should see the s2spice window open.  There are 2 executables, one in the s2spice\build\Release folder, and the other in s2spice\build\Debug folder.  The Debug version is slower but it will give more helpful messages if something goes wrong.
```
