@echo off
set configuration=Release
echo Build: MecScript Compiler - %configuration%
cd ..
rmdir build-compiler /s /q
mkdir build-compiler
cd build-compiler
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=%configuration% -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_CXX_STANDARD=20
cmake --build . --target MecCompile
cmake --install ./Compiler --config %configuration%
pause