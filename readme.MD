# c-deltaparser
This is a tool for decoding Growtopia's items.dat file. Compatible with every items.dat version, from v2 to v12.
This is a port of [cernodile/py-deltaparser](https://github.com/cernodile/py-deltaparser) to C, made with speed in mind.

## Usage
Get the latest release from the "Releases" section of this GitHub repository, extract it and place items.dat in the directory - then run the program.
You can run the program by just launching the executable or typing ".\c-deltaparser.c [output file name]". The default output file name is "itemsdat_parsed".

## Compiling it yourself
Should you have trouble trusting the executable in the "Releases" section, feel free to compile this program yourself. It's fully written in C99 (ANSI C)
and should be supported by every modern compiler. Tested on MSVC 19.27.29111.0, GCC 10.2 and Clang 11.0.0. Oh yeah, it also compiles for ARM. You can build this for Android.
There are two gcc build scripts (one for Windows systems, one for Linux systems) in the repository. Feel free to use these!

## Permissions
You're free to use this code for whatever you please - should you use it, though, credit would be appreciated.

## Footnote
Please note that this code is very target-oriented and the current expandability is low. An embeddable & expandable library is in the works.
Thanks to Cernodile for help in resolving bugs and expanding my knowledge of the items.dat format.
