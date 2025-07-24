//
// Created by Declan Walsh on 18/02/2024.
//

#include <iostream>
#include <winerror.h>
#include <fstream>
#include <chrono>
#include <cstring>
#include <thread>

#include "Options.h"
#include "VmConfig.h"
#include "MecVm.h"
#include "Console.h"

#define STACK_SIZE 0x1000

static Value NativePrint(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    const char *chars = MecVm::ResolveString(script, AS_UINT32(args[0]));
    std::cout << chars;

    return BOOL_VAL(true);
}

static Value NativePrintLine(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    const char *chars = MecVm::ResolveString(script, AS_UINT32(args[0]));
    std::cout << chars << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintI(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << std::to_string(args[0].Int) << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintF(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {

    if (argCount < 1) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    std::cout << std::to_string(args[0].Float) << std::endl;

    return BOOL_VAL(true);
}

static Value NativePrintFormat(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {

    if (argCount < 2) {
        MSG("Print Error! Nothing to print.");
        return BOOL_VAL(false);
    }

    const char *chars = MecVm::ResolveString(script, AS_UINT32(args[0]));
    float val = AS_FLOAT(args[1]);
    char strBuf[256 + 32];
    if (strlen(chars) > 256) {
        MSG("Print Error! String too long.");
        return BOOL_VAL(false);
    }
    sprintf(strBuf, chars, val);
    std::cout << strBuf << std::endl;

    return BOOL_VAL(true);
}

using Clock = std::chrono::steady_clock;
using nanos = std::chrono::nanoseconds;
using millis = std::chrono::milliseconds;
template<class Duration>
using TimePoint = std::chrono::time_point<Clock, Duration>;
static nanos ClockStartTime;

static long long int Millis() {
    const auto now = Clock::now().time_since_epoch() - ClockStartTime;
    const long long int ms = now.count() / 1000000;
    return ms;
}

static Value NativeClock(const ScriptInfo *const script, void *sysParam, int argCount, Value *args) {
    const auto ms = Millis();
    return INT32_VAL((int) ms);
}

static Value NativeYield(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {
    if (argCount < 1) {
        MSG("Yield Error! No time given.");
    }

    long long int delay = AS_UINT32(args[0]);

    MSG("Yield(" << delay << ")");

    auto start = Millis();
    while (Millis() < (start + delay)) {
        // spin...
    }

    return BOOL_VAL(true);
}

static Value NativeYieldUntil(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {
    if (argCount < 2) {
        MSG("Yield Error! No time given.");
    }

    long long int lastTime = AS_UINT32(args[0]);
    long long int delay = AS_UINT32(args[1]);

    MSG("Yield(" << delay << ")");

    auto end = lastTime + delay;
    while (Millis() < end) {
        // spin...
    }
    lastTime += delay;

    return UINT32_VAL(lastTime);
}

static Value NativeDummy(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args) {
    MSG("Native Function not defined");
    return BOOL_VAL(false);
}

static NativeFunc ResolveNativeFunction(const NativeFuncId funcId, const u8 argCount) {

    NativeFunc func = nullptr;

    switch (funcId) {
        case nfPrint:
            func = NativePrint;
            break;

        case nfPrintLine:
            func = NativePrintLine;
            break;

        case nfPrintInt:
            func = NativePrintI;
            break;

        case nfPrintFloat:
            func = NativePrintF;
            break;

        case nfPrintFormat:
            func = NativePrintFormat;
            break;

        case nfClock:
            func = NativeClock;
            break;

        case nfYieldFor:
            func = NativeYield;
            break;

        case nfYieldUntil:
            func = NativeYieldUntil;
            break;

        default:
            func = NativeDummy;
            break;
    }

    return func;
}


int main(int argc, char *argv[]) {

    ClockStartTime = Clock::now().time_since_epoch();

    std::string inputFilePath;

    // Read args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        // Verbose
        if (arg == "-v") {
            MSG("Verbose Output = On");
            Console::VerboseOutput = true;
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

    MSG_V("====== MecScript Virtual Machine ======");

    std::ifstream scriptFile(inputFilePath, std::fstream::binary);

    if (!scriptFile.good()) {
        ERR("File does not exist or cannot be opened: \"" << inputFilePath << "\"");
        exit(ERROR_FILE_NOT_FOUND);
    }

    MSG_V("Reading input file: \"" << inputFilePath << "\"");
    std::vector<unsigned char> scriptData(std::istreambuf_iterator<char>(scriptFile), {});

    if (scriptData.empty()) {
        ERR("Program binary is empty.");
        exit(ERROR_INVALID_DATA);
    }

    MSG_V("Program size: " << scriptData.size() << " bytes.");

    // Give the VM a way to access native functions
    MecVm::SetNativeFunctionResolver(ResolveNativeFunction);

    // Declare a script and give it a stack
    ScriptInfo script{};
    u8 stack[STACK_SIZE];

    // Create a VM and decode the script code into the script struct
    MecVm vm;
    MecVm::DecodeScript(scriptData.data(), scriptData.size(), stack, STACK_SIZE, &script);
    MSG_V("Stack size after globals: " << (script.Stack.Count * sizeof(Value)) << " bytes.");

    // Run the script
    MSG_V("======== Script Start ========");

    vm.Run(&script);

    MSG_V("\n====== Script Finished =======");

    scriptFile.close();

    return 0;
}