//
// Created by Declan Walsh on 18/02/2024.
//

#ifndef MECVM_H
#define MECVM_H

#include "Instructions.h"
#include "NativeFunctions.h"
#include "ScriptInfo.h"
#include "Value.h"

/* MUST BE INCLUDED BY APPLICATION */
#include "VmConfig.h"
/* =============================== */

#define VIRTUAL_MACHINE_NAME "MecVm"

enum VmStatus {
    vmOk = 0,
    vmStop,
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
class MecVm
{
  public:
    MecVm();
    ~MecVm();

    static u32 DecodeScript(u8 *data, const u32 dataSize, u8 *stack, const u32 stackSize, ScriptInfo *script);

    void Run(ScriptInfo *script, void *sysParam = nullptr);
    void Stop();

    void Reset();

    VmStatus GetStatus();

    static void SetNativeFunctionResolver(ResolverFunction resolver);

    static void GetLanguageVersion(u8 &major, u8 &minor);

    static const char *ResolveString(const ScriptInfo *const script, const u32 index);

  private:
    volatile VmStatus m_Status = vmOk;

    struct CallFrame {
        CallFrame *Enclosing;
        opCode_t *Ip;
        Value *Slots;
    };

    void *m_SystemParameter = nullptr;
    Value *m_StackPtr       = nullptr;
    Value *m_StackEnd       = nullptr;

    ScriptInfo *m_Script = nullptr;

    CallFrame m_Frame;

    void Push(const Value &data);
    void PushN(u32 num);
    Value Pop();
    Value PopN(u32 num);
    Value Peek(u32 pos = 1);
    void Duplicate(u32 count = 1);

    Value *ResolvePointer(const VmPointer &pointer);
    void IncrementValue(const VmPointer &pointer, bool push);
    void DecrementValue(const VmPointer &pointer, bool push);
    bool Call(funcPtr_t functionId, int argCount);
    bool CallNative(NativeFuncId nativeId, int argCount);

    static ResolverFunction FunctionResolver;
    NativeFunc ResolveNativeFunction(NativeFuncId funcId, u8 argCount);

    VmStatus SetStatus(VmStatus status);
};

#endif // MECVM_H
