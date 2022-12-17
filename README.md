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
