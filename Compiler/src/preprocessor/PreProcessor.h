//
// Created by Declan Walsh on 12/06/2024.
//

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "ScriptUtils.h"
#include "MecScriptBase.h"
#include "OrderedMap.hpp"
#include "Tokens.h"

using string = std::string;

struct Definition {
    string Input;
    string Output;
};

class PreProcessor : public MecScriptBase {
public:
    explicit PreProcessor(ErrorHandler *errorHandler);
    ~PreProcessor();

    StatusCode Run(const std::vector<Token> &tokens);

private:


    OrderedMap<string, Definition> m_Definitions;
};

#endif //PREPROCESSOR_H
