//
// Created by Declan Walsh on 13/04/2024.
//

#include "TypeSystem.h"

TypeCompatibility TypeInfo::CheckCompatibility(DataType expecting, DataType input) {
    if (input == expecting)
        return tcMatch;

    if ((expecting == dtNone) || (input == dtNone))
        return tcNotApplicable;

    if (expecting >= dtBool && expecting <= dtInt32) {
        if (input >= dtBool && input <= dtInt32) return tcMatch;
        if (input == dtUint32) return tcCastUnsignedToSigned;
        if (input == dtFloat) return tcCastFloatToSigned;
    }

    if (expecting == dtUint32) {
        if (input == dtFloat) return tcCastFloatToUnsigned;
        else return tcCastSignedToUnsigned;
    }

    if (expecting == dtFloat) {
        if (input >= dtBool && input <= dtInt32) return tcCastSignedToFloat;
        return tcCastUnsignedToFloat;
    }

    if (expecting == dtPointer) {
        if (input >= dtInt8 && input <= dtInt32) return tcMatch;
        if (input == dtFloat) return tcCastFloatToSigned;
    }

    return tcIncompatible;
}

TypeCompatibility TypeInfo::CheckCompatibleWith(DataType other) const {
    return CheckCompatibility(Type, other);
}

DataType TypeInfo::Expecting() const {
    if (Enclosing != nullptr)
        return Enclosing->Type == dtNone ? Enclosing->Expecting() : Enclosing->Type;

    return Type;
}

int TypeInfo::GetByteSize(DataType dataType) {
    switch (dataType) {
        case dtBool:
        case dtInt8:
        case dtUint8:
            return 1;

        case dtInt16:
        case dtUint16:
            return 2;

        default:
            return 4;
    }
}

int TypeInfo::ByteSize() const {

    return GetByteSize(Type);
}

int TypeInfo::GetPackedCount(DataType dataType) {

    return (int)sizeof(Value) / GetByteSize(dataType);
}
