SiMI - (Si)mple (MI)PS pipeline simulator
===

About
---
This is a very simple MIPS pipeline simulator.  The input file provides the system state (the register and memory values) as well as the assembly code to execute.  An 8-stage pipeline with forwarding and bypassing hardware is then simulated, with the instruction timing information printed to the output file, along with the final system state.

Compiling SiMI
---
SiMI is a single C++ source file written using C++11 and Boost.  Boost must be installed in the host system for the program to run.  The makefile provides two targets: default and debug.  The debug target provides symbol information that may be used by gdb for debugging purposes.  The default target builds the source as a standard executable.

Running an Input
---
The input file must contain the three headings REGISTERS, MEMORY, and CODE exactly as shown in the given order.  The supported operations are LD, SD, BNEZ, DADD (and DADDI), and SUB (and SUBI).  Two methods exist for running input files:

(1) Command-line arguments.  Calling $ simi.bin [input] [output] from the command-line will automatically run the simulator on the input file and write the result to the output file.

(2) Enter files at runtime.  If no arguments are provided, SiMI will ask for the location of the input and output files before running any simulations.  This is the only way to run multiple simulations in a single session.  Note that sessions are independent and do not persist.
