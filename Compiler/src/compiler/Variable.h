//
// Created by Declan Walsh on 25/04/2024.
//

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include "Tokens.h"
#include "Value.h"
#include <string>

enum VariableFlags : u32 {

    vfNormal = 0x00,
    vfArray = 0x01,
    vfClass = 0x02,
    vfFunction = 0x04,
    vfField = 0x08,
    vfPointer = 0x10,
    vfConst = 0x20,
};

struct ConstantInfo {
    DataType Type;
    Value ConstValue;

    bool operator<(const ConstantInfo &other) const
    {
        if (Type == dtFloat) {
            if (other.Type == dtFloat)
                return ConstValue.Float < other.ConstValue.Float;
            if (other.Type == dtInt8)
                return ConstValue.Float < (float)other.ConstValue.Char;
            if (other.Type == dtInt16)
                return ConstValue.Float < (float)other.ConstValue.Short;
            if (other.Type == dtInt32)
                return ConstValue.Float < (float)other.ConstValue.Int;

            return ConstValue.Float < (float)other.ConstValue.UInt;
        }

        if (Type == dtInt8) {
            if (other.Type == dtInt8)
                return ConstValue.Char < other.ConstValue.Char;
            if (other.Type == dtInt16)
                return ConstValue.Char < (char)other.ConstValue.Short;
            if (other.Type == dtInt32)
                return ConstValue.Char < (char)other.ConstValue.Int;
            if (other.Type == dtFloat)
                return ConstValue.Char < (char)other.ConstValue.Float;

            return ConstValue.Char < (char)other.ConstValue.UInt;
        }

        if (Type == dtInt16) {
            if (other.Type == dtInt16)
                return ConstValue.Short < other.ConstValue.Short;
            if (other.Type == dtInt8)
                return ConstValue.Short < (s16)other.ConstValue.Short;
            if (other.Type == dtInt32)
                return ConstValue.Short < (s16)other.ConstValue.Int;
            if (other.Type == dtFloat)
                return ConstValue.Short < (s16)other.ConstValue.Float;

            return ConstValue.Short < (s16)other.ConstValue.UInt;
        }

        if (Type == dtInt32) {
            if (other.Type == dtInt32)
                return ConstValue.Int < other.ConstValue.Int;
            if (other.Type == dtInt8)
                return ConstValue.Int < (s32)other.ConstValue.Char;
            if (other.Type == dtInt16)
                return ConstValue.Int < (s32)other.ConstValue.Short;
            if (other.Type == dtFloat)
                return ConstValue.Int < (s32)other.ConstValue.Float;

            return ConstValue.Int < (s32)other.ConstValue.UInt;
        }

        { // Unsigned
            if (other.Type == dtInt8)
                return ConstValue.UInt < (u32)other.ConstValue.Char;
            if (other.Type == dtInt16)
                return ConstValue.UInt < (u32)other.ConstValue.Short;
            if (other.Type == dtInt32)
                return ConstValue.UInt < (u32)other.ConstValue.Int;
            if (other.Type == dtFloat)
                return ConstValue.UInt < (u32)other.ConstValue.Float;

            return ConstValue.UInt < (u32)other.ConstValue.UInt;
        }

        return false;
    }

    bool operator==(const ConstantInfo &other) const
    {
        if (Type == dtFloat && other.Type == dtFloat) {
            return ConstValue.Float == other.ConstValue.Float;
        } else if (Type == dtFloat) {
            return ConstValue.Float == (float)other.ConstValue.Int;
        }

        return ConstValue.UInt == other.ConstValue.UInt;
    }
};

struct VariableInfo {
    std::string Name;
    Token Token;
    std::string ParentClass;
    std::string ParentInstance;
    VmPointer Pointer;
    u32 Flags; // VariableFlags
    int ParentAddress = 0;
    int MemberIndex = 0;
    int MemberDepth = 0;
    int Depth = NOT_SET;
    int Reads = 0;
    int Writes = 0;
    int Size = 1;

    DataType Type() const
    {
        return Pointer.Type;
    }

    bool IsFunction() const
    {
        return Flags & vfFunction;
    }

    bool IsArray() const
    {
        return Flags & vfArray;
    }

    bool IsPointer() const
    {
        return Flags & vfPointer;
    }

    bool IsConst() const
    {
        return Flags & vfConst;
    }

    bool IsField() const
    {
        return !ParentClass.empty() || (Flags & vfField);
    }

    int Address() const
    {
        return Pointer.Address;
    }

    VarScopeType Scope() const
    {
        return Pointer.Scope;
    }

    bool IsHeadMemberOf(const std::string &instance) const;

    bool IsClassHead() const;

    bool Match(const std::string &name, const std::string &parent);
};

#endif // VARIABLE_H_
