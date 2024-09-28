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

#define STACK_SIZE 0x1000

static Value NativePrint(const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << (char *)args;

    return BOOL_VAL(true);
}

static Value NativePrintLine(const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << (char *)args << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintI(const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << std::to_string(args[0].Int) << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintF(const int argCount, Value *args) {

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

    std::ifstream scriptFile(inputFilePath, std::fstream::binary);

    if (!scriptFile.good()) {
        ERR("File does not exist or cannot be opened: \"" << inputFilePath << "\"");
        exit(ERROR_FILE_NOT_FOUND);
    }

    MSG("Reading input file: \"" << inputFilePath << "\"");
    std::vector<unsigned char> scriptData(std::istreambuf_iterator<char>(scriptFile), {});

    if (scriptData.empty()) {
        ERR("Program binary is empty.");
        exit(ERROR_INVALID_DATA);
    }

    MSG("Program size: " << scriptData.size() << " bytes.");

    // Give the VM a way to access native functions
    MecVm::SetNativeFunctionResolver(ResolveNativeFunction);

    // Declare a script and give it a stack
    ScriptInfo script{};
    u8 stack[STACK_SIZE];

    // Create a VM and decode the script code into the script struct
    MecVm vm;
    vm.DecodeScript(scriptData.data(), scriptData.size(), stack, STACK_SIZE, &script);
    MSG("Stack size after globals: " << (script.Stack.Count * sizeof(Value)) << " bytes.");

    // Run the script
    MSG("======== Script Start ========");
    vm.Run(&script);

    MSG("\n====== Script Finished =======");

    scriptFile.close();

    return 0;
}