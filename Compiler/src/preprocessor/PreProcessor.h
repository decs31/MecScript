//
// Created by Declan Walsh on 12/06/2024.
//

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "MecScriptBase.h"
#include "OrderedMap.hpp"
#include "ScriptUtils.h"
#include "Tokens.h"

struct Definition {
    std::string Input;
    std::string Output;
};

class PreProcessor : public MecScriptBase
{
  public:
    explicit PreProcessor(ErrorHandler *errorHandler);
    ~PreProcessor();

    StatusCode Run(const std::vector<Token> &tokens);

  private:
    OrderedMap<std::string, Definition> m_Definitions;
};

#endif // PREPROCESSOR_H
