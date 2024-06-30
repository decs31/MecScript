//
// Created by Declan Walsh on 18/03/2024.
//

#ifndef RULES_H
#define RULES_H

#include "Tokens.h"

enum ParseFunc {
    fnNone,
    fnGrouping,
    fnLiteral,
    fnUnary,
    fnBinary,
    fnString,
    fnVariable,
    fnAnd,
    fnOr,
    fnTernary,          // ?
    fnCall,
    fnArrayIndex,
    fnVariablePrefix,   // ++ --
    fnVariablePostfix,  // ++ --
};

enum Precedence {
    precNone = 0,
    precAssignment,     // = += -= *= /=
    precTernary,        // ?
    precOr,             // or
    precAnd,            // and
    precEquality,       // == !=
    precComparison,     // < > <= >=
    precTerm,           // + -
    precFactor,         // * / %
    precUnary,          // ! -- ++
    precCall,           // . [] ()
};

struct ParseRule {
    ParseFunc Prefix = fnNone;
    ParseFunc Infix = fnNone;
    Precedence Prec = precNone;
};

namespace Rules {
    ParseRule Get(TokenType tokenType);
}

#endif //RULES_H
