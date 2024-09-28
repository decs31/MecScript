//
// Created by Declan Walsh on 18/02/2024.
//

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "Value.h"

typedef uint8_t opCode_t;

enum OpCode : opCode_t {
    OP_NOP = 0,

    // Slots
    OP_PUSH,
    OP_PUSH_N,
    OP_POP,
    OP_POP_N,

    // Stack Operations
    OP_DUPLICATE,
    OP_DUPLICATE_2,

    // Constants
    OP_NIL,
    OP_FALSE,
    OP_TRUE,
    OP_CONSTANT,
    OP_CONSTANT_16,
    OP_CONSTANT_24,
    OP_STRING,
    OP_STRING_16,
    OP_STRING_24,

    // Variables
    OP_GET_VARIABLE,
    OP_SET_VARIABLE,

    OP_ABSOLUTE_POINTER,

    // Array Indexes
    OP_ARRAY,
    OP_GET_INDEXED_S8,
    OP_GET_INDEXED_U8,
    OP_GET_INDEXED_S16,
    OP_GET_INDEXED_U16,
    OP_GET_INDEXED_S32,
    OP_GET_INDEXED_U32,
    OP_GET_INDEXED_FLOAT,
    OP_SET_INDEXED_S8,
    OP_SET_INDEXED_U8,
    OP_SET_INDEXED_S16,
    OP_SET_INDEXED_U16,
    OP_SET_INDEXED_S32,
    OP_SET_INDEXED_U32,
    OP_SET_INDEXED_FLOAT,

    // Casting
    OP_CAST_INT_TO_FLOAT,
    OP_CAST_PREV_INT_TO_FLOAT,
    OP_CAST_FLOAT_TO_INT,
    OP_CAST_PREV_FLOAT_TO_INT,

    // Math
    OP_MODULUS, // Always int
    OP_NEGATE_I,
    OP_NEGATE_F,
    OP_ADD_S,
    OP_ADD_U,
    OP_ADD_F,
    OP_SUB_S,
    OP_SUB_U,
    OP_SUB_F,
    OP_MULT_S,
    OP_MULT_U,
    OP_MULT_F,
    OP_DIV_S,
    OP_DIV_U,
    OP_DIV_F,

    // Inc / Dec
    OP_PREFIX_DECREASE,
    OP_PREFIX_INCREASE,
    OP_PLUS_PLUS,
    OP_MINUS_MINUS,

    // Assignment
    OP_ASSIGN,

    // Logic
    OP_NOT, // Always int
    OP_EQUAL_S,
    OP_EQUAL_U,
    OP_EQUAL_F,
    OP_NOT_EQUAL_S,
    OP_NOT_EQUAL_U,
    OP_NOT_EQUAL_F,
    OP_LESS_S,
    OP_LESS_U,
    OP_LESS_F,
    OP_LESS_OR_EQUAL_S,
    OP_LESS_OR_EQUAL_U,
    OP_LESS_OR_EQUAL_F,
    OP_GREATER_S,
    OP_GREATER_U,
    OP_GREATER_F,
    OP_GREATER_OR_EQUAL_S,
    OP_GREATER_OR_EQUAL_U,
    OP_GREATER_OR_EQUAL_F,

    // Bitwise
    OP_BIT_NOT,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_BIT_SHIFT_L,
    OP_BIT_SHIFT_R,

    // Functions
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_EQUAL,
    OP_LOOP,
    OP_SWITCH,
    OP_BREAK,
    OP_CONTINUE,
    OP_FRAME,
    OP_CALL,
    OP_CALL_NATIVE,
    OP_RETURN,


    // Reserved
    OP_FUNCTION_START = 254,
    OP_END = 255
};

struct CodeData {
    opCode_t *Data;
    u32 Length;
};

struct ValueData {
    Value *Values;
    u32 Count;
};

enum FunctionType {
    ftScript,
    ftFunction,
    ftClassInit,
    ftClassMethod,
    ftNative,
};

struct ProgramInfo {
    CodeData Code;
    ValueData Constants;
    ValueData Strings;
    ValueData Globals;
    ValueData Stack;
};

struct ProgramBinaryHeader {
    u8 HeaderSize;
    u8 HeaderVersion;
    u8 LangVersionMajor;
    u8 LangVersionMinor;
    u16 BuildDay; // Days sine 01/01/2000
    u16 BuildTime; // Seconds since midnight / 2
    u32 CodePos; // Bytes
    u32 ConstantsPos; // Bytes
    u32 StringsPos; // Bytes
    u32 GlobalsSize; // Bytes
    u32 TotalSize; // Bytes
    u32 CheckSum; // XOR byte code
};

#endif //INSTRUCTIONS_H
