//
// Created by Declan Walsh on 18/02/2024.
//

#include "MecVm.h"

#include <algorithm>

#include "Checksum.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "Debugger.h"
#include "Checksum.h"
#include <iostream>
#define DISASSEMBLE_INSTRUCTION(start, code)    Debugger::DebugInstruction(start, code)
#else
#define DISASSEMBLE_INSTRUCTION(start, code)
#define MSG(msg)
#define ERR(msg)
#define PRINT(msg)
#endif

#define CONSTANTS_START_PTR             m_Script->Constants.Values
#define STRINGS_START_PTR               m_Script->Strings.Values
#define STACK_GLOBALS_START_PTR         m_Script->Globals.Values
#define STACK_LOCALS_START_PTR          m_Script->Stack.Values
#define STACK_POSITION                  (m_StackPtr - STACK_LOCALS_START_PTR)
#define STACK_POS_AT(ptr)               (ptr - STACK_LOCALS_START_PTR)
#define STACK_END_PTR                   (m_Script->Stack.Values + m_Script->Stack.Count)

#define PGM_CODE                        m_Script->Code.Data
#define PGM_CONSTANTS                   CODE_CONSTANTS_START_PTR
#define PGM_STRINGS                     STRINGS_START_PTR
#define PGM_GLOBALS                     STACK_GLOBALS_START_PTR
#define FRAME_LOCALS                    m_Frame.Slots
#define THIS_ADDRESS                    FRAME_LOCALS[0]
#define STACK_ADDRESS_OF(ptr)           (u32)(ResolvePointer(ptr) - PGM_GLOBALS);

#define FRAME_SIZE                      ((sizeof(CallFrame) / sizeof(Value)) + (sizeof(CallFrame) % sizeof(Value) > 0 ? 1 : 0))

/* Instruction Readers */
#define READ_BYTE()             (*m_Frame.Ip++)
#define READ_UINT16()           (m_Frame.Ip += 2, (uint16_t)(m_Frame.Ip[-2] | (m_Frame.Ip[-1] << 8)))
#define READ_UINT24()           (m_Frame.Ip += 3, (uint32_t)(m_Frame.Ip[-3] | (m_Frame.Ip[-2] << 8) | (m_Frame.Ip[-1] << 16)))
#define READ_INT32()            (m_Frame.Ip += 4, (int32_t)(m_Frame.Ip[-4] | (m_Frame.Ip[-3] << 8) | (m_Frame.Ip[-2] << 16) | (m_Frame.Ip[-1] << 24)))

/* Instruction Macros */
#define OP_BINARY(op, resultType, valueType) \
do { \
    Value rhs = Pop(); \
    Value lhs = Pop(); \
    Push(resultType(valueType(lhs) op valueType(rhs))); \
} while(false)


// Float 0 is the same as Int 0, so we only need to check the Int value.
#define IS_FALSEY(value)              (AS_INT32(value) == 0)

ResolverFunction MecVm::FunctionResolver = nullptr;

MecVm::MecVm() {
}

MecVm::~MecVm() {
}

void MecVm::GetLanguageVersion(u8 &major, u8 &minor) {

    major = LANG_VERSION_MAJOR;
    minor = LANG_VERSION_MINOR;
}

VmStatus MecVm::GetStatus() {

    return m_Status;
}


void MecVm::Run(ScriptInfo *script, void *sysParam) {

    m_Script = script;
    m_SystemParameter = sysParam;

    if (m_Script == nullptr || m_Script->Code.Length == 0) {
        SetStatus(vmNoProgramLoaded);
        return;
    }

    m_Status = vmOk;

    Reset();

    for (;;) {
        DISASSEMBLE_INSTRUCTION(m_Script->Code.Data, m_Frame.Ip);
        opCode_t instruction = READ_BYTE();

        switch (instruction) {
            case OP_NOP: {
                // No operation
                break;
            }

            case OP_PUSH: {
                Push({INT32_VAL(0)});
                break;
            }

            case OP_PUSH_N: {
                PushN(READ_BYTE());
                break;
            }

            case OP_POP: {
                Pop();
                break;
            }

            case OP_POP_N: {
                PopN(READ_BYTE());
                break;
            }

            case OP_DUPLICATE: {
                Push(Peek());
                break;
            }
            case OP_DUPLICATE_2: {
                Push(Peek(2));
                Push(Peek(2));
                break;
            }

            case OP_NIL: {
                Push(INT32_VAL(0));
                break;
            }

            case OP_FALSE: {
                Push(BOOL_VAL(false));
                break;
            }

            case OP_TRUE: {
                Push(BOOL_VAL(true));
                break;
            }

            case OP_CONSTANT: {
                u32 address = READ_BYTE();
                Push(m_Script->Constants.Values[address]);
                break;
            }

            case OP_CONSTANT_16: {
                u32 address = READ_UINT16();
                Push(m_Script->Constants.Values[address]);
                break;
            }

            case OP_CONSTANT_24: {
                u32 address = READ_UINT24();
                Push(m_Script->Constants.Values[address]);
                break;
            }

            case OP_STRING: {
                u32 address = READ_BYTE();
                Push({.UInt = address});
                break;
            }

            case OP_STRING_16: {
                u32 address = READ_UINT16();
                Push({.UInt = address});
                break;
            }

            case OP_STRING_24: {
                u32 address = READ_UINT24();
                Push({.UInt = address});
                break;
            }

            case OP_ARRAY: {
                int size = READ_UINT16();
                PushN(size);
                break;
            }

            case OP_GET_INDEXED_S8: {
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 2);
                Value data = INT32_VAL(ResolvePointer(ptr)->Chars[i & 0x03]);
                Push(data);
                break;
            }

            case OP_GET_INDEXED_U8: {
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 2);
                Value data = INT32_VAL(ResolvePointer(ptr)->Bytes[i & 0x03]);
                Push(data);
                break;
            }

            case OP_GET_INDEXED_S16: {
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 1);
                Value data = INT32_VAL(ResolvePointer(ptr)->Shorts[i & 0x01]);
                Push(data);
                break;
            }

            case OP_GET_INDEXED_U16: {
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 1);
                Value data = INT32_VAL(ResolvePointer(ptr)->UShorts[i & 0x01]);
                Push(data);
                break;
            }


            case OP_GET_INDEXED_S32:
            case OP_GET_INDEXED_U32:
            case OP_GET_INDEXED_FLOAT: {
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += i;
                Value data = *ResolvePointer(ptr);
                Push(data);
                break;
            }

            case OP_SET_INDEXED_S8: {
                Value value = Pop();
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 2);
                ResolvePointer(ptr)->Chars[i & 0x03] = AS_INT8(value);
                Push(value);
                break;
            }

            case OP_SET_INDEXED_U8: {
                Value value = Pop();
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 2);
                ResolvePointer(ptr)->Bytes[i & 0x03] = AS_UINT8(value);
                Push(value);
                break;
            }

            case OP_SET_INDEXED_S16: {
                Value value = Pop();
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 1);
                ResolvePointer(ptr)->Shorts[i & 0x01] = AS_INT16(value);
                Push(value);
                break;
            }

            case OP_SET_INDEXED_U16: {
                Value value = Pop();
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += (i >> 1);
                ResolvePointer(ptr)->UShorts[i & 0x01] = AS_UINT16(value);
                Push(value);
                break;
            }

            case OP_SET_INDEXED_S32:
            case OP_SET_INDEXED_U32:
            case OP_SET_INDEXED_FLOAT: {
                Value value = Pop();
                int i = AS_INT32(Pop());
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address += i;
                *ResolvePointer(ptr) = value;
                Push(value);
                break;
            }


                /* Variables */
            case OP_GET_VARIABLE: {
                VmPointer ptr = AS_POINTER(Pop());
                Push(*ResolvePointer(ptr));
                break;
            }

            case OP_ABSOLUTE_POINTER: {
                VmPointer ptr = AS_POINTER(Pop());
                ptr.Address = STACK_ADDRESS_OF(ptr);
                ptr.Scope = scopeStackAbsolute;
                Push(POINTER_VAL(ptr));
                break;
            }

                // Cast
            case OP_CAST_INT_TO_FLOAT: {
                Push(FLOAT_VAL((float) AS_INT32(Pop())));
                break;
            }
            case OP_CAST_PREV_INT_TO_FLOAT: {
                Value *prev = (m_StackPtr - 2);
                prev->Float = (float) prev->Int;
                break;
            }
            case OP_CAST_FLOAT_TO_INT: {
                Push(INT32_VAL((int) AS_FLOAT(Pop())));
                break;
            }
            case OP_CAST_PREV_FLOAT_TO_INT: {
                Value *prev = (m_StackPtr - 2);
                prev->Int = (int) prev->Float;
                break;
            }

                // Unary
            case OP_NEGATE_I: {
                Value value = Pop();
                Push(INT32_VAL(-(value.Int)));
                break;
            }
            case OP_NEGATE_F: {
                Value value = Pop();
                Push(FLOAT_VAL(-(value.Float)));
                break;
            }
            case OP_BIT_NOT: {
                Push(INT32_VAL(~AS_INT32(Pop())));
                break;
            }

            case OP_PREFIX_DECREASE: {
                VmPointer ptr = AS_POINTER(Pop());
                DecrementValue(ptr, true);
                break;
            }
            case OP_PREFIX_INCREASE: {
                VmPointer ptr = AS_POINTER(Pop());
                IncrementValue(ptr, true);
                break;
            }
            case OP_MINUS_MINUS: {
                VmPointer ptr = AS_POINTER(Pop());
                DecrementValue(ptr, false);
                break;
            }
            case OP_PLUS_PLUS: {
                VmPointer ptr = AS_POINTER(Pop());
                IncrementValue(ptr, false);
                break;
            }

                // Binary
            case OP_ADD_S: {
                OP_BINARY(+, INT32_VAL, AS_INT32);
                break;
            }
            case OP_ADD_U: {
                OP_BINARY(+, UINT32_VAL, AS_UINT32);
                break;
            }
            case OP_ADD_F: {
                OP_BINARY(+, FLOAT_VAL, AS_FLOAT);
                break;
            }
            case OP_SUB_S: {
                OP_BINARY(-, INT32_VAL, AS_INT32);
                break;
            }
            case OP_SUB_U: {
                OP_BINARY(-, UINT32_VAL, AS_UINT32);
                break;
            }
            case OP_SUB_F: {
                OP_BINARY(-, FLOAT_VAL, AS_FLOAT);
                break;
            }
            case OP_MULT_S: {
                OP_BINARY(*, INT32_VAL, AS_INT32);
                break;
            }
            case OP_MULT_F: {
                OP_BINARY(*, FLOAT_VAL, AS_FLOAT);
                break;
            }
            case OP_DIV_S: {
                OP_BINARY(/, INT32_VAL, AS_INT32);
                break;
            }
            case OP_DIV_F: {
                OP_BINARY(/, FLOAT_VAL, AS_FLOAT);
                break;
            }
            case OP_MODULUS: {
                // Must always be done as integers
                OP_BINARY(%, INT32_VAL, AS_INT32);
                break;
            }

            case OP_ASSIGN: {
                VmPointer ptr = AS_POINTER(Pop());
                Value operand = Peek();
                *ResolvePointer(ptr) = operand;
                break;
            }

                // Bitwise
            case OP_BIT_AND: {
                OP_BINARY(&, INT32_VAL, AS_INT32);
                break;
            }
            case OP_BIT_OR: {
                OP_BINARY(|, INT32_VAL, AS_INT32);
                break;
            }
            case OP_BIT_XOR: {
                OP_BINARY(^, INT32_VAL, AS_INT32);
                break;
            }
            case OP_BIT_SHIFT_L: {
                OP_BINARY(<<, INT32_VAL, AS_INT32);
                break;
            }
            case OP_BIT_SHIFT_R: {
                OP_BINARY(>>, INT32_VAL, AS_INT32);
                break;
            }

                // Logic
            case OP_NOT: {
                Push(BOOL_VAL(IS_FALSEY(Pop())));
                break;
            }

            case OP_EQUAL_S: {
                OP_BINARY(==, BOOL_VAL, AS_INT32);
                break;
            }
            case OP_EQUAL_U: {
                OP_BINARY(==, BOOL_VAL, AS_UINT32);
                break;
            }
            case OP_EQUAL_F: {
                OP_BINARY(==, BOOL_VAL, AS_FLOAT);
                break;
            }

            case OP_NOT_EQUAL_S: {
                OP_BINARY(!=, BOOL_VAL, AS_INT32);
                break;
            }
            case OP_NOT_EQUAL_F: {
                OP_BINARY(!=, BOOL_VAL, AS_FLOAT);
                break;
            }

            case OP_LESS_S: {
                OP_BINARY(<, BOOL_VAL, AS_INT32);
                break;
            }
            case OP_LESS_U: {
                OP_BINARY(<, BOOL_VAL, AS_UINT32);
                break;
            }
            case OP_LESS_F: {
                OP_BINARY(<, BOOL_VAL, AS_FLOAT);
                break;
            }

            case OP_LESS_OR_EQUAL_S: {
                OP_BINARY(<=, BOOL_VAL, AS_INT32);
                break;
            }
            case OP_LESS_OR_EQUAL_U: {
                OP_BINARY(<=, BOOL_VAL, AS_UINT32);
                break;
            }
            case OP_LESS_OR_EQUAL_F: {
                OP_BINARY(<=, BOOL_VAL, AS_FLOAT);
                break;
            }

            case OP_GREATER_S: {
                OP_BINARY(>, BOOL_VAL, AS_INT32);
                break;
            }
            case OP_GREATER_U: {
                OP_BINARY(>, BOOL_VAL, AS_UINT32);
                break;
            }
            case OP_GREATER_F: {
                OP_BINARY(>, BOOL_VAL, AS_FLOAT);
                break;
            }

            case OP_GREATER_OR_EQUAL_S: {
                OP_BINARY(>=, BOOL_VAL, AS_INT32);
                break;
            }
            case OP_GREATER_OR_EQUAL_U: {
                OP_BINARY(>=, BOOL_VAL, AS_UINT32);
                break;
            }
            case OP_GREATER_OR_EQUAL_F: {
                OP_BINARY(>=, BOOL_VAL, AS_FLOAT);
                break;
            }

            case OP_JUMP:
            case OP_BREAK: {
                u16 offset = READ_UINT16();
                m_Frame.Ip += offset;
                break;
            }

            case OP_JUMP_IF_FALSE: {
                u16 offset = READ_UINT16();
                if (IS_FALSEY(Peek())) {
                    m_Frame.Ip += offset;
                }
                break;
            }

            case OP_JUMP_IF_TRUE: {
                u16 offset = READ_UINT16();
                if (!IS_FALSEY(Peek())) {
                    m_Frame.Ip += offset;
                }
                break;
            }

            case OP_JUMP_IF_EQUAL: {
                u16 offset = READ_UINT16();
                // Type doesn't matter. Compare the bits.
                if (AS_INT32(Pop()) == AS_INT32(Pop())) {
                    m_Frame.Ip += offset;
                }
                break;
            }

            case OP_CONTINUE:
            case OP_LOOP: {
                u16 offset = READ_UINT16();
                m_Frame.Ip -= offset;
                break;
            }
            
            case OP_SWITCH: {
                u16 tableEndOffset = READ_UINT16() - 8; // Skip the 2 ints to follow.
                int min = READ_INT32();
                int max = READ_INT32();
                int value = AS_INT32(Pop());
                int index;
                /* JUMP TABLE
                 * [default]        << index = range + 2
                 * [case min]       << index = max + 1
                 * [case ...]
                 * [case max]       << index = min + 1
                 * [JumpTableEnd]   << Jump lands here with 0 index (skips table entirely)
                 */
                if ((value >= min) && (value <= max)) {
                    // Value is within the jump table range.
                    // Calculate the index from the end of the table BACK to the index.
                    index = ((max - min) - (value - min)) + 1;
                } else {
                    // Value is outside the jump table, get the default jump at the top of the table.
                    index = (max - min) + 2;
                }

                // Jump to the case label
                m_Frame.Ip += (tableEndOffset - (index * 2)); // 16 bit addresses
                u16 caseJump = READ_UINT16();
                // Case jumps are stored as offsets. Jump is backwards.
                m_Frame.Ip -= (caseJump + 2);

                break;
            }

            case OP_FRAME: {
                // Push the stack to accommodate a call frame.
                CallFrame *frame = (CallFrame *)m_StackPtr;
                constexpr int frameSize = FRAME_SIZE;
                m_StackPtr += frameSize;
                // Store the current frame.
                *frame = m_Frame;
                m_Frame.Enclosing = frame;
                break;
            }

            case OP_CALL: {
                const int argCount = READ_BYTE();
                Value func = Peek(argCount + 1);
                if (!Call(AS_FUNCTION(func), argCount)) {
                    // A call error occurred.
                    return;
                }
                break;
            }

            case OP_CALL_NATIVE: {
                const int argCount = READ_BYTE();
                Value func = Peek(argCount + 1);
                NativeFuncId nativeId = (NativeFuncId) AS_NATIVE(func);
                if (!CallNative(nativeId, argCount)) {
                    // A call error occurred.
                    return;
                }
                break;
            }

            case OP_RETURN: {
                // Pop the return value off the stack
                Value result = Pop();

                // Rewind the frame. -1 because the function itself was before the first arg.
                m_StackPtr = m_Frame.Slots - 1 - FRAME_SIZE;

                // Roll back the stack frame
                m_Frame = *m_Frame.Enclosing;

                Push(result);
                break;
            }

            case OP_END: {
                SetStatus(vmEnd);
                return;
            }

            default: {
                SetStatus(vmUnknownInstruction);
                return;
            }
        }
    }
}

void MecVm::Push(const Value &data) {

#ifdef STACK_BOUNDS_CHECKING
    if (m_StackPtr >= STACK_END_PTR) {
        SetStatus(vmStackOverflow);
#ifdef DEBUG_TRACE_EXECUTION
        MSG("STACK OVERFLOW!");
#endif
        return;
    }
#endif

    *m_StackPtr = data;

#ifdef DEBUG_TRACE_EXECUTION
    u32 stack = STACK_POSITION;
    MSG("    >> Push[" << stack << "] = " << Debugger::PrintValue(*m_StackPtr));
#endif

    ++m_StackPtr;
}

void MecVm::PushN(const u32 num) {

#ifdef STACK_BOUNDS_CHECKING
    if (m_StackPtr + num >= STACK_END_PTR) {
        SetStatus(vmStackOverflow);
#ifdef DEBUG_TRACE_EXECUTION
        MSG("STACK OVERFLOW!");
#endif
        return;
    }
#endif

#ifdef DEBUG_TRACE_EXECUTION
    MSG("Pushing >>> " << num);
#endif

    m_StackPtr += num;
}

Value MecVm::Pop() {

#ifdef STACK_BOUNDS_CHECKING
    if (m_StackPtr <= STACK_LOCALS_START_PTR) {
        SetStatus(vmStackOverflow);
#ifdef DEBUG_TRACE_EXECUTION
        MSG("STACK UNDERFLOW!");
#endif
        return *m_StackPtr;
    }
#endif

    --m_StackPtr;

#ifdef DEBUG_TRACE_EXECUTION
    u32 stack = STACK_POSITION;
    MSG("    << Pop[" << stack << "] = " << Debugger::PrintValue(*m_StackPtr));
#endif

    return *m_StackPtr;
}

Value MecVm::PopN(const u32 num) {

#ifdef STACK_BOUNDS_CHECKING
    if ((m_StackPtr - num) < STACK_LOCALS_START_PTR)
        return *STACK_LOCALS_START_PTR;
#endif

#ifdef DEBUG_TRACE_EXECUTION
    MSG("Popping <<< " << num);
#endif

    m_StackPtr -= num;
    return *m_StackPtr;
}

Value MecVm::Peek(const u32 pos) {

#ifdef STACK_BOUNDS_CHECKING
    if ((m_StackPtr - pos) < STACK_LOCALS_START_PTR) {
        return *STACK_LOCALS_START_PTR;
    }
#endif

    return *(m_StackPtr - pos);
}

void MecVm::Duplicate(const u32 count) {

    for (u32 i = 0; i < count; ++i) {
        Push(Peek(count));
    }
}

bool MecVm::Call(const funcPtr_t functionId, const int argCount) {

    if (m_StackPtr >= STACK_END_PTR) {
        SetStatus(vmCallFrameOverflow);
        return false;
    }

    // Store the return Ip to the previously stored frame
    if (m_Frame.Enclosing != nullptr) {
        m_Frame.Enclosing->Ip = m_Frame.Ip;
    }

    // Update the new frame data
    m_Frame.Ip = (PGM_CODE + functionId + 3);
    // Check we're at a valid function header
    if (m_Frame.Ip[-3] != OP_FUNCTION_START) {
        SetStatus(vmCallNotAFunction);
        return false;
    }

    // Return Type
    // Info is stored in the code but not currently used
    // const DataType returnType = (DataType)(m_Frame.Ip[-2]);

    // Sanity check the arg count
    const u8 arity = m_Frame.Ip[-1];
    if (argCount != arity) {
        SetStatus(vmCallArgCountError);
        return false;
    }

    // Place the slot frame at the start of the arguments list.
    // ...[][this*][arg0][arg1][arg...]
    m_Frame.Slots = m_StackPtr - argCount;

#ifdef DEBUG_TRACE_EXECUTION
    u32 pos = STACK_POS_AT(m_Frame.Slots);
    MSG("Call frame slots position: " << pos);
#endif

    return true;
}

bool MecVm::CallNative(const NativeFuncId nativeId, const int argCount) {
    const NativeFunc nativeFunc = ResolveNativeFunction(nativeId, argCount);
    if (nativeFunc == nullptr) {
        // Function couldn't be resolved.
        SetStatus(vmNativeFunctionNotResolved);
        return false;
    }

    Value *args;
    if (nativeId == nfPrint || nativeId == nfPrintLn) {
        args = String((m_StackPtr - argCount)->UInt);
    } else {
        args = (m_StackPtr - argCount);
    }

    const Value result = nativeFunc(m_SystemParameter, argCount, args);

    m_StackPtr -= (argCount + 1);
    Push(result);

    return true;
}

Value *MecVm::String(const u32 index) const {
    const u32 stringIndex = (index >> 2);
    if (stringIndex > m_Script->Strings.Count) {
        return nullptr;
    }
    Value *strings = m_Script->Strings.Values;
    return (strings + stringIndex);
}

VmStatus MecVm::SetStatus(const VmStatus status) {

    m_Status = status;
    return m_Status;
}

void MecVm::Reset() {

    if (m_Script == nullptr) {
        m_StackPtr = nullptr;
        m_StackEnd = nullptr;
    } else {
        m_StackPtr = m_Script->Stack.Values;
        m_StackEnd = (m_Script->Stack.Values + m_Script->Stack.Count);
        m_Frame.Slots = m_StackPtr;
        m_Frame.Ip = m_Script->Code.Data;
        m_Frame.Enclosing = nullptr;
    }
}

u32 MecVm::DecodeScript(u8 *data, const u32 dataSize, u8 *stack, const u32 stackSize, ScriptInfo *script) {

    if (data == nullptr || dataSize == 0 || stack == nullptr || stackSize == 0 || script == nullptr)
        return 0;

    const ScriptBinaryHeader *header = (ScriptBinaryHeader *) data;

    // Validate the header
    if (header->HeaderSize != sizeof (ScriptBinaryHeader))
        return 0;

    // Validate the entire script will fit in the given data
    if (header->TotalSize > dataSize) {
        return 0;
    }

    // Checksum
    u32 checksum = Checksum::Calculate(data + header->CodePos, header->TotalSize - header->CodePos);
    if (checksum != header->CheckSum) {
        return 0;
    }

    // Globals fill up the bottom of the stack.
    // Stack starts from the top of the globals.
    uint32_t stackOffset = header->GlobalsSize;
    // Make sure the stack is aligned properly.
    while ((stackOffset & 0x03) != 0) ++stackOffset;

    script->Code.Data = (data + header->CodePos);
    script->Code.Length = (header->ConstantsPos / sizeof(opCode_t));
    script->Constants.Values = (Value *) (data + header->ConstantsPos);
    script->Constants.Count = ((header->StringsPos - header->ConstantsPos) / sizeof(Value));
    script->Strings.Values = (Value *) (data + header->StringsPos);
    script->Strings.Count = ((dataSize - header->StringsPos) / sizeof(Value));
    script->Globals.Values = (Value *) stack;
    script->Globals.Count = (header->GlobalsSize / sizeof(Value));
    script->Stack.Values = (Value *) (stack + stackOffset);
    script->Stack.Count = (stackSize - stackOffset) / sizeof(Value);

    if ((header->Flags & cfEmbeddedFileName) && script->Strings.Count > 0) {
        script->FileName = (char *)&script->Strings.Values[0];
    } else {
        script->FileName = nullptr;
    }

    return header->TotalSize;
}

void MecVm::SetNativeFunctionResolver(ResolverFunction resolver) {

    FunctionResolver = resolver;
}

NativeFunc MecVm::ResolveNativeFunction(const NativeFuncId funcId, const u8 argCount) {

    if (FunctionResolver)
        return FunctionResolver(funcId, argCount);

    return nullptr;
}

Value *MecVm::ResolvePointer(const VmPointer &pointer) {

    switch (pointer.Scope) {
        case scopeStackAbsolute:
        case scopeGlobal: {
            return &PGM_GLOBALS[pointer.Address];
        }
        case scopeLocal: {
            return &FRAME_LOCALS[pointer.Address];
        }
        case scopeField: {
            VmPointer absPtr = AS_POINTER(FRAME_LOCALS[0]);
            absPtr.Address += pointer.Address;
            return PGM_GLOBALS + absPtr.Address;
        }
        default: {
            return nullptr;
        }
    }
}

void MecVm::IncrementValue(const VmPointer &pointer, bool push) {

    Value *value = ResolvePointer(pointer);

    switch (pointer.Type) {
        case dtInt8:
            ++value->Char;
            break;
        case dtUint8:
            ++value->Byte;
            break;
        case dtInt16:
            ++value->Short;
            break;
        case dtUint16:
            ++value->UShort;
            break;
        case dtInt32:
            ++value->Int;
            break;
        case dtUint32:
            ++value->UInt;
            break;
        case dtFloat:
            ++value->Float;
            break;
        default:
            ++value->Int;
            break;
    }

    if (push)
        Push(*value);
}

void MecVm::DecrementValue(const VmPointer &pointer, bool push) {

    Value *value = ResolvePointer(pointer);

    switch (pointer.Type) {
        case dtInt8:
            --value->Char;
            break;
        case dtUint8:
            --value->Byte;
            break;
        case dtInt16:
            --value->Short;
            break;
        case dtUint16:
            --value->UShort;
            break;
        case dtInt32:
            --value->Int;
            break;
        case dtUint32:
            --value->UInt;
            break;
        case dtFloat:
            --value->Float;
            break;
        default:
            --value->Int;
            break;
    }

    if (push)
        Push(*value);
}
