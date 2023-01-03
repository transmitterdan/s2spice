### S2SPICE README

Version 1, 2023-JAN-2

S2spice is a GUI utility program that reads S-Parameter files in Touchstone format. It can convert the data into a Spice .SUBCKT format and write it to a .LIB file. The .LIB file can be utilized by Spice simulators such as LTspice or PSpice to analyze the AC performance of the circuit described by the S-Parameters.  This is useful for analyzing systems with components that only have S-Parameter models.

Another feature is the ability to create LTspice symbol (.ASY) files making it easier to add the new library file to a schematic.

### Download:

## Binary installation for Windows

https://dl.cloudsmith.io/public/dan-dickey/s2spice/raw/names/s2spice/versions/1.0.0/setup.zip

### Usage:

Usage is mostly self explanatory.  The program is designed to run on a variety of operating systems including Linux and WIndows.  It should also compile and run under MacOS, however I don't have a compiler or system for testing.

There are buttons (and menu options) for the following actions:

Open, Make LIB, Make SYM, Quit

## Open

A file open dialog will appear. Select from the dialog any S-Paramter file and press OK.

## Make LIB

Converts the S-parameter file to Spice .SUBCKT format and writes a file with same name as the S-Parameter file but with extension .lib (or .LIB in Windows).  The .SUBCKT name is the same as the S-paramter file name (without the extension).

## Make SYM

Creates a generic LTSpice symbol file that can be used to add the model to a schematic. The pins are evenly divided between left and right sides except for the reference node pin which is on the bottom. You can edit this file in the LTspice symbol editor. The ports are numbered the same as in the S-Parameter file ports with on extra port for the reference node.  Most users will connect that pin to GND (node 0).

__DO NOT FORGET TO HOOK UP THE REFERENCE NODE PIN!__

## Quit

Exit the program.

### Examples provided

## Typical Usage

![LTspice schematic](Test/Screenshot%205.png)

![LTspice schematic](Test/Screenshot%206.png)

## Windows Usage Example 
```
cd \myProjects\s2spice\build\Release | REM go to the Release folder under s2spice
s2spice.exe
```
When s2spice starts the GUI window looks like this:

![Screenshot 1](Test/Screenshot%201.png)

Click the "Open" button (or press ctrl-O, or use menu File->Open)
In the file dialog navigate to the supplied "Test" folder and select one of the S-parameter files supplied with s2spice and click Open button.  Here we selected: AD6PS-1+___+25.S7P

![Screenshot 2](Test/Screenshot%202.png)

Now simply press the 'Make LIB' and/or 'Make SYM' buttons to create Spice library file.  The symbol created is for LTspice.

You can open additional files and create more library files and symbols.  For example, open file BBP-20R5+_Plus25degC.s2p and make LIB and SYM files.

Demonstrate the newly created components using the supplied LTspice simulation file 'test.asc' found in the Test folder. If you created the necessary LIB and ASY files it should demonstrate the frequency response of a Mini-Circuits 6-way splitter and a bandpass filter.

Any symbols and library files you create from your own S-parameter files can be easily added to a schematic the same way as adding any other device.  Use the 'F2' button or the 'Component' ```NAND``` gate tool.

## Command Line Usage
The program also operates from the command line.  This allows to use s2spice in a batch file or simply when you don't need to use the GUI.
```
Usage: s2spice [-h] [-f] [-l] [-s] [-q] [file name...]
  -h, --help    displays command line options
  -f, --force   overwrite any existing file
  -l, --lib     creates LIB library file
  -s, --symbol  creates ASY symbol file
  -q, --quiet   disables the GUI (for command line only usage)

  [file name] is one or more names of a S-parameter file you wish to read.
  If you do not use the -q (quiet) option then after processing each file 
  on the command line the GUI will open.
```
  If you are using Windows you can automate processing of several *.snp files like this:
```
 for %a in (*.s?p) DO s2spice /f /l /s %a
```
## ___THE NEXT PART IS FOR EXPERIENCED DEVELOPERS ONLY___

### Building for Linux
Release build on debian like (e.g. Ubuntu):
```
mkdir ~/myProjects
cd ~/myProjects
sudo apt update
sudo apt install libwxbase3.0-dev  ; or whatever is latest version of wxWidgets
git clone https://github.com/transmitterdan/s2spice
cd s2spice
git checkout main
mkdir build
cd build
cmake ..
cmake --build . --config Release
cd ../Test
../build/s2spice
```

### Building for Windows
Install 7zip: https://www.7-zip.org/download.html

Install MS Visual Studio 2022 Community edition: https://visualstudio.microsoft.com/vs/community/

Open a "x86 Native Tools Command Prompt for VS 2022" window and create a folder where you want to build s2spice.  In this example, we assume you have created a folder called "C:\myProjects". Then issue these commands
```
c:
cd \myProjects
git clone https://github.com/transmitterdan/s2spice
git checkout main
.\s2spice\setup_wxWidgets.bat
```
If all goes well you should see the s2spice window open.  There are 2 executables created, one in the s2spice\build\Release folder, and the other in s2spice\build\Debug folder.  The Debug version is slower but it will give more helpful messages if something goes wrong.  See setup_wxWidgets.bat for more details.
