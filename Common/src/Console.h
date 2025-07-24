//
// Created by Declan Walsh on 1/06/2024.
//

#ifndef MECSCRIPT_CONSOLE_H
#define MECSCRIPT_CONSOLE_H

#include "Options.h"
#include <iostream>

#if (VERBOSE_OUTPUT == 1)
#define MSG_V(msg)                          \
    if (Console::VerboseOutput == true) \
    std::cout << msg << std::endl
#else
#define MSG_V(msg)
#endif

#define MSG(msg)   std::cout << msg << std::endl
#define ERR(msg)   std::cerr << "Error: " << msg << std::endl
#define PRINT(msg) std::cout << msg;

namespace Console
{
    inline bool VerboseOutput = false;
}

#endif // MECSCRIPT_CONSOLE_H
