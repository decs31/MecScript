//
// Created by Declan Walsh on 12/02/2024.
//

#ifndef COMPILER_UTILS_H_
#define COMPILER_UTILS_H_

#include "BasicTypes.h"
#include <iostream>
#include <string>

namespace ScriptUtils
{
    bool StringToFloat(const std::string &str, float &outFloat);
    bool StringToInt(const std::string &str, int &outInt, int base = 10);

    int ParseInteger(const std::string &str);

    uint32_t AlignTo(uint32_t value, uint32_t pos);
}

#endif // COMPILER_UTILS_H_
