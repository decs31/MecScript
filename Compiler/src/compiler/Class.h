//
// Created by Declan Walsh on 25/04/2024.
//

#ifndef CLASS_H_
#define CLASS_H_

#include <vector>
#include <string>
#include "Variable.h"

using string = std::string;

struct ClassInfo {
    Token Token;
    string Name;
    int Id = -1;
    int EnclosingId = -1;
    ClassInfo *Enclosing = nullptr;
    int ParentFunctionId = -1;
    std::vector<VariableInfo> Fields;
    std::vector<string> Methods;
    int InitFunctionId = -1;
    int ConstructorFunctionId = -1;

    int Size() const {

        return (int) Fields.size();
    }

    bool HasConstructor() const {

        return ConstructorFunctionId >= 0;
    }
};

#endif //CLASS_H_
