//
// Created by Declan Walsh on 14/03/2024.
//

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include "Instructions.h"
#include "Value.h"
#include <iostream>
#include <string>


namespace Debugger
{
    void DebugInstruction(const opCode_t *codeStart, const opCode_t *code);

    std::string PrintValue(const Value &value);
}

#endif // DEBUGGER_H_
