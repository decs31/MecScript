#include "CompilerBase.h"
#include "Console.h"
#include "ScriptUtils.h"

CompilerBase::CompilerBase(ErrorHandler *errorhandler, const std::string &script) : MecScriptBase(errorhandler), m_Lexer(errorhandler, script)
{
}

CompilerBase::~CompilerBase()
{
}

bool CompilerBase::IsAtEnd()
{
    if (CurrentToken().TokenType == tknEndOfFile) {
        MSG_V("End of file token reached.");
        return true;
    }

    if (m_CurrentPos >= m_Lexer.Tokens().size()) {
        MSG_V("End of tokens!");
        return true;
    }

    return false;
}

bool CompilerBase::IsSkippable(const Token &token)
{
    switch (token.TokenType) {
        case tknComment:
        case tknPreProcessor:
        case tknEndLine:
            return true;
        default:
            return false;
    }
}

Token CompilerBase::CurrentToken()
{
    return TokenAt(m_CurrentPos);
}

Token CompilerBase::ConsumeToken(TokenType expect, int errorOffset, const std::string &errorMsg)
{
    AdvanceToken(expect, errorOffset, errorMsg);

    return m_TokenPrev;
}

void CompilerBase::AdvanceToken(TokenType expect, int errorOffset, const std::string &errorMsg)
{
    m_TokenPrev = CurrentToken();

    // Advance
    do {
        m_CurrentPos++;
    } while (IsSkippable(CurrentToken()) && !IsAtEnd());

    m_TokenCurrent = CurrentToken();
    Expect(m_TokenPrev, expect, errorOffset, errorMsg);
}

bool CompilerBase::Expect(const Token &token, TokenType expect, int errorOffset, const std::string &errorMsg)
{
    if ((expect > tknNone) && token.TokenType != expect) {
        size_t pos       = m_CurrentPos + errorOffset;
        Token errorToken = TokenAt(pos);
        while (IsSkippable(errorToken)) {
            if (errorOffset <= 0)
                --pos;
            else
                ++pos;

            errorToken = TokenAt(pos);
        }

        if (!errorMsg.empty()) {
            AddError(errorMsg, errorToken);
        } else if (errorOffset <= 0) {
            AddError("Expected '" + Lexer::TokenTypeToValue(expect) + "' after '" + token.Value + "'.", errorToken);
        } else {
            AddError("Expected '" + Lexer::TokenTypeToValue(expect) + "' before '" + token.Value + "'.", errorToken);
        }

        return false;
    }

    return true;
}

bool CompilerBase::Check(TokenType tokenType)
{
    return CurrentToken().TokenType == tokenType;
}

bool CompilerBase::CheckAhead(TokenType tokenType, int num)
{
    return LookAhead(num).TokenType == tokenType;
}

bool CompilerBase::Match(TokenType tokenType)
{
    if (!Check(tokenType))
        return false;

    AdvanceToken();
    return true;
}

Token CompilerBase::LookAhead(int num)
{
    size_t pos = m_CurrentPos + num;
    return TokenAt(pos);
}

Token CompilerBase::LookBack(int num)
{
    if (m_CurrentPos < (size_t)num) {
        return TokenAt(0);
    }

    size_t pos = m_CurrentPos - num;
    return TokenAt(pos);
}

Token CompilerBase::TokenAt(size_t pos)
{
    if (m_Lexer.Tokens().empty()) {
        return {};
    }

    if (pos >= m_Lexer.Tokens().size())
        return m_Lexer.Tokens().back();

    return m_Lexer.Tokens().at(pos);
}

void CompilerBase::AddError(std::string errMsg, const Token &token)
{
    AddError(std::move(errMsg), token.Position.LineNum, token.Position.LinePos);
}

void CompilerBase::AddError(std::string errMsg, size_t lineNum, size_t linePos)
{
    if (m_PanicMode) // Already in error, don't report more.
        return;

    m_PanicMode = true;
    m_Status    = errPanicSync;

    CompilerMessage msg;
    msg.Source  = csParser;
    msg.Code    = errSyntaxError;
    msg.FilePos = 0;
    msg.LineNum = lineNum;
    msg.LinePos = linePos;
    msg.Message = std::move(errMsg);

    m_ErrorHandler->AddMessage(msg);
}

void CompilerBase::AddWarning(std::string warningMsg, const Token &token)
{
    AddWarning(std::move(warningMsg), token.Position.LineNum, token.Position.LinePos);
}

void CompilerBase::AddWarning(std::string warningMsg, size_t lineNum, size_t linePos)
{
    CompilerMessage msg;
    msg.Source  = csParser;
    msg.Code    = wrnWarning;
    msg.FilePos = 0;
    msg.LineNum = lineNum;
    msg.LinePos = linePos;
    msg.Message = std::move(warningMsg);

    m_ErrorHandler->AddMessage(msg);
}

void CompilerBase::Synchronize()
{
    m_Status    = stsParserHasErrors; // Maybe?
    m_PanicMode = false;

    while (!IsAtEnd()) {
        switch (CurrentToken().TokenType) {
            case tknVoid:
            case tknChar:
            case tknByte:
            case tknShort:
            case tknUShort:
            case tknInt:
            case tknUInt:
            case tknFloat:
            case tknFor:
            case tknIf:
            case tknWhile:
            case tknSwitch:
            case tknReturn:
            case tknClass:
                return;

            default:; // Do nothing.
        }

        AdvanceToken();
    }
}

bool CompilerBase::MatchTypeDeclaration(DataType &outDataType, u32 &outFlags)
{
    outFlags = vfNormal;

    if (Match(tknConst)) {
        outFlags |= vfConst;
    }

    if (Match(tknStar)) {
        outFlags |= vfPointer;
    }

    if (Match(tknVoid)) {
        outDataType = dtVoid;
    } else if (Match(tknBool)) {
        outDataType = dtBool;
    } else if (Match(tknChar)) {
        outDataType = dtInt8;
    } else if (Match(tknByte)) {
        outDataType = dtUint8;
    } else if (Match(tknShort)) {
        outDataType = dtInt16;
    } else if (Match(tknUShort)) {
        outDataType = dtUint16;
    } else if (Match(tknInt)) {
        outDataType = dtInt32;
    } else if (Match(tknUInt)) {
        outDataType = dtUint32;
    } else if (Match(tknFloat)) {
        outDataType = dtFloat;
    } else if (Match(tknString)) {
        outDataType = dtString;
    } else {
        outDataType = dtNone;
        if (outFlags != vfNormal) {
            AddError("Expected type initializer.", LookBack());
        }
    }

    return (outDataType != dtNone);
}