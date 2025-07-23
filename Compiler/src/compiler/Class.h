//
// Created by Declan Walsh on 25/04/2024.
//

#ifndef CLASS_H_
#define CLASS_H_

#include "Variable.h"
#include <string>
#include <vector>

struct ClassInfo {
    Token Token;
    std::string Name;
    int Id               = -1;
    ClassInfo *Enclosing = nullptr;
    int ParentFunctionId = -1;
    std::vector<VariableInfo *> Fields;
    std::vector<std::string> Methods;
    int InitFunctionId        = -1;
    int ConstructorFunctionId = -1;

    int Size() const
    {
        return (int)Fields.size();
    }

    bool HasConstructor() const
    {
        return ConstructorFunctionId >= 0;
    }
};

#endif // CLASS_H_
