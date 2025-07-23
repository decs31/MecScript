#include "Lexer.h"
#include "Console.h"
#include "ScriptUtils.h"
#include <iostream>
#include <map>
#include <utility>

static std::map<std::string, TokenType> TokenMap = {
    {         "",               tknNone },

    // Braces
    {        "'",        tknSingleQuote },
    {       "\"",        tknDoubleQuote },
    {        "(",          tknLeftParen },
    {        ")",         tknRightParen },
    {        "{",          tknLeftCurly },
    {        "}",         tknRightCurly },
    {        "[",  tknLeftSquareBracket },
    {        "]", tknRightSquareBracket },

    // Punctuation
    {        ",",              tknComma },
    {        ".",                tknDot },
    {        ";",          tknSemiColon },
    {        ":",              tknColon },
    {        "?",       tknQuestionMark },
    {       "::",         tknColonColon },

    // Operators
    {        "=",             tknAssign },
    {        "-",              tknMinus },
    {        "!",        tknExclamation },
    {        "+",               tknPlus },
    {        "*",               tknStar },
    {        "/",              tknSlash },
    {        "%",            tknPercent },
    {        "<",           tknLessThan },
    {        ">",        tknGreaterThan },
    {        "&",         tknBitwiseAnd },
    {        "|",          tknBitwiseOr },
    {        "^",         tknBitwiseXor },
    {        "~",         tknBitwiseNot },
    {       "<<",          tknShiftLeft },
    {       ">>",         tknShiftRight },
    {       "==",             tknEquals },
    {       "!=",           tknNotEqual },
    {       "<=",          tknLessEqual },
    {       ">=",       tknGreaterEqual },
    {       "&&",                tknAnd },
    {       "||",                 tknOr },
    {       "+=",         tknPlusEquals },
    {       "-=",        tknMinusEquals },
    {       "*=",        tknTimesEquals },
    {       "/=",       tknDivideEquals },
    {       "&=",   tknBitwiseAndEquals },
    {       "|=",    tknBitwiseOrEquals },
    {       "^=",   tknBitwiseXorEquals },
    {       "++",           tknPlusPlus },
    {       "--",         tknMinusMinus },
    {       "->",              tknArrow },

    // Types
    {     "void",               tknVoid },
    {     "bool",               tknBool },
    {     "char",               tknChar },
    {     "byte",               tknByte },
    {    "short",              tknShort },
    {   "ushort",             tknUShort },
    {      "int",                tknInt },
    {     "uint",               tknUInt },
    {    "float",              tknFloat },
    {   "string",             tknString },

    // Keywords
    {     "null",               tknNull },
    {     "NULL",               tknNull },
    {      "nil",               tknNull },
    {    "const",              tknConst },
    {    "false",              tknFalse },
    {     "true",               tknTrue },
    {    "class",              tknClass },
    {     "this",               tknThis },
    {     "base",               tknBase },
    {       "if",                 tknIf },
    {     "else",               tknElse },
    {    "while",              tknWhile },
    {      "for",                tknFor },
    {   "return",             tknReturn },
    {    "break",              tknBreak },
    { "continue",           tknContinue },
    {   "switch",             tknSwitch },
    {     "case",               tknCase },
    {  "default",            tknDefault },
};

Lexer::Lexer(ErrorHandler *errorHandler, const std::string &script) : MecScriptBase(errorHandler)
{
    m_Script = script;
    if (m_Script.empty()) {
        AddError("Script is empty.");
    }
}

Lexer::~Lexer() = default;

size_t Lexer::ScriptLength() const
{
    return m_Script.length();
}

StatusCode Lexer::Tokenize()
{
    if (ErrorHandler::IsError(m_Status)) {
        return m_Status;
    }

    MSG("Lexical analysis begin...");
    m_Status = stsOk;

    while (m_Pos <= m_Script.length()) {
        m_Status = ProcessNextToken();
    }

    if (m_ErrorHandler->ErrorCount() == 0)
        return SetResult(stsLexEndOfFile, std::to_string(m_Tokens.size()) + " Tokens");
    else
        return SetResult(wrnLexEndOfFileWithErrors, std::to_string(m_ErrorHandler->ErrorCount()) + " Errors");
}

StatusCode Lexer::ProcessNextToken()
{
    // Create Token
    Token token;
    token.Position = { LineNum(), LinePos() };
    size_t i       = 0;

    // Read the first char
    char c;
    char p;
    GetNextChars(c, p);

    // End of file
    if (m_Pos > m_Script.length()) {
        token.TokenType = tknEndOfFile;
        token.Value     = "END_FILE";
    }

    // Comment
    else if (c == '/' && (p == '/' || p == '*')) {
        token.TokenType = tknComment;
        token.Value += c;
        std::string esc = p == '*' ? "*/" : "\n";
        while (!token.Value.ends_with(esc)) {
            if (!GetNextChars(c, p)) {
                AddError("Comment end not reached. Possibly missing '*/' token.");
                break; // End of file reached
            }
            token.Value += c;
            if (esc != "\n" && c == '\n') {
                ++m_LineNum;
                m_LineStart = m_Pos;
            }
        }

        // Deal with end lines
        if (esc == "\n") {
            // Remove the new line char
            token.Value = token.Value.substr(0, token.Value.length() - 1);
            ++m_LineNum;
            m_LineStart = m_Pos;
        } else if (IsEndLine(p)) {
            GetNextChars(c, p);
            ++m_LineNum;
            m_LineStart = m_Pos;
        }
    }

    // Preprocessor
    else if (c == '#') {
        token.TokenType = tknPreProcessor;
        token.Value += c;

        while (!IsEndLine(p)) {
            GetNextChars(c, p);
            token.Value += c;
            ++i;
        }
    }

    // String Literal
    else if (c == '\"') {
        token.TokenType = tknStringLiteral;
        do {
            GetNextChars(c, p);
            if (c != '\"') {
                token.Value += c;
            }
            ++i;
        } while (c != '\"');
    }

    // Identifier
    else if (IsIdentifier(c, i)) {
        token.TokenType = tknIdentifier;
        token.Value += c;
        while (IsIdentifier(p, i + 1)) {
            GetNextChars(c, p);
            token.Value += c;
            ++i;
        }
    }

    // Operator
    else if (IsOperator(c)) {
        token.TokenType = tknOperator;
        token.Value += c;
        while (IsOperator(p)) {
            GetNextChars(c, p);
            token.Value += c;
            ++i;
        }
    }

    // Number
    else if (IsNumber(c, i)) {
        bool isFloat = false;
        token.Value += c;
        while (IsNumber(p, i + 1)) {
            if (p == '.') {
                if (isFloat) {
                    AddError("Numbers cannot have more than one decimal character.");
                }
                isFloat = true;
            }
            GetNextChars(c, p);
            token.Value += c;
            ++i;
        }
        if (std::isalpha(p)) {
            AddError("Number format error.");
        }
        token.TokenType = isFloat ? tknFloatLiteral : tknIntegerLiteral;
    }

    // Block
    else if (IsBlock(c)) {
        token.TokenType = tknBlock;
        token.Value += c;
    }

    // End ;
    else if (IsSemiColon(c)) {
        token.Value += c;
        token.TokenType = tknSemiColon;
    }

    // End Line
    else if (IsEndLine(c)) {
        token.TokenType = tknEndLine;
        token.Value     = "END_LINE";
        m_LineNum++;
        m_LineStart = m_Pos;
    }

    // Skip white space
    while (m_Pos < m_Script.length() && IsSpace(m_Script.at(m_Pos))) {
        ++m_Pos;
    }

    // Add Token
    if (!token.Value.empty()) {
        // Narrow down the token type
        ImproveTokenType(token);

        m_Tokens.push_back(token);

        MSG_V(
            "[" + std::to_string(token.Position.LineNum) + ":" + std::to_string(token.Position.LinePos) + "]Token<" + std::to_string(token.TokenType) + ">: \""
            << token.Value << "\"");

        if (token.TokenType == tknEndOfFile)
            return SetResult(stsLexEndOfFile, "End of file reached.");
    }

    return stsOk;
}

Token Lexer::CurrentToken() const
{
    if (m_Tokens.empty())
        return {};

    return m_Tokens.back();
}

bool Lexer::GetNextChars(char &c, char &peek)
{
    if (m_Pos < m_Script.length()) {
        c = m_Script.at(m_Pos);
    } else {
        c = '\0';
        m_Pos++;
        return false;
    }

    if (m_Pos < m_Script.length() - 1) {
        peek = m_Script.at(m_Pos + 1);
    } else {
        peek = '\0';
    }

    m_Pos++;
    return true;
}

bool Lexer::IsIdentifier(char c, size_t tokenPos)
{
#if (USE_SWITCH_RANGES == 1)
    // Doesn't work with MSVC
    switch (c) {
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '_':
            return true;

        case '0' ... '9':
            return (tokenPos > 0);

        default:
            return false;
    }
#else
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;
    if (c == '_')
        return true;
    if (c >= '0' && c <= '9')
        return (tokenPos > 0);

    return false;
#endif
}

bool Lexer::IsOperator(char c)
{
    switch (c) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '!':
        case '=':
        case '&':
        case '#':
        case ',':
        case '%':
        case '^':
        case '~':
        case '|':
        case ':':
        case '?':
        case '.':
        case '<':
        case '>':
            return true;

        default:
            return false;
    }
}

bool Lexer::IsNumber(char c, size_t tokenPos)
{
    switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return true;

        case 'x': // Hex "0xFF"
        case 'o': // Octal "0o377"
            return (tokenPos == 1);

        case 'b': // Binary "0b11111111" or Hex
            return (tokenPos >= 1);

            // Hex Values
        case 'a':
            // 'b' Handled above
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return (tokenPos >= 2);

        case '.':
            return (tokenPos > 0);

        default:
            return false;
    }
}

bool Lexer::IsBlock(char c)
{
    switch (c) {
        case '(':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
        case '"':
        case '<':
        case '>':
        case '\'':
            return true;

        default:
            return false;
    }
}

bool Lexer::IsSemiColon(char c)
{
    switch (c) {
        case ';':
            return true;

        default:
            return false;
    }
}

bool Lexer::IsEndLine(char c)
{
    switch (c) {
        case '\n':
        case '\r':
            return true;

        default:
            return false;
    }
}

bool Lexer::IsSpace(char c)
{
    switch (c) {
        case ' ':
        case '\t':
        case '\v':
        case '\f':
            return true;

        default:
            return false;
    }
}

void Lexer::ImproveTokenType(Token &token)
{
    if (token.Value.empty())
        return;

    auto tk = TokenMap.find(token.Value);
    if (tk != TokenMap.end())
        token.TokenType = tk->second;
}

const std::vector<Token> &Lexer::Tokens() const
{
    return m_Tokens;
}

std::string Lexer::CurrentLine()
{
    std::string line;
    size_t i = m_LineStart;
    while (i < m_Script.length()) {
        char c = m_Script.at(i);
        line += c;
        i++;
        if (IsEndLine(c))
            break;
    }

    return line;
}

StatusCode Lexer::SetResult(StatusCode status, std::string message)
{
    m_Status   = status;
    m_ErrorMsg = std::move(message);
    return m_Status;
}

StatusCode Lexer::Status() const
{
    return m_Status;
}

std::string Lexer::Message()
{
    return m_ErrorMsg;
}

size_t Lexer::LineNum() const
{
    return m_LineNum;
}

size_t Lexer::LinePos() const
{
    return m_Pos - m_LineStart + 1;
}

std::string Lexer::TokenTypeToValue(TokenType tokenType)
{
    switch (tokenType) {
        case tknSingleQuote:
            return "'";
        case tknDoubleQuote:
            return "\"";
        case tknLeftParen:
            return "(";
        case tknRightParen:
            return ")";
        case tknLeftCurly:
            return "{";
        case tknRightCurly:
            return "}";
        case tknLeftSquareBracket:
            return "[";
        case tknRightSquareBracket:
            return "]";
        case tknLeftAngleBracket:
            return "<";
        case tknRightAngleBracket:
            return ">";
        case tknComma:
            return ",";
        case tknSemiColon:
            return ";";

        default:
            return std::to_string(tokenType);
    }
}

void Lexer::AddError(std::string message)
{
    CompilerMessage msg;
    msg.Source  = csLexer;
    msg.Code    = errLexError;
    msg.FilePos = m_Pos;
    msg.LineNum = LineNum();
    msg.LinePos = LinePos();
    msg.Message = std::move(message);

    m_ErrorHandler->AddMessage(msg);
}
