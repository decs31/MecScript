//
// Created by Declan Walsh on 23/04/2024.
//

#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <vector>
#include "Value.h"
#include "NativeFunctions.h"
#include "Instructions.h"
#include "Variable.h"
#include "Tokens.h"

class FunctionInfo {
public:
    string Name;
    string ParentClass;
    Token Token;
    FunctionType Type = ftFunction;
    DataType ReturnType = dtVoid;
    std::vector<DataType> Args;
    bool IsParameterless = false;

    FunctionInfo() = default;

    FunctionInfo(FunctionType functionType, DataType returnType)
            : Type(functionType), ReturnType(returnType) {}

    FunctionInfo(FunctionType functionType, DataType returnType, std::vector<DataType> &args)
            : Type(functionType), ReturnType(returnType), Args(args) {}

    string Key() {

        return ParentClass.empty() ? Name : ParentClass + "_" + Name;
    }

    int TotalArgCount() const { return (int) Args.size(); }

    int ArgCount() const { return TotalArgCount() - (Type == ftClassMethod ? 1 : 0); }

    int MaxArgs() const { return Type == ftClassMethod ? 254 : 255; }
};

class ScriptFunction : public FunctionInfo {
public:
    int Id = -1;
    //int EnclosingId = -1;
    ScriptFunction *Enclosing = nullptr;
    std::vector<opCode_t> Code;
    std::vector<VariableInfo *> Locals;
    u32 LocalsMaxHeight = 0;
    int ConditionalDepth = 0;
    bool ReturnSupplied = false;

    ScriptFunction(FunctionType type, int id);
    ~ScriptFunction();

    u32 TotalLocalsHeight();
};

struct NativeFuncInfo : public FunctionInfo {
    NativeFuncId Id = nfNull;

    NativeFuncInfo() = default;

    NativeFuncInfo(NativeFuncId id, DataType returnType)
            : FunctionInfo(ftNative, returnType), Id(id) {}

    NativeFuncInfo(NativeFuncId id, DataType returnType, std::vector<DataType> &args)
            : FunctionInfo(ftNative, returnType, args), Id(id) {}

    bool operator==(const NativeFuncInfo &other) const {

        return Id == other.Id;
    }

    static NativeFuncInfo Null() {

        return {};
    }
};

#define NULL_NATIVE     NativeFuncInfo::Null()

#endif //FUNCTION_H_
