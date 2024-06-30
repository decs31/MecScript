//
// Created by Declan Walsh on 8/05/2024.
//

#ifndef ARRAY_H
#define ARRAY_H

#include "Value.h"
#include "NativeFunctions.h"
#include "Instructions.h"
#include "Variable.h"
#include "Tokens.h"

struct Array {
    string Name;
    int Count = 0;

    int StackSize() {
        return 0;
    }
};

#endif //ARRAY_H
