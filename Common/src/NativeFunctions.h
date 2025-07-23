//
// Created by Declan Walsh on 11/04/2024.
//

#ifndef NATIVEFUNCTIONS_H
#define NATIVEFUNCTIONS_H

#include "BasicTypes.h"
#include "ScriptInfo.h"
#include "Value.h"


enum NativeFuncId : u8 {
    nfNull = 0,
    nfPrint,
    nfPrintLine,
    nfPrintInt,
    nfPrintFloat,
    nfPrintFormat,
    nfClock,
    nfYieldFor,
    nfYieldUntil,
    nfReadRuntime,
    nfReadRuntimeReal,
    nfWriteRuntime,
    nfWriteRuntimeReal,
    nfReadVariable,
    nfWriteVariable,
    nfLookupTable,
    nfSendCanMessage,
    nfReadCanMessage,
};

/* Native Functions */
typedef Value (*NativeFunc)(const ScriptInfo *const script, void *sysParam, const int argCount, Value *args);
typedef NativeFunc (*ResolverFunction)(const NativeFuncId funcId, const u8 argCount);

#endif // NATIVEFUNCTIONS_H
