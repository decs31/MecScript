#ifndef LEXER_H_
#define LEXER_H_

#include "ErrorHandler.h"
#include "MecScriptBase.h"
#include "Tokens.h"
#include <string>
#include <vector>

#define USE_SWITCH_RANGES

using string = std::string;

class Lexer : public MecScriptBase
{
public:
	explicit Lexer(ErrorHandler *errorHandler, const string &script);
	~Lexer();

    StatusCode Tokenize();
    const std::vector<Token> &Tokens() const;
    [[nodiscard]] StatusCode Status() const;
    [[nodiscard]] size_t ScriptLength() const;
    string Message();
    [[nodiscard]] size_t LineNum() const;
    [[nodiscard]] size_t LinePos() const;
    string CurrentLine();

    static string TokenTypeToValue(TokenType tokenType);

private:

    std::string_view m_Script;
    size_t m_Pos = 0;
    size_t m_LineNum = 1;
    size_t m_LineStart = 0;
    std::vector<Token> m_Tokens;

    string m_ErrorMsg;

    StatusCode ProcessNextToken();
    [[nodiscard]] Token CurrentToken() const;

    /* Reads the next char to c, and one ahead to peek. Advances m_Pos. */
    bool GetNextChars(char &c, char &peek);

    static bool IsIdentifier(char c, size_t tokenPos);
    static bool IsOperator(char c);
    static bool IsNumber(char c, size_t tokenPos);
    static bool IsBlock(char c);
    static bool IsSemiColon(char c);
    static bool IsEndLine(char c);
    static bool IsSpace(char c);

    static void ImproveTokenType(Token &token);

    void AddError(string message);

    StatusCode SetResult(StatusCode status, string message);
};

#endif // !LEXER_H_
