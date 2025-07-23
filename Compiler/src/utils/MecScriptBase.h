//
// Created by Declan Walsh on 26/02/2024.
//

#ifndef COMPILERBASE_H
#define COMPILERBASE_H

#include "ErrorHandler.h"

class MecScriptBase
{
  public:
    MecScriptBase(ErrorHandler *errorhandler);

  protected:
    ErrorHandler *m_ErrorHandler;
    StatusCode m_Status = stsOk;
    std::string m_Message;
};

#endif // COMPILERBASE_H
