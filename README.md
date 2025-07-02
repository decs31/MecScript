# MecScript
A fast, heap-less, C style scripting language for memory constrained embedded devices.

> Inspired by and loosely based on the [Wren](https://wren.io/) scripting lanuguage and contents of the book [Crafting Interpreters](https://craftinginterpreters.com/).

MecScript is designed to be used in small embedded systems such as microcontrollers with very small amounts of memory. As such, heap allocation is never used and the languges is statically typed.

*MecScript is fully functional, but very much a work in progress...*

## Language Features
 - C style syntax.
 - Statically typed.
 - No heap useage.
 - All basic mathematical operations.
 - Bit-wise operations.
 - Loops (while, for).
 - Branches (if, else if, else).
 - Switch statements.
 - Arrays (size fixed at compile time).
 - Classes.
 - Class constructors & destructors.
 - Class methods.
 - Targetted to 32bit systems.
 - Integer and floating point variables. 
 - 16 bit address range supports compiled scripts up to 64KB.
 - Supports strings present at compile time, no string manipulation.
 - Easy to integrate into the wider system with "Native Functions".
 - Supports printing via Native Function calls.
 - Includes yield functions to allow Realtime Operating Systems such as FreeRTOS to switch tasks.

## Compiler Features
 - Command line interface.
 - Single pass compilation.
 - Compiles to tight byte code.
 - Includes a decompiler for reading the byte code.

## Virtual Machine Features
 - Simple class based implementation.
 - No external dependencies.
 - Stack size and location is controlled by the application writer.
 - All data, variables, and call frames are located on the preallocated stack.