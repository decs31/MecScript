# CMakeList.txt : CMake project for ScriptUtils, include source and define
# project specific logic here.
#

set(PROJECT_NAME MecCompile)

# Add source to this project's executable.
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/error/ErrorHandler.cpp
        src/lexer/Lexer.cpp
        src/preprocessor/PreProcessor.cpp
        ../Common/src/OrderedMap.hpp
        ../Common/src/Value.cpp
        ../Common/src/Checksum.cpp
        src/utils/ScriptUtils.cpp
        src/utils/MecScriptBase.cpp
        src/utils/Disassembler.cpp
        src/compiler/Compiler.cpp
        ../VirtualMachine/src/debugger/Debugger.cpp
        src/compiler/Rules.cpp
        src/compiler/TypeSystem.cpp
        src/compiler/Function.cpp
        src/compiler/Variable.cpp
)

include_directories(${PROJECT_NAME}
        ../../Device/CommonSrc/Utils
        ../Common/src
        src/utils
        src/error
        src/lexer
        src/compiler
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET MecCompile PROPERTY CXX_STANDARD 20)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")


# TODO: Add tests and install targets if needed.
