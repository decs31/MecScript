//
// Created by Declan Walsh on 28/09/2024.
//

#ifndef SCRIPTINFO_H
#define SCRIPTINFO_H

#include "Instructions.h"
#include "Value.h"

#define LANG_VERSION_MAJOR 0
#define LANG_VERSION_MINOR 1

enum CompileOptions : u32 {
    coEmbeddedFileName = 0x01,
    coShortAddressing = 0x02,
    coDecompileResult = 0x04,
};

struct CodeData {
    opCode_t *Data;
    u32 Length;
};

struct ValueData {
    Value *Values;
    u32 Count;
};

struct ScriptInfo {
    CodeData Code;
    ValueData Constants;
    ValueData Strings;
    ValueData Globals;
    ValueData Stack;
    const char *FileName;
};

struct ScriptBinaryHeader {
    u8 HeaderSize;
    u8 Flags;
    u8 LangVersionMajor;
    u8 LangVersionMinor;
    u16 BuildDay;     // Days sine 01/01/2000
    u16 BuildTime;    // Seconds since midnight / 2
    u32 CodePos;      // Bytes
    u32 ConstantsPos; // Bytes
    u32 StringsPos;   // Bytes
    u32 GlobalsSize;  // Bytes
    u32 TotalSize;    // Bytes
    u32 CheckSum;     // XOR byte code
};

#endif // SCRIPTINFO_H
