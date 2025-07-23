//
// Created by Declan Walsh on 16/02/2024.
//

#include "ScriptUtils.h"

bool ScriptUtils::StringToFloat(const std::string &str, float &outFloat)
{
    char *end;
    float floatVal = std::strtof(str.c_str(), &end);
    if (*end != 0) { // Error!
        outFloat = 0.0f;
        return false;
    }

    outFloat = floatVal;
    return true;
}

bool ScriptUtils::StringToInt(const std::string &str, int &outInt, int base)
{
    char *end;
    int intVal = std::strtol(str.c_str(), &end, base);
    if (*end != 0) { // Error!
        outInt = 0;
        return false;
    }

    outInt = intVal;
    return true;
}

int ScriptUtils::ParseInteger(const std::string &intString)
{
    int iVal;
    std::string str;
    int base;
    if (intString.starts_with("0b")) { // Binary
        str  = intString.substr(2);
        base = 2;
    } else if (intString.starts_with("0o")) { // Octal
        str  = intString.substr(2);
        base = 8;
    } else { // Decimal or Hex
        str  = intString;
        base = 0; // Determined from string
    }

    if (ScriptUtils::StringToInt(str, iVal, base)) {
        return iVal;
    } else {
        return 0;
    }
}

uint32_t ScriptUtils::AlignTo(uint32_t value, uint32_t offset)
{
    uint32_t aligned = value;
    while (aligned % offset > 0) {
        ++aligned;
    }

    return aligned;
}