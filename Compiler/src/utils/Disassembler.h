//
// Created by Declan Walsh on 25/06/2024.
//

#ifndef MECSCRIPT_DISASSEMBLER_H
#define MECSCRIPT_DISASSEMBLER_H

#include "ScriptUtils.h"
#include "Instructions.h"

class Disassembler {
public:
    Disassembler() = default;
    Disassembler(u8 *code, const size_t length);

    void SetCode(u8 *code, const size_t length);
    void Disassemble();

private:
    opCode_t *m_Code = nullptr;
    size_t m_Length = 0;
    size_t m_Pos = 0;
    size_t m_CodeStartPos = 0;
    size_t m_ConstantsPos = 0;
    size_t m_StringsPos = 0;
    size_t m_GlobalsSize = 0;
    u32 m_Checksum = 0;

    bool m_ShowDescription = false;

    size_t m_CurrentJumpTableStart = 0;
    size_t m_CurrentJumpTableEnd = 0;

    void OutputLine(const string &line);
    bool ReadHeader();
    bool ValidateChecksum();
    string ReadInstruction();
    string WriteFunctionHeader(int id, u8 returnType, u8 argCount);
    string WriteInstruction(size_t addr, const string &op, const string &arg1 = "", const string &arg2 = "",const string &arg3 = "", const string &arg4 = "");
    string ReadHex(int count);
    string ReadString();
};

#endif //MECSCRIPT_DISASSEMBLER_H
