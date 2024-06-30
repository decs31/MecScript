//
// Created by Declan Walsh on 14/03/2024.
//

#ifndef DEBUGGER_H_
#define DEBUGGER_H_


#include <vector>
#include <string>
#include <iostream>
#include "Instructions.h"
#include "Console.h"

namespace Debugger {
    void DebugInstruction(opCode_t *codeStart, opCode_t *code);

    std::string PrintValue(const Value &value);
}

#endif //DEBUGGER_H_
