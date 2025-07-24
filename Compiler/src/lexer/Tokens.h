//
// Created by Declan Walsh on 18/03/2024.
//

#ifndef TOKENS_H
#define TOKENS_H

#include <cstddef>
#include <cstdint>
#include <string>

enum TokenType {
    tknNone,

    // Comments
    tknComment,

    // Preprocessor
    tknPreProcessor,
    tknDefine,
    tknImport,

    // Identifiers
    tknIdentifier,
    tknString,

    // Constants
    tknConst,
    tknIntegerLiteral,
    tknFloatLiteral,
    tknFalse,
    tknTrue,
    tknStringLiteral,

    // Types
    tknVoid,
    tknBool,
    tknChar,
    tknByte,
    tknShort,
    tknUShort,
    tknInt,
    tknUInt,
    tknFloat,

    // Operators
    tknOperator,
    tknAssign,
    tknMinus,
    tknExclamation,
    tknBitwiseAnd,
    tknBitwiseOr,
    tknBitwiseXor,
    tknBitwiseNot,
    tknShiftLeft,
    tknShiftRight,
    tknLessThan,
    tknGreaterThan,
    tknEquals,
    tknNotEqual,
    tknLessEqual,
    tknGreaterEqual,
    tknPlus,
    tknStar,
    tknSlash,
    tknPercent,
    tknPlusPlus,
    tknMinusMinus,
    tknPlusEquals,
    tknMinusEquals,
    tknTimesEquals,
    tknDivideEquals,
    tknBitwiseAndEquals,
    tknBitwiseOrEquals,
    tknBitwiseXorEquals,
    tknAnd,
    tknOr,
    tknQuestionMark,
    tknColon,
    tknArrow,

    // Braces
    tknBlock,
    tknSingleQuote,
    tknDoubleQuote,
    tknLeftParen,
    tknRightParen,
    tknLeftCurly,
    tknRightCurly,
    tknLeftSquareBracket,
    tknRightSquareBracket,
    tknLeftAngleBracket,
    tknRightAngleBracket,

    // Keywords
    tknNull,
    tknClass,
    tknThis,
    tknBase,
    tknIf,
    tknElse,
    tknWhile,
    tknFor,
    tknBreak,
    tknContinue,
    tknSwitch,
    tknCase,
    tknDefault,
    tknReturn,

    // Punctuation
    tknColonColon,
    tknDot,
    tknComma,
    tknSemiColon,
    tknEndLine,
    tknEndOfFile,
};

struct TokenPosition {
    size_t LineNum;
    size_t LinePos;
};

struct Token {
    TokenType TokenType = tknNone;
    TokenPosition Position;
    std::string Value;
};

enum OperatorType {
    opNone,

    // Binary
    opAdd,
    opSubtract,
    opMultiply,
    opDivide,
    opModulus,

    // Logic
    opLogicalAnd,
    opLogicalOr,

    // Unary
    opUnaryIncrement,
    opUnaryDecrement,
};

struct ExprOp {
    OperatorType Type;
    std::string String;
};

#endif // TOKENS_H
