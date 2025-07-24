@echo off
set configuration=Release
echo Build: MecDecompile - %configuration%
cd ..
rmdir build-decompiler /s /q
mkdir build-decompiler
cd build-decompiler
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=%configuration% -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_CXX_STANDARD=20
cmake --build . --target MecDecompile
cmake --install ./Decompiler --config %configuration%
pause