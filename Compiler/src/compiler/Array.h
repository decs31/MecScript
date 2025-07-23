//
// Created by Declan Walsh on 8/05/2024.
//

#ifndef ARRAY_H
#define ARRAY_H

#include "Instructions.h"
#include "NativeFunctions.h"
#include "Tokens.h"
#include "Value.h"
#include "Variable.h"
#include <string>

struct Array {
    std::string Name;
    int Count = 0;

    int StackSize()
    {
        return 0;
    }
};

#endif // ARRAY_H
