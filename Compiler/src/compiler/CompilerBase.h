#ifndef COMPILER_BASE_H_
#define COMPILER_BASE_H_

#include "Lexer.h"
#include "MecScriptBase.h"
#include "Value.h"
#include "Variable.h"

class CompilerBase : public MecScriptBase
{
  public:
    CompilerBase(ErrorHandler *errorhandler, const std::string &script);
    ~CompilerBase();

  protected:
    Lexer m_Lexer;

    size_t m_CurrentPos  = 0;
    Token m_TokenCurrent = {};
    Token m_TokenPrev    = {};

    /* Tokens */
    bool Expect(const Token &token, TokenType expect, int errorOffset = -2, const std::string &errorMsg = "");
    bool Check(TokenType tokenType);
    bool CheckAhead(TokenType tokenType, int num = 1);
    bool Match(TokenType tokenType);
    bool IsSkippable(const Token &token);

    /* Get the Token at the current position */
    Token CurrentToken();

    /* Get the Token at the current position then advance the position */
    Token ConsumeToken(TokenType expect = tknNone, int errorOffset = -2, const std::string &errorMsg = "");

    /* Get the Token at the current position then advance the position */
    void AdvanceToken(TokenType expect = tknNone, int errorOffset = -2, const std::string &error = "");

    /* Get the Token at the specified position */
    Token TokenAt(size_t pos);

    /* Get the Token at a number of positions ahead of the current position */
    Token LookAhead(int num = 1);

    /* Get the Token at a number of positions behind the current position */
    Token LookBack(int num = 1);

    bool IsAtEnd();

    /* Matching */
    bool MatchTypeDeclaration(DataType &outDataType, u32 &outFlags);

    /* Error Handling */
    bool m_PanicMode = false;
    void AddError(std::string errMsg, const Token &token);
    void AddError(std::string errMsg, size_t lineNum, size_t linePos);
    void AddWarning(std::string warningMsg, const Token &token);
    void AddWarning(std::string waringMsg, size_t lineNum, size_t linePos);
    void Synchronize();
};

#endif // COMPILER_BASE_H_