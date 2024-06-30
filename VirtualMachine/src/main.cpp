//
// Created by Declan Walsh on 18/02/2024.
//

#include <iostream>
#include <winerror.h>
#include <fstream>
#include <chrono>
#include "Options.h"
#include "VmConfig.h"
#include "MecVm.h"
#include "Console.h"


static Value NativePrint(int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << (char *)args;

    return BOOL_VAL(true);
}

static Value NativePrintLine(int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << (char *)args << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintI(int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << std::to_string(args[0].Int) << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintF(int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << std::to_string(args[0].Float) << std::endl;

    return BOOL_VAL(true);
}

using Clock = std::chrono::steady_clock;
using nanos = std::chrono::nanoseconds;
using millis = std::chrono::milliseconds;
template<class Duration>
using TimePoint = std::chrono::time_point<Clock, Duration>;
static nanos ClockStartTime;

static Value NativeClock(int argCount, Value *args) {

    const auto now = Clock::now().time_since_epoch() - ClockStartTime;
    long long int ms = now.count() / 1000000;
    return INT32_VAL((int) ms);
}

static NativeFunc ResolveNativeFunction(const NativeFuncId funcId, const u8 argCount) {

    NativeFunc func = nullptr;
    u8 funcArgs = 0;

    switch (funcId) {
        case nfPrint:
            func = NativePrint;
            funcArgs = 1;
            break;

        case nfPrintLn:
            func = NativePrintLine;
            funcArgs = 1;
            break;

        case nfPrintI:
            func = NativePrintI;
            funcArgs = 1;
            break;

        case nfPrintF:
            func = NativePrintF;
            funcArgs = 1;
            break;

        case nfClock:
            func = NativeClock;
            funcArgs = 0;
            break;

        default:
            break;
    }

    if (argCount != funcArgs)
        func = nullptr;

    return func;
}


int main(int argc, char *argv[]) {

    ClockStartTime = Clock::now().time_since_epoch();

    std::string inputFilePath;

    // Read args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        // Verbose
        if (arg.substr(0, 2) == "-v") {
            MSG("Verbose Output = On");
        }
            // Input path
        else if (inputFilePath.empty()) {
            inputFilePath = arg;
        }
    }

    if (inputFilePath.empty()) {
        ERR("Incorrect usage!");
        ERR("Correct usage is: " << VIRTUAL_MACHINE_NAME << " <file." << OUTPUT_EXTENSION << ">");
        exit(ERROR_INVALID_FUNCTION);
    }

    std::cout << "<<<< MecScript Virtual Machine >>>>" << std::endl;

    std::ifstream pgmFile(inputFilePath, std::fstream::binary);

    if (!pgmFile.good()) {
        ERR("File does not exist or cannot be opened: \"" << inputFilePath << "\"");
        exit(ERROR_FILE_NOT_FOUND);
    }

    MSG("Reading input file: \"" << inputFilePath << "\"");
    std::vector<unsigned char> pgmData(std::istreambuf_iterator<char>(pgmFile), {});

    if (pgmData.empty()) {
        ERR("Program binary is empty.");
        exit(ERROR_INVALID_DATA);
    }

    MSG("Program size: " << pgmData.size() << " bytes.");

    // Give the VM a way to access native functions
    MecVm::SetNativeFunctionResolver(ResolveNativeFunction);

    // Declare a program and give it a stack
    ProgramInfo prog{};
    u8 stack[1024];

    // Create a VM and decode the program code into the program struct
    MecVm vm;
    vm.DecodeProgram(pgmData.data(), pgmData.size(), stack, 1024, &prog);
    MSG("Stack size after globals: " << (prog.Stack.Count * sizeof(Value)) << " bytes.");

    // Run the program
    MSG("======== Program Start ========");
    vm.Run(&prog);

    MSG("\n====== Program Finished =======");

    pgmFile.close();

    return 0;
}