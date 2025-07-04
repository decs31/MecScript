@echo off
set configuration=Release
echo Build: MecScript Compiler - %configuration%
cd ..
rmdir build-compiler /s /q
mkdir build-compiler
cd build-compiler
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=%configuration%
cmake --build . --target MecCompile
cmake --install ./Compiler --config %configuration%
pause