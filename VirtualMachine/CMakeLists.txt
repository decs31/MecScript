# CMakeList.txt : CMake project for MecVm, include source and define
# project specific logic here.
#

set(PROJECT_NAME MecScriptVirtualMachine)

# Add source to this project's executable.
add_executable(${PROJECT_NAME}
        src/main.cpp
        ../Common/src/Value.cpp
        ../Common/src/Checksum.cpp
        src/vm/MecVm.cpp
        src/debugger/Debugger.cpp
)

include_directories(${PROJECT_NAME}
        ../../Device/CommonSrc/Utils
        ../Common/src
        ../Compiler/src/utils
        src
        src/vm
        src/debugger
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET MecScriptVirtualMachine PROPERTY CXX_STANDARD 20)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")


# TODO: Add tests and install targets if needed.
