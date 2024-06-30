//
// Created by Declan Walsh on 1/06/2024.
//

#ifndef MECSCRIPT_CONSOLE_H
#define MECSCRIPT_CONSOLE_H

#include <iostream>

#define MSG(msg)                std::cout << msg << std::endl
#define ERR(msg)                std::cerr << "Error: " << msg << std::endl
#define PRINT(msg)              std::cout << msg;

#endif //MECSCRIPT_CONSOLE_H
