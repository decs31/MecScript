//
// Created by Declan Walsh on 14/03/2024.
//

#include "Debugger.h"
#include "../../Compiler/src/utils/ScriptUtils.h"

#define DBG_PRINT_VALUE_OP(op, val)      MSG(op << "(" << val << ")")
#define DBG_READ_UINT8(codePtr)          (u32)((codePtr)[0])
#define DBG_READ_UINT16(codePtr)         (u32)((codePtr)[0] | ((codePtr)[1] << 8))
#define DBG_READ_UINT24(codePtr)         (u32)((codePtr)[0] | ((codePtr)[1] << 8) | ((codePtr)[2] << 16))
#define DBG_READ_UINT32(codePtr)         (u32)((codePtr)[0] | ((codePtr)[1] << 8) | ((codePtr)[2] << 16) | ((codePtr)[3] << 24))

void Debugger::DebugInstruction(opCode_t *codeStart, opCode_t *code) {

    size_t codePos = code - codeStart;
    PRINT("Code[" << codePos << "]: ");


    opCode_t op = code[0];
    opCode_t *valPtr = code + 1;
    switch (op) {
        case OP_NOP: {
            MSG("No Operation");
            break;
        }
        case OP_PUSH: {
            MSG("Push");
            break;
        }
        case OP_PUSH_N: {
            u32 count = DBG_READ_UINT8(valPtr);
            DBG_PRINT_VALUE_OP("PushN", count);
            break;
        }
        case OP_POP: {
            MSG("Pop");
            break;
        }
        case OP_POP_N: {
            u32 count = DBG_READ_UINT8(valPtr);
            DBG_PRINT_VALUE_OP("PopN", count);
            break;
        }
        case OP_DUPLICATE: {
            MSG("Duplicate");
            break;
        }
        case OP_DUPLICATE_2: {
            MSG("Duplicate2");
            break;
        }
        case OP_NIL: {
            MSG("NIL");
            break;
        }
        case OP_FALSE: {
            MSG("FALSE");
            break;
        }
        case OP_TRUE: {
            MSG("TRUE");
            break;
        }
        case OP_CONSTANT: {
            u32 addr = DBG_READ_UINT8(valPtr);
            DBG_PRINT_VALUE_OP("PushConst", addr);
            break;
        }
        case OP_CONSTANT_16: {
            u32 addr = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("PushConst16", addr);
            break;
        }
        case OP_CONSTANT_24: {
            u32 addr = DBG_READ_UINT24(valPtr);
            DBG_PRINT_VALUE_OP("PushConst24", addr);
            break;
        }

        case OP_GET_VARIABLE: {
            MSG("GetVariable");
            break;
        }

        case OP_SET_VARIABLE: {
            MSG("SetVariable");
            break;
        }

        // Indexed values
        case OP_GET_INDEXED_S8: {
            MSG("GetIndexedS8");
            break;
        }
        case OP_GET_INDEXED_U8: {
            MSG("GetIndexedU8");
            break;
        }
        case OP_GET_INDEXED_S16: {
            MSG("GetIndexedS16");
            break;
        }
        case OP_GET_INDEXED_U16: {
            MSG("GetIndexedU16");
            break;
        }
        case OP_GET_INDEXED_S32: {
            MSG("GetIndexedS32");
            break;
        }
        case OP_GET_INDEXED_U32: {
            MSG("GetIndexedU32");
            break;
        }
        case OP_GET_INDEXED_FLOAT: {
            MSG("GetIndexedFloat");
            break;
        }

        case OP_SET_INDEXED_S8: {
            MSG("SetIndexedS8");
            break;
        }
        case OP_SET_INDEXED_U8: {
            MSG("SetIndexedU8");
            break;
        }
        case OP_SET_INDEXED_S16: {
            MSG("SetIndexedS16");
            break;
        }
        case OP_SET_INDEXED_U16: {
            MSG("SetIndexedU16");
            break;
        }
        case OP_SET_INDEXED_S32: {
            MSG("SetIndexedS32");
            break;
        }
        case OP_SET_INDEXED_U32: {
            MSG("SetIndexedU32");
            break;
        }
        case OP_SET_INDEXED_FLOAT: {
            MSG("SetIndexedFloat");
            break;
        }

            // Math
        case OP_NEGATE_I: {
            MSG("Negate(-)");
            break;
        }
        case OP_ADD_S: {
            MSG("Add(+)");
            break;
        }
        case OP_SUB_S: {
            MSG("Subtract(-)");
            break;
        }
        case OP_MULT_S: {
            MSG("Multiply(*)");
            break;
        }
        case OP_DIV_S: {
            MSG("Divide(/)");
            break;
        }
        case OP_MODULUS: {
            MSG("Modulus(%)");
            break;
        }

            // Logic
        case OP_NOT: {
            MSG("Not(!)");
            break;
        }
        case OP_EQUAL_S: {
            MSG("Equal(==)");
            break;
        }
        case OP_NOT_EQUAL_S: {
            MSG("NotEqual(!=)");
            break;
        }
        case OP_LESS_S: {
            MSG("Less(<)");
            break;
        }
        case OP_LESS_OR_EQUAL_S: {
            MSG("LessOrEqual(<=)");
            break;
        }
        case OP_GREATER_S: {
            MSG("Greater(>)");
            break;
        }
        case OP_GREATER_OR_EQUAL_S: {
            MSG("GreaterOrEqual(>=)");
            break;
        }

        // Bitwise
        case OP_BIT_NOT: {
            MSG(("BitwiseNOT(~)"));
            break;
        }
        case OP_BIT_AND: {
            MSG(("BitwiseAND(&)"));
            break;
        }
        case OP_BIT_OR: {
            MSG(("BitwiseOR(|)"));
            break;
        }
        case OP_BIT_XOR: {
            MSG(("BitwiseXOR(^)"));
            break;
        }
        case OP_BIT_SHIFT_L: {
            MSG(("BitShiftLeft(<<)"));
            break;
        }
        case OP_BIT_SHIFT_R: {
            MSG(("BitShiftLeft(>>)"));
            break;
        }

            // Assignment
        case OP_ASSIGN: {
            MSG("Assign = ");
            break;
        }

            // Functions
        case OP_JUMP: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Jump: ", offset);
            break;
        }
        case OP_BREAK: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Break: ", offset);
            break;
        }
        case OP_CONTINUE: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Continue: ", offset);
            break;
        }
        case OP_JUMP_IF_FALSE: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Jump If False: ", offset);
            break;
        }
        case OP_JUMP_IF_TRUE: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Jump If True: ", offset);
            break;
        }
        case OP_JUMP_IF_EQUAL: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Jump If Equal: ", offset);
            break;
        }
        case OP_LOOP: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Loop (Jump Back): ", offset);
            break;
        }
        case OP_SWITCH: {
            u32 offset = DBG_READ_UINT16(valPtr);
            DBG_PRINT_VALUE_OP("Switch (Jump Table): ", offset);
            break;
        }
        case OP_FRAME: {
            MSG("FRAME");
            break;
        }
        case OP_CALL: {
            MSG("Call");
            break;
        }
        case OP_CALL_NATIVE: {
            MSG("Call Native");
            break;
        }
        case OP_RETURN: {
            MSG("Return");
            break;
        }
        case OP_END: {
            MSG("END!");
            break;
        }

        default: {
            MSG("Unknown Instruction! [" << std::to_string(op) << "]");
            break;
        }
    }
}

std::string Debugger::PrintValue(const Value &value) {

    return "int: " + std::to_string(value.Int)
            + " | float: " + std::to_string(value.Float)
            + " | pointer: " + std::to_string(value.Pointer.Address);
}
