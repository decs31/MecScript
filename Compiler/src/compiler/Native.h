#ifndef NATIVE_H
#define NATIVE_H

#include "BasicTypes.h"
#include "CompilerBase.h"
#include "Function.h"
#include "Lexer.h"
#include "Options.h"
#include <filesystem>
#include <map>
#include <string>

class NativeFunctionParser : public CompilerBase
{
  public:
    NativeFunctionParser(ErrorHandler *errorHandler, std::string &script);

    StatusCode Parse();

    const std::map<std::string, NativeFuncInfo> &Functions() const;

  private:
    std::map<std::string, NativeFuncInfo> m_FunctionMap;

    bool m_ScriptOk;

    void ParseNativeFunction();
};

#endif // NATIVE_H