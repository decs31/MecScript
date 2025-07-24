//
// Created by Declan Walsh on 23/04/2024.
//

#ifndef COMPILERDATA_H_
#define COMPILERDATA_H_

#include "Instructions.h"
#include "Value.h"
#include <string>
#include <vector>

struct StringData {
    uint32_t Index;
    uint32_t Length;
    std::string String;
};

struct SwitchInfo {
    // Data type of the input expression
    DataType Type;

    // Index of the argument for the CODE_JUMP_IF instruction used to exit the
    // loop. Stored so we can patch it once we know where the loop ends.
    int ExitJump = NOT_SET;

    // Index of the first instruction of the body of the loop.
    int Body = NOT_SET;

    // Depth of the scope(s) that need to be exited if a break is hit inside the loop.
    int ScopeDepth = NOT_SET;

    // The loop enclosing this one, or NULL if this is the outermost loop.
    SwitchInfo *Enclosing = nullptr;
};

struct LoopInfo {
    // Index of the instruction that the loop should jump back to.
    int Start = NOT_SET;

    // Index of the argument for the CODE_JUMP_IF instruction used to exit the
    // loop. Stored so we can patch it once we know where the loop ends.
    int ExitJump = NOT_SET;

    // Index of the first instruction of the body of the loop.
    int Body = NOT_SET;

    // Depth of the scope(s) that need to be exited if a break is hit inside the loop.
    int ScopeDepth = NOT_SET;

    // The loop enclosing this one, or NULL if this is the outermost loop.
    LoopInfo *Enclosing = nullptr;
};

#endif // COMPILERDATA_H_
