//
// Created by Declan Walsh on 13/04/2024.
//

#ifndef TYPESYSTEM_H_
#define TYPESYSTEM_H_

#include "Instructions.h"
#include "Value.h"


enum TypeCompatibility {
    tcIncompatible,
    tcCastSignedToUnsigned,
    tcCastSignedToFloat,
    tcCastUnsignedToSigned,
    tcCastUnsignedToFloat,
    tcCastFloatToUnsigned,
    tcCastFloatToSigned,
    tcMatch,
    tcNotApplicable,
};

struct ExpressionTypeSet {
    opCode_t PrefixOp = OP_NOP;
    DataType LhsType  = dtNone;
    opCode_t InfixOp  = OP_NOP;
    DataType RhsType  = dtNone;
};

class TypeInfo
{
  public:
    DataType Type             = dtNone;
    TypeInfo *Enclosing       = nullptr;
    bool IgnoreExpectingOnSet = false;

    TypeInfo() = default;
    explicit TypeInfo(DataType type) : Type(type)
    {
    }

    DataType Expecting() const;
    TypeCompatibility CheckCompatibleWith(DataType other) const;
    int ByteSize() const;

    static TypeCompatibility CheckCompatibility(DataType expecting, DataType input);
    static int GetByteSize(DataType dataType);
    static int GetPackedCount(DataType dataType);
};

#endif // TYPESYSTEM_H_
