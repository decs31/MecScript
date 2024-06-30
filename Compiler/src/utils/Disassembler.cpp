//
// Created by Declan Walsh on 25/06/2024.
//

#include <format>
#include "Disassembler.h"
#include "Checksum.h"

// Handy Macros
#define STRING(var)             std::to_string(var)
#define ALIGN_STRING(str, pos)  while(str.length() < pos) str += " "

/* Instruction Readers */
#define READ_BYTE()             STRING(m_Code[m_Pos++])
#define READ_UINT16()           STRING((m_Pos += 2, (uint16_t)(m_Code[m_Pos-2] | (m_Code[m_Pos-1] << 8))))
#define READ_UINT24()           STRING((m_Pos += 3, (uint32_t)(m_Code[m_Pos-3] | (m_Code[m_Pos-2] << 8) | (m_Code[m_Pos-1] << 16))))
#define READ_INT32()            STRING((m_Pos += 4, (int32_t)(m_Code[m_Pos-4] | (m_Code[m_Pos-3] << 8) | (m_Code[m_Pos-2] << 16) | (m_Code[m_Pos-1] << 24))))

#define COL_BIN                 16
#define COL_ADDR                0
#define COL_OP                  8
#define COL_ARGS                24

static const string divider = "--------------------------------------------------------------";

Disassembler::Disassembler(u8 *code, const size_t length) {
    SetCode(code, length);
}

void Disassembler::SetCode(u8 *code, const size_t length) {
    m_Code = code;
    m_Length = length;
    m_Pos = 0;
    m_Checksum = 0;
    m_CodeStartPos = 0;
    m_ConstantsPos = 0;
    m_StringsPos = 0;
    m_GlobalsSize = 0;
}

void Disassembler::OutputLine(const string &line) {

    std::cout << line << std::endl;
}

bool Disassembler::ReadHeader() {
    if (m_Code == nullptr || m_Length < sizeof (ProgramBinaryHeader))
        return false;

    ProgramBinaryHeader *header = (ProgramBinaryHeader *)m_Code;
    if (header->HeaderSize != sizeof (ProgramBinaryHeader)) {
        OutputLine("Script header invalid!");
        return false;
    }

    if (header->TotalSize != m_Length) {
        OutputLine("Script size invalid!");
        return false;
    }

    m_Checksum = header->CheckSum;
    m_CodeStartPos = header->CodePos;
    m_ConstantsPos = header->ConstantsPos;
    m_StringsPos = header->StringsPos;
    m_GlobalsSize = header->GlobalsSize;

    OutputLine("========== MecScript Disassembly ==========");
    OutputLine("    Header Size:       " + STRING(header->HeaderSize) + " bytes");
    OutputLine("    Header Version:    " + STRING(header->HeaderVersion));
    OutputLine("    Language Version:  " + STRING(header->LangVersionMajor) + "." + STRING(header->LangVersionMinor));
    OutputLine("    Build Day/Time:    " + STRING(header->BuildDay) + ":" + STRING(header->BuildTime));
    OutputLine("    Globals Size:      " + STRING(header->GlobalsSize) + " bytes");
    OutputLine("    Checksum:          " + STRING(header->CheckSum));
    //OutputLine(divider);
    OutputLine("    ");

    return true;
}

bool Disassembler::ValidateChecksum() {
    if (m_Code == nullptr || m_Length == 0)
        return false;

    if (m_Checksum == 0)
        return true;

    u32 checksum = Checksum::Calculate(&m_Code[m_CodeStartPos], m_Length - m_CodeStartPos);

    return m_Checksum == checksum;
}

string Disassembler::ReadInstruction() {

    string instr;
    string desc;

    size_t addr = m_Pos++;
    opCode_t op = m_Code[addr];

    switch (op) {
        case OP_NOP: {
            instr = WriteInstruction(addr, "NOP");
            desc = "No operation";
            break;
        }
        case OP_PUSH: {
            instr = WriteInstruction(addr, "PUSH");
            desc = "Push a value onto the stack";
            break;
        }
        case OP_PUSH_N: {
            instr = WriteInstruction(addr, "PUSH_N");
            desc = "Push N values onto the stack";
            break;
        }
        case OP_POP: {
            instr = WriteInstruction(addr, "POP");
            desc = "Pop the last value off the top of the stack";
            break;
        }
        case OP_POP_N: {
            instr = WriteInstruction(addr, "POP_N");
            desc = "Pop N values off the top of the stack";
            break;
        }
        case OP_DUPLICATE: {
            instr = WriteInstruction(addr, "DUPLICATE");
            desc = "Duplicates the stack top value and pushes it";
            break;
        }
        case OP_DUPLICATE_2: {
            instr = WriteInstruction(addr, "DUPLICATE2");
            desc = "Duplicates the top 2 stack values and pushes them";
            break;
        }
        case OP_NIL: {
            instr = WriteInstruction(addr, "NIL");
            desc = "Push 0 onto stack";
            break;
        }
        case OP_FALSE: {
            instr = WriteInstruction(addr, "FALSE");
            desc = "Push 'false' onto stack";
            break;
        }
        case OP_TRUE: {
            instr = WriteInstruction(addr, "TRUE");
            desc = "Push 'true' onto stack";
            break;
        }
        case OP_CONSTANT: {
            instr = WriteInstruction(addr, "GET_CONST", READ_BYTE());
            desc = "Get the constant value at the 8 bit offset and push it onto the stack";
            break;
        }
        case OP_CONSTANT_16: {
            instr = WriteInstruction(addr, "GET_CONST", READ_UINT16());
            desc = "Get the constant value at the 16 bit offset and push it onto the stack";
            break;
        }
        case OP_CONSTANT_24: {
            instr = WriteInstruction(addr, "GET_CONST", READ_UINT24());
            desc = "Get the constant value at the 16 bit offset and push it onto the stack";
            break;
        }
        case OP_STRING: {
            instr = WriteInstruction(addr, "GET_STRING", READ_BYTE());
            desc = "Get the string pointer at the 8 bit offset and push it onto the stack";
            break;
        }
        case OP_STRING_16: {
            instr = WriteInstruction(addr, "GET_STRING", READ_UINT16());
            desc = "Get the string pointer at the 16 bit offset and push it onto the stack";
            break;
        }
        case OP_STRING_24: {
            instr = WriteInstruction(addr, "GET_STRING", READ_UINT24());
            desc = "Get the string pointer at the 24 bit offset and push it onto the stack";
            break;
        }
        case OP_GET_VARIABLE: {
            instr = WriteInstruction(addr, "GET_VAR");
            desc = "Get variable and push it onto the stack";
            break;
        }
        case OP_SET_VARIABLE: {
            instr = WriteInstruction(addr, "SET_VAR");
            desc = "Set variable from the value on top of the stack";
            break;
        }
        case OP_ABSOLUTE_POINTER: {
            instr = WriteInstruction(addr, "ABS_PTR");
            desc = "Convert a scoped pointer to absolute";
            break;
        }

        // INDEXED VARS



        default: {
            instr = WriteInstruction(addr, "UNKNOWN!", STRING(op));
            break;
        }
    }

    string bin = std::format("{:2X}", op) + " ";
    for (size_t i = addr + 1; i < m_Pos; ++i) {
        bin += std::format("{:2X}", m_Code[i]) + " ";
    }
    ALIGN_STRING(bin, COL_BIN - 2);
    bin += "| ";

    if (m_ShowDescription)
        return bin + instr + " | " + desc;
    else
        return bin + instr;
}

string Disassembler::WriteInstruction(size_t addr, const string &op, const string &arg1, const string &arg2, const string &arg3,
                                      const string &arg4) {
    string str = STRING(addr) + ":";

    ALIGN_STRING(str, COL_OP);
    str += op;

    ALIGN_STRING(str, COL_ARGS);
    if (!arg1.empty())
        str += arg1;
    if (!arg2.empty())
        str += ", " + arg2;
    if (!arg3.empty())
        str += ", " + arg3;
    if (!arg4.empty())
        str += ", " + arg4;


    return str;
}

void Disassembler::Disassemble() {

    if (!ReadHeader())
        return;

    if (!ValidateChecksum())
        return;

    OutputLine(divider);
    string bin = "#";
    ALIGN_STRING(bin, COL_BIN);
    string header = "Addr";
    ALIGN_STRING(header, COL_OP);
    header += "Instruction";
    ALIGN_STRING(header, COL_ARGS);
    header += "Args";
    OutputLine(bin + header);
    OutputLine(divider);

    m_Pos = m_CodeStartPos;

    while(m_Pos < m_Length) {
        string line = ReadInstruction();
        OutputLine(line);
    }
    OutputLine(divider);
    OutputLine("    ");

    // Constants
    OutputLine("CONSTANTS");
    OutputLine(divider);


    OutputLine(divider);
    OutputLine("    ");

    // Strings
    OutputLine("STRINGS");
    OutputLine(divider);


    OutputLine(divider);
    OutputLine("    ");

    OutputLine("========== END ==========");
}
