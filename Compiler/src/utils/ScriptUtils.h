//
// Created by Declan Walsh on 12/02/2024.
//

#ifndef COMPILER_UTILS_H_
#define COMPILER_UTILS_H_

#include "BasicTypes.h"
#include "Options.h"
#include <iostream>
#include <string>

#if (VERBOSE_OUTPUT == 1)
#define MSG_V(msg)                          \
    if (ScriptUtils::VerboseOutput == true) \
    std::cout << msg << std::endl
#else
#define MSG_V(msg)
#endif

namespace ScriptUtils
{
    inline bool VerboseOutput = false;

    bool StringToFloat(const std::string &str, float &outFloat);
    bool StringToInt(const std::string &str, int &outInt, int base = 10);

    int ParseInteger(const std::string &str);

    uint32_t AlignTo(uint32_t value, uint32_t pos);
}

#endif // COMPILER_UTILS_H_
