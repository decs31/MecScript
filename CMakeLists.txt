﻿# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)

project ("MecScript")

# Include sub-projects.
add_subdirectory("Common")
add_subdirectory("Compiler")
add_subdirectory("VirtualMachine")
add_subdirectory("Decompiler")
