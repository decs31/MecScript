# CMakeList.txt : CMake project for MecDecompile, include source and define
# project specific logic here.
#

set(PROJECT_NAME MecDecompile)

# Add source to this project's executable.
add_executable(${PROJECT_NAME}
        src/main.cpp
        ../Compiler/src/utils/Disassembler.cpp
)

include_directories(${PROJECT_NAME}
        ../../Device/CommonSrc/Utils
        ../Common/src
        ../Compiler/src/utils
        src
)

set_property(TARGET MecDecompile PROPERTY CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-old-style-cast")

# TODO: Add tests and install targets if needed.
