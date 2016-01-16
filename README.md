# COP4610 - Project 2 - Kernel Module
## Due Date: 11-02-2015 11:59:59pm

##### Group Members

* Evan Lee (El12b)
* John Cyr(Jrc11v)
* Abraheem Omari(Afo12)

##### Tar Archive / Files

PART 1:

null.c - This is an empty C program used to compare the number of syscalls to our program.

syscall.c - Our simple C program that should call exactly 10 syscalls.

PART 2:

my_xtime.c - The my_xtime kernel module source code. This will output the number of seconds
since the Unix Epoch. Upon subsequent calls, this module will output the number of seconds since the last call.

PART 3:

elevator.c - This is the main source file of the elevator kernel module. This holds all the methods for procfs interaction, module install/uninstall, and interaction with user-space programs.

obj.h - This holds the main data structure and variable declaractions of the elevator structure. 

##### Server

Linprog, compiled with gcc.

##### Makefile Commands

For all three parts, the 'make' command will compile all necessary components.
The 'clean' command will clean the directory of generated files.

##### Known Bugs / Incomplete

None
