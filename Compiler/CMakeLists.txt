# CMakeList.txt : CMake project for ScriptUtils, include source and define
# project specific logic here.
#

set(PROJECT_NAME MecCompile)

# Add source to this project's executable.
add_executable(${PROJECT_NAME}
        ../Common/src/OrderedMap.hpp
        ../Common/src/Value.cpp
        ../Common/src/Checksum.cpp        
        src/main.cpp
        src/error/ErrorHandler.cpp
        src/lexer/Lexer.cpp
        src/preprocessor/PreProcessor.cpp
        src/utils/ScriptUtils.cpp
        src/utils/MecScriptBase.cpp
        src/utils/Disassembler.cpp
        src/compiler/CompilerBase.cpp
        src/compiler/Compiler.cpp
        src/compiler/Rules.cpp
        src/compiler/TypeSystem.cpp
        src/compiler/Function.cpp
        src/compiler/Variable.cpp
        src/compiler/Native.cpp
)

include_directories(${PROJECT_NAME}
        ../Common/src
        src/utils
        src/error
        src/lexer
        src/compiler
)

set_property(TARGET MecCompile PROPERTY CXX_STANDARD 20)
set_property(TARGET MecCompile PROPERTY CXX_STANDARD_REQUIRED ON)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-old-style-cast")

# TODO: Add tests and install targets if needed.
