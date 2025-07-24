@echo off
set configuration=Release
echo Build: MecVM - %configuration%
cd ..
rmdir build-vm /s /q
mkdir build-vm
cd build-vm
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=%configuration% -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_CXX_STANDARD=20
cmake --build . --target MecVM
cmake --install ./VirtualMachine --config %configuration%
pause