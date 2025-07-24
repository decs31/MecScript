//
// Created by Declan Walsh on 12/06/2024.
//

#include "PreProcessor.h"

PreProcessor::PreProcessor(ErrorHandler *errorHandler) : MecScriptBase(errorHandler)
{
}

PreProcessor::~PreProcessor()
{
}

StatusCode PreProcessor::Run(const std::vector<Token> &tokens)
{
    return stsOk;
}
