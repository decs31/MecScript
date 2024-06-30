//
// Created by Declan Walsh on 18/03/2024.
//

#include "Rules.h"


ParseRule Rules::Get(TokenType tokenType) {

    switch (tokenType) {
        case tknRightParen:
        case tknLeftCurly:
        case tknRightCurly:
            return {fnNone, fnNone, precNone};

        case tknFalse:
        case tknTrue:
        case tknNull:
        case tknIntegerLiteral:
        case tknFloatLiteral:
            return {fnLiteral, fnNone, precNone};

        case tknStringLiteral:
            return {fnString, fnNone, precNone};

        case tknLeftParen:
            return {fnGrouping, fnCall, precCall};

        case tknLeftSquareBracket:
            return {fnNone, fnArrayIndex, precCall};

        case tknMinus:
            return {fnUnary, fnBinary, precTerm};

        case tknPlus:
            return {fnNone, fnBinary, precTerm};

        case tknPlusPlus:
        case tknMinusMinus:
            return {fnVariablePrefix, fnVariablePostfix, precUnary};

        case tknExclamation:
            return {fnUnary, fnNone, precUnary};

        case tknSlash:
        case tknStar:
        case tknPercent:
            return {fnNone, fnBinary, precFactor};

        case tknEquals:
        case tknNotEqual:
            return {fnNone, fnBinary, precEquality};

        case tknLessThan:
        case tknLessEqual:
        case tknGreaterThan:
        case tknGreaterEqual:
            return {fnNone, fnBinary, precComparison};

        case tknIdentifier:
            return {fnVariable, fnNone, precNone};

        case tknAnd:
            return {fnNone, fnAnd, precAnd};
        case tknOr:
            return {fnNone, fnOr, precOr};

        case tknBitwiseNot:
            return {fnUnary, fnNone, precUnary};

        case tknBitwiseAnd:
        case tknBitwiseOr:
        case tknBitwiseXor:
        case tknShiftLeft:
        case tknShiftRight:
            return {fnNone, fnBinary, precTerm};

        case tknQuestionMark:
            return {fnNone, fnTernary, precTernary};

        default:
            return {fnNone, fnNone, precNone};
    }
}