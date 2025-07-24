//
// Created by Declan Walsh on 21/03/2024.
//

#ifndef VALUE_H_
#define VALUE_H_

#include "BasicTypes.h"

typedef uint32_t funcPtr_t;

#define NOT_SET             (-0x80081E5)

#define BOOL_VAL(value)     ((Value){ .Bool = value })
#define INT8_VAL(value)     ((Value){ .Char = (int8_t)value })
#define UINT8_VAL(value)    ((Value){ .Byte = (uint8_t)value })
#define INT16_VAL(value)    ((Value){ .Short = (int16_t)value })
#define UINT16_VAL(value)   ((Value){ .UShort = (uint16_t)value })
#define INT32_VAL(value)    ((Value){ .Int = (int)value })
#define UINT32_VAL(value)   ((Value){ .UInt = (uint32_t)value })
#define FLOAT_VAL(value)    ((Value){ .Float = (float)value })
#define FUNCTION_VAL(value) ((Value){ .FuncPointer = (funcPtr_t)value })
#define NATIVE_VAL(value)   ((Value){ .FuncPointer = (funcPtr_t)value })
#define POINTER_VAL(value)  ((Value){ .Pointer = (VmPointer)value })

#define AS_BOOL(value)      ((value).Bool)
#define AS_INT8(value)      ((value).Char)
#define AS_UINT8(value)     ((value).Byte)
#define AS_INT16(value)     ((value).Short)
#define AS_UINT16(value)    ((value).UShort)
#define AS_INT32(value)     ((value).Int)
#define AS_UINT32(value)    ((value).UInt)
#define AS_FLOAT(value)     ((value).Float)
#define AS_FUNCTION(value)  ((value).FuncPointer)
#define AS_NATIVE(value)    ((value).FuncPointer)
#define AS_POINTER(value)   ((value).Pointer)

#define NULL_VALUE          0

enum DataType : u8 {
    dtNone = 0,

    // Void
    dtVoid,

    // Numbers
    dtBool, // Integers
    dtInt8,
    dtUint8,
    dtInt16,
    dtUint16,
    dtInt32,
    dtUint32,
    dtFloat, // Float

    // Other
    dtPointer,
    dtFunction,
    dtNativeFunc,
    dtClass,
    dtCppPointer, // This is problematic with 64bit/32bit discrepancies
    dtString,

    // User
    dtUserStruct,
};

enum VarScopeType : uint8_t {
    scopeStackAbsolute = 0,
    scopeGlobal,
    scopeLocal,
    scopeField,
};

class VmPointer
{
  public:
    uint16_t Address;
    DataType Type;
    VarScopeType Scope;

    VmPointer();
    VmPointer(u16 address, DataType type, VarScopeType scope);

    bool operator==(const VmPointer &other) const
    {
        if (Type == dtNone && other.Type == dtNone)
            return true;
        if (other.Type != Type)
            return false;
        if (other.Scope != Scope)
            return false;
        if (other.Address != Address)
            return false;
        return true;
    }

    static VmPointer Null();
};

union Value {
    bool Bool;
    char Char;
    char Chars[4];
    uint8_t Byte;
    uint8_t Bytes[4];
    int16_t Short;
    int16_t Shorts[2];
    uint16_t UShort;
    uint16_t UShorts[2];
    int32_t Int;
    uint32_t UInt = 0;
    float Float;
    VmPointer Pointer;
    funcPtr_t FuncPointer;
};

#endif // VALUE_H_
