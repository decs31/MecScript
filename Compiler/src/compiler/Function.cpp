//
// Created by Declan Walsh on 25/04/2024.
//

#include "Function.h"

u32 ScriptFunction::TotalLocalsHeight() {
    return 0;
    /*
    if (Enclosing == nullptr)
        return 0;

    return Enclosing->TotalLocalsHeight() + LocalsMaxHeight;
     */
}
