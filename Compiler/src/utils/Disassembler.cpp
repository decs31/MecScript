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
#define READ_BYTE()             (uint8_t)m_Code[m_Pos++]
#define READ_UINT16()           (m_Pos += 2, (uint16_t)(m_Code[m_Pos-2] | (m_Code[m_Pos-1] << 8)))
#define READ_UINT24()           (m_Pos += 3, (uint32_t)(m_Code[m_Pos-3] | (m_Code[m_Pos-2] << 8) | (m_Code[m_Pos-1] << 16)))
#define READ_INT32()            (m_Pos += 4, (int32_t)(m_Code[m_Pos-4] | (m_Code[m_Pos-3] << 8) | (m_Code[m_Pos-2] << 16) | (m_Code[m_Pos-1] << 24)))

#define UINT16_AT(pos)          (uint16_t)(m_Code[pos] | (m_Code[pos + 1] << 8))
#define INT32_AT(pos)           (int32_t)(m_Code[pos] | (m_Code[pos + 1] << 8) | (m_Code[pos + 2] << 16) | (m_Code[pos + 3] << 24))


#define COL_BIN                 14
#define COL_ADDR                0
#define COL_OP                  8
#define COL_ARGS                28

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

    size_t addr = m_Pos - m_CodeStartPos;
    opCode_t op = m_Code[m_Pos++];

    if (m_CurrentJumpTableEnd > 0 && m_Pos >= m_CurrentJumpTableStart) {
        // Process Jump Table Address
        if (m_Pos >= m_CurrentJumpTableStart) {
            instr = std::format("{:6}", addr) + ":";
            ALIGN_STRING(instr, COL_OP);
            instr += "JUMP_TBL_ADDR";
            ALIGN_STRING(instr, COL_ARGS);
            u16 jumpAddr = (u16) (op | ((u16) READ_BYTE() << 8));
            instr += STRING(jumpAddr);
        }
        if (m_Pos >= m_CurrentJumpTableEnd) {
            m_CurrentJumpTableStart = 0;
            m_CurrentJumpTableEnd = 0;
        }
    } else {
        // Process Instruction
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
                u8 index = READ_BYTE();
                instr = WriteInstruction(addr, "GET_CONST", STRING(index));
                desc = "Get the constant value at the 8 bit offset and push it onto the stack";
                break;
            }
            case OP_CONSTANT_16: {
                u16 index = READ_UINT16();
                instr = WriteInstruction(addr, "GET_CONST", STRING(index));
                desc = "Get the constant value at the 16 bit offset and push it onto the stack";
                break;
            }
            case OP_CONSTANT_24: {
                u32 index = READ_UINT24();
                instr = WriteInstruction(addr, "GET_CONST", STRING(index));
                desc = "Get the constant value at the 16 bit offset and push it onto the stack";
                break;
            }
            case OP_STRING: {
                u8 index = READ_BYTE();
                instr = WriteInstruction(addr, "GET_STRING", STRING(index));
                desc = "Get the string pointer at the 8 bit offset and push it onto the stack";
                break;
            }
            case OP_STRING_16: {
                u16 index = READ_UINT16();
                instr = WriteInstruction(addr, "GET_STRING", STRING(index));
                desc = "Get the string pointer at the 16 bit offset and push it onto the stack";
                break;
            }
            case OP_STRING_24: {
                u32 index = READ_UINT24();
                instr = WriteInstruction(addr, "GET_STRING", STRING(index));
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
            case OP_ARRAY: {
                u16 size = READ_UINT16();
                instr = WriteInstruction(addr, "ARRAY", STRING(size));
                desc = "Grow the stack by the size of the array";
                break;
            }

            case OP_GET_INDEXED_S8: {
                instr = WriteInstruction(addr, "GET_INDEXED_S8");
                desc = "Get indexed value from array of S8";
                break;
            }
            case OP_GET_INDEXED_U8: {
                instr = WriteInstruction(addr, "GET_INDEXED_U8");
                desc = "Get indexed value from array of U8";
                break;
            }
            case OP_GET_INDEXED_S16: {
                instr = WriteInstruction(addr, "GET_INDEXED_S16");
                desc = "Get indexed value from array of S16";
                break;
            }
            case OP_GET_INDEXED_U16: {
                instr = WriteInstruction(addr, "GET_INDEXED_U16");
                desc = "Get indexed value from array of U16";
                break;
            }
            case OP_GET_INDEXED_S32: {
                instr = WriteInstruction(addr, "GET_INDEXED_S32");
                desc = "Get indexed value from array of S32";
                break;
            }
            case OP_GET_INDEXED_U32: {
                instr = WriteInstruction(addr, "GET_INDEXED_U32");
                desc = "Get indexed value from array of U32";
                break;
            }
            case OP_GET_INDEXED_FLOAT: {
                instr = WriteInstruction(addr, "GET_INDEXED_FLOAT");
                desc = "Get indexed value from array of FLOAT";
                break;
            }
            case OP_SET_INDEXED_S8: {
                instr = WriteInstruction(addr, "SET_INDEXED_S8");
                desc = "Set indexed value from array of S8";
                break;
            }
            case OP_SET_INDEXED_U8: {
                instr = WriteInstruction(addr, "SET_INDEXED_U8");
                desc = "Set indexed value from array of U8";
                break;
            }
            case OP_SET_INDEXED_S16: {
                instr = WriteInstruction(addr, "SET_INDEXED_S16");
                desc = "Set indexed value from array of S16";
                break;
            }
            case OP_SET_INDEXED_U16: {
                instr = WriteInstruction(addr, "SET_INDEXED_U16");
                desc = "Set indexed value from array of U16";
                break;
            }
            case OP_SET_INDEXED_S32: {
                instr = WriteInstruction(addr, "SET_INDEXED_S32");
                desc = "Set indexed value from array of S32";
                break;
            }
            case OP_SET_INDEXED_U32: {
                instr = WriteInstruction(addr, "SET_INDEXED_U32");
                desc = "Set indexed value from array of U32";
                break;
            }
            case OP_SET_INDEXED_FLOAT: {
                instr = WriteInstruction(addr, "SET_INDEXED_FLOAT");
                desc = "Set indexed value from array of FLOAT";
                break;
            }

                // Casting
            case OP_CAST_INT_TO_FLOAT: {
                instr = WriteInstruction(addr, "CAST_INT_TO_FLOAT");
                desc = "Cast int to float";
                break;
            }
            case OP_CAST_PREV_INT_TO_FLOAT: {
                instr = WriteInstruction(addr, "CAST_PREV_INT_TO_FLOAT");
                desc = "Cast int at stack top -1 to float";
                break;
            }
            case OP_CAST_FLOAT_TO_INT: {
                instr = WriteInstruction(addr, "CAST_FLOAT_TO_INT");
                desc = "Cast float to int";
                break;
            }
            case OP_CAST_PREV_FLOAT_TO_INT: {
                instr = WriteInstruction(addr, "CAST_PREV_FLOAT_TO_INT");
                desc = "Cast float at stack top -1 to int";
                break;
            }

                // Unary
            case OP_NEGATE_I: {
                instr = WriteInstruction(addr, "NEGATE_I");
                desc = "Negate the int value at the top of the stack";
                break;
            }
            case OP_NEGATE_F: {
                instr = WriteInstruction(addr, "NEGATE_F");
                desc = "Negate the float value at the top of the stack";
                break;
            }
            case OP_BIT_NOT: {
                instr = WriteInstruction(addr, "BIT_NOT");
                desc = "Bitwise Not";
                break;
            }
            case OP_PREFIX_DECREASE: {
                instr = WriteInstruction(addr, "PREFIX_DEC");
                desc = "Decrement the value, then push onto the stack";
                break;
            }
            case OP_PREFIX_INCREASE: {
                instr = WriteInstruction(addr, "PREFIX_INC");
                desc = "Increment the value, then push onto the stack";
                break;
            }
            case OP_MINUS_MINUS: {
                instr = WriteInstruction(addr, "MINUS_MINUS");
                desc = "Decrement the value in place";
                break;
            }
            case OP_PLUS_PLUS: {
                instr = WriteInstruction(addr, "PLUS_PLUS");
                desc = "Increment the value in place";
                break;
            }

                // Binary
            case OP_ADD_S: {
                instr = WriteInstruction(addr, "ADD_S");
                desc = "Add (Signed)";
                break;
            }
            case OP_ADD_U: {
                instr = WriteInstruction(addr, "ADD_U");
                desc = "Add (Unsigned)";
                break;
            }
            case OP_ADD_F: {
                instr = WriteInstruction(addr, "ADD_F");
                desc = "Add (Float)";
                break;
            }
            case OP_SUB_S: {
                instr = WriteInstruction(addr, "SUB_S");
                desc = "Sub (Signed)";
                break;
            }
            case OP_SUB_U: {
                instr = WriteInstruction(addr, "SUB_U");
                desc = "Sub (Unsigned)";
                break;
            }
            case OP_SUB_F: {
                instr = WriteInstruction(addr, "SUB_F");
                desc = "Sub (Float)";
                break;
            }
            case OP_MULT_S: {
                instr = WriteInstruction(addr, "MULT_S");
                desc = "Multiply (Signed)";
                break;
            }
            case OP_MULT_U: {
                instr = WriteInstruction(addr, "MULT_U");
                desc = "Multiply (Unsigned)";
                break;
            }
            case OP_MULT_F: {
                instr = WriteInstruction(addr, "MULT_F");
                desc = "Sub (Float)";
                break;
            }
            case OP_DIV_S: {
                instr = WriteInstruction(addr, "DIV_S");
                desc = "Divide (Signed)";
                break;
            }
            case OP_DIV_U: {
                instr = WriteInstruction(addr, "DIV_U");
                desc = "Divide (Unsigned)";
                break;
            }
            case OP_DIV_F: {
                instr = WriteInstruction(addr, "DIV_F");
                desc = "Divide (Float)";
                break;
            }
            case OP_MODULUS: {
                instr = WriteInstruction(addr, "MODULUS");
                desc = "Modulus";
                break;
            }
            case OP_ASSIGN: {
                instr = WriteInstruction(addr, "ASSIGN");
                desc = "Assign value";
                break;
            }
            case OP_BIT_AND: {
                instr = WriteInstruction(addr, "BIT_AND");
                desc = "Bitwise And";
                break;
            }
            case OP_BIT_OR: {
                instr = WriteInstruction(addr, "BIT_OR");
                desc = "Bitwise Or";
                break;
            }
            case OP_BIT_XOR: {
                instr = WriteInstruction(addr, "BIT_XOR");
                desc = "Bitwise XOr";
                break;
            }
            case OP_BIT_SHIFT_L: {
                instr = WriteInstruction(addr, "BIT_SHIFT_L");
                desc = "Bitwise Shift Left";
                break;
            }
            case OP_BIT_SHIFT_R: {
                instr = WriteInstruction(addr, "BIT_SHIFT_R");
                desc = "Bitwise Shift Right";
                break;
            }

                // Logic
            case OP_NOT: {
                instr = WriteInstruction(addr, "NOT");
                desc = "Check the value is false and push the result";
                break;
            }
            case OP_EQUAL_S: {
                instr = WriteInstruction(addr, "EQUAL_S");
                desc = "Check values are equal (Signed)";
                break;
            }
            case OP_EQUAL_U: {
                instr = WriteInstruction(addr, "EQUAL_U");
                desc = "Check values are equal (Unsigned)";
                break;
            }
            case OP_EQUAL_F: {
                instr = WriteInstruction(addr, "EQUAL_F");
                desc = "Check values are equal (Float)";
                break;
            }
            case OP_NOT_EQUAL_S: {
                instr = WriteInstruction(addr, "NOT_EQUAL_S");
                desc = "Check values are not equal (Signed)";
                break;
            }
            case OP_NOT_EQUAL_U: {
                instr = WriteInstruction(addr, "NOT_EQUAL_U");
                desc = "Check values are not equal (Unsigned)";
                break;
            }
            case OP_NOT_EQUAL_F: {
                instr = WriteInstruction(addr, "NOT_EQUAL_F");
                desc = "Check values are not equal (Float)";
                break;
            }
            case OP_LESS_S: {
                instr = WriteInstruction(addr, "LESS_S");
                desc = "Check values is lesser (Signed)";
                break;
            }
            case OP_LESS_U: {
                instr = WriteInstruction(addr, "LESS_U");
                desc = "Check values is lesser (Unsigned)";
                break;
            }
            case OP_LESS_F: {
                instr = WriteInstruction(addr, "LESS_F");
                desc = "Check values is lesser (Float)";
                break;
            }
            case OP_LESS_OR_EQUAL_S: {
                instr = WriteInstruction(addr, "LESS_EQUAL_S");
                desc = "Check values is lesser or equal (Signed)";
                break;
            }
            case OP_LESS_OR_EQUAL_U: {
                instr = WriteInstruction(addr, "LESS_EQUAL_U");
                desc = "Check values is lesser or equal (Unsigned)";
                break;
            }
            case OP_LESS_OR_EQUAL_F: {
                instr = WriteInstruction(addr, "LESS_EQUAL_F");
                desc = "Check values is lesser or equal (Float)";
                break;
            }
            case OP_GREATER_S: {
                instr = WriteInstruction(addr, "GREATER_S");
                desc = "Check values is greater (Signed)";
                break;
            }
            case OP_GREATER_U: {
                instr = WriteInstruction(addr, "GREATER_U");
                desc = "Check values is greater (Unsigned)";
                break;
            }
            case OP_GREATER_F: {
                instr = WriteInstruction(addr, "GREATER_F");
                desc = "Check values is greater (Float)";
                break;
            }
            case OP_GREATER_OR_EQUAL_S: {
                instr = WriteInstruction(addr, "GREATER_EQUAL_S");
                desc = "Check values is greater or equal (Signed)";
                break;
            }
            case OP_GREATER_OR_EQUAL_U: {
                instr = WriteInstruction(addr, "GREATER_EQUAL_U");
                desc = "Check values is greater or equal (Unsigned)";
                break;
            }
            case OP_GREATER_OR_EQUAL_F: {
                instr = WriteInstruction(addr, "GREATER_EQUAL_F");
                desc = "Check values is greater or equal (Float)";
                break;
            }

                // Flow Control
            case OP_JUMP: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "JUMP", STRING(offset));
                desc = "Unconditionally jump the instruction pointer";
                break;
            }
            case OP_BREAK: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "BREAK", STRING(offset));
                desc = "Jump the instruction pointer out of the current block";
                break;
            }
            case OP_JUMP_IF_FALSE: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "JUMP_FALSE", STRING(offset));
                desc = "Jump the instruction pointer if the value is false";
                break;
            }
            case OP_JUMP_IF_TRUE: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "JUMP_TRUE", STRING(offset));
                desc = "Jump the instruction pointer if the value is true";
                break;
            }
            case OP_JUMP_IF_EQUAL: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "JUMP_EQUAL", STRING(offset));
                desc = "Jump the instruction pointer if the values are equal";
                break;
            }
            case OP_CONTINUE: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "CONTINUE", STRING(offset));
                desc = "Jump the instruction pointer back to the start of the loop";
                break;
            }
            case OP_LOOP: {
                u16 offset = READ_UINT16();
                instr = WriteInstruction(addr, "LOOP", STRING(offset));
                desc = "Jump the instruction pointer back to the start of the loop";
                break;
            }
            case OP_SWITCH: {
                u16 end = READ_UINT16();
                m_CurrentJumpTableEnd = m_Pos - 1 + end;
                s32 min = READ_INT32();
                s32 max = READ_INT32();
                m_CurrentJumpTableStart = m_CurrentJumpTableEnd - (((max - min) + 1) * 2);
                instr = WriteInstruction(addr, "SWITCH", STRING(end), STRING(min), STRING(max));
                desc = "[End][Min][Max] Set up a jump table and jump to the desired offset";
                break;
            }
            case OP_FRAME: {
                instr = WriteInstruction(addr, "FRAME");
                desc = "Stores the current call frame on the stack";
                break;
            }
            case OP_CALL: {
                u8 args = READ_BYTE();
                instr = WriteInstruction(addr, "CALL", STRING(args));
                desc = "[Arg Count] Calls a function";
                break;
            }
            case OP_CALL_NATIVE: {
                u8 args = READ_BYTE();
                instr = WriteInstruction(addr, "CALL_NATIVE", STRING(args));
                desc = "[Arg Count] Calls a native function";
                break;
            }
            case OP_CALL_NO_ARGS: {
                u16 funcId = READ_UINT16();
                instr = WriteInstruction(addr, "CALL_NO_ARG", STRING(funcId));
                desc = "[Func ID] Calls a function";
                break;
            }
            case OP_RETURN: {
                instr = WriteInstruction(addr, "RETURN");
                desc = "Return from called function";
                break;
            }

            case OP_END: {
                instr = WriteInstruction(addr, "END");
                desc = "<< END OF PROGRAM >>";
                break;
            }

            case OP_FUNCTION_START: {
                int id = (int)addr;
                u8 returnType = READ_BYTE();
                u8 argCount = READ_BYTE();
                instr = WriteFunctionHeader(id, returnType, argCount);
                break;
            }

            default: {
                instr = WriteInstruction(addr, "UNKNOWN!", STRING(op));
                break;
            }
        }
    }

    string bin;
    size_t addrPos = (addr + m_CodeStartPos);
    int count = 0;
    for (size_t i = addrPos; i < m_Pos; ++i) {
        count ++;
        opCode_t b = m_Code[i];
        if (b < 0x10)
            bin += "0";
        bin += std::format("{:X}", b) + " ";
    }
    if (count > 4) {
        size_t len = bin.length();
        bin += "\n";
        ALIGN_STRING(bin, COL_BIN + len - 1);
    } else {
        ALIGN_STRING(bin, COL_BIN - 2);
    }
    bin += "| ";

    if (m_ShowDescription)
        return bin + instr + " | " + desc;
    else
        return bin + instr;
}

string Disassembler::WriteFunctionHeader(int id, u8 returnType, u8 argCount)
{
    string func = "<<< Function [" + STRING(id) + "] "
                + "(" + STRING(argCount) + ") "
                + ": " + STRING(returnType) + " >>>";
    return func;
}

string Disassembler::WriteInstruction(size_t addr, const string &op, const string &arg1, const string &arg2, const string &arg3,
                                      const string &arg4) {
    string str = std::format("{:6}", addr) + ":";

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

string Disassembler::ReadHex(int count) {

    string line;
    for (int i = 0; i < count; ++i) {
        opCode_t b = m_Code[m_Pos++];
        if (b < 0x10)
            line += "0";
        line += std::format("{:X}", b) + " ";
    }

    return line;
}

string Disassembler::ReadString() {
    string str = "\"";
    do {
        char c = (char)m_Code[m_Pos++];
        str += c;
    } while (m_Code[m_Pos] != 0);
    str += "\"";

    // Strings are terminated and padded to 4 byte boundaries with null chars.
    while (m_Code[m_Pos] == 0) {
        m_Pos++;
    }

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

    while(m_Pos < m_ConstantsPos) {
        string line = ReadInstruction();
        OutputLine(line);
    }
    OutputLine(divider);
    OutputLine("    ");

    // Constants
    OutputLine("CONSTANTS");
    OutputLine(divider);
    int constId = 0;
    while (m_Pos < m_StringsPos) {
        string c = std::format("{:4}", constId++) + ":";
        ALIGN_STRING(c, 8);
        c += ReadHex(4);
        c += " |  " + STRING(INT32_AT(m_Pos - 4));
        OutputLine(c);
    }

    OutputLine(divider);
    OutputLine("    ");

    // Strings
    OutputLine("STRINGS");
    OutputLine(divider);
    int stringId = 0;
    while (m_Pos < m_Length) {
        string str = std::format("{:4}", stringId++) + ":";
        ALIGN_STRING(str, 8);
        str += ReadString();
        OutputLine(str);
    }


    OutputLine(divider);
    OutputLine("    ");

    OutputLine("========== END ==========");
}
