//
// Created by Declan Walsh on 18/02/2024.
//

#ifndef MECVM_H
#define MECVM_H

#include "Value.h"
#include "Instructions.h"
#include "NativeFunctions.h"
#include "ScriptInfo.h"

/* MUST BE INCLUDED BY APPLICATION */
#include "VmConfig.h"
/* =============================== */

#define VIRTUAL_MACHINE_NAME        "MecVm"

#define VM_SIZE_BYTES               (1024 * 8)
#define VM_FRAMES_SIZE              (64)
#define VM_FRAMES_SIZE_BYTES        (VM_FRAMES_SIZE * sizeof (CallFrame))
#define VM_VALUE_SIZE               ((VM_SIZE_BYTES - VM_FRAMES_SIZE_BYTES) / sizeof(Value))


enum VmStatus {
    vmOk = 0,
    vmEnd,

    // Errors
    vmError,
    vmNoProgramLoaded,
    vmUnknownInstruction,
    vmStackUnderflow,
    vmStackOverflow,
    vmUnknownFieldScope,
    vmCallArgCountError,
    vmCallNotAFunction,
    vmCalledNonCallable,
    vmCallFrameOverflow,
    vmNativeFunctionNotResolved,
};

/* Virtual Machine */
class MecVm {
public:
    MecVm();
    ~MecVm();

    static bool DecodeScript(u8 *data, u32 dataSize, u8 *stack, u32 stackSize, ScriptInfo *program);

    void Run(ScriptInfo *program);

    void Reset();

    static void SetNativeFunctionResolver(ResolverFunction resolver);

private:
    VmStatus m_Status = vmOk;

    struct CallFrame {
        CallFrame *Enclosing;
        opCode_t *Ip;
        Value *Slots;
    };

    Value *m_StackPtr = nullptr;
    Value *m_StackEnd = nullptr;

    ScriptInfo *m_Program = nullptr;

    CallFrame m_Frame;

    void Push(const Value &data);
    void PushN(u32 num);
    Value Pop();
    Value PopN(u32 num);
    Value Peek(u32 pos = 1);
    void Duplicate(u32 count = 1);

    Value *FindVariable(const VmPointer &pointer);
    void IncrementValue(const VmPointer &pointer, bool push);
    void DecrementValue(const VmPointer &pointer, bool push);
    bool Call(funcPtr_t functionId, int argCount);
    bool CallNative(NativeFuncId nativeId, int argCount);
    Value *String(const u32 index);

    static ResolverFunction FunctionResolver;
    NativeFunc ResolveNativeFunction(NativeFuncId funcId, u8 argCount);

    VmStatus SetStatus(VmStatus status);
};


#endif //MECVM_H
