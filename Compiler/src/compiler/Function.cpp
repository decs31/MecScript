//
// Created by Declan Walsh on 25/04/2024.
//

#include "Function.h"

ScriptFunction::ScriptFunction(FunctionType type, int id)
        : FunctionInfo(type, dtNone), Id(id)
        {

}

ScriptFunction::~ScriptFunction() {
    for (auto var : Locals) {
        delete(var);
    }
    Locals.clear();
}

u32 ScriptFunction::TotalLocalsHeight() {
    return 0;
    /*
    if (Enclosing == nullptr)
        return 0;

    return Enclosing->TotalLocalsHeight() + LocalsMaxHeight;
     */
}
