#include "Native.h"
#include "Console.h"
#include "Function.h"
#include "NativeFunctions.h"
#include <fstream>

NativeFunctionParser::NativeFunctionParser(ErrorHandler *errorHandler, std::string &script) : CompilerBase(errorHandler, script)
{
    m_ScriptOk = !script.empty();
}

StatusCode NativeFunctionParser::Parse()
{
    if (!m_ScriptOk) {
        // No data. Skipping.
        return stsOk;
    }

    if (m_Lexer.Status() >= errError) {
        // At this point the Lexer error can only be an empty script.
        return errLexError;
    }

    if (m_ErrorHandler == nullptr) {
        return errError;
    }

    m_Status = stsOk;
    MSG("Starting Native Function Parser...");

    m_FunctionMap.clear();

    // Tokenize and parse the script.
    if (m_Lexer.Tokenize() != stsLexEndOfFile) {
        m_Status = errLexError;
        return errLexError;
    }

    // Start by skipping any initial comments
    while (IsSkippable(CurrentToken())) {
        m_CurrentPos++;
    }

    // Churn through the script
    while (!IsAtEnd()) {
        ParseNativeFunction();
    }

    MSG("Parsed " << m_FunctionMap.size() << " native functions");

    m_Status = stsOk;

    return m_Status;
}

std::map<std::string, NativeFuncInfo> &NativeFunctionParser::Functions()
{
    return m_FunctionMap;
}

void NativeFunctionParser::ParseNativeFunction()
{
    // Expect the native function declaration to start with "[native]"
    (void)ConsumeToken(tknLeftSquareBracket, -1, "Expected \"[native]\" annotation.");
    Token tknNative = ConsumeToken(tknIdentifier, -1, "Expected \"[native]\" annotation.");
    if (tknNative.Value != "native") {
        AddError("Expected \"[native]\" annotation.", tknNative);
        return;
    }
    // Expect a function ID
    if (!Check(tknIntegerLiteral)) {
        AddError("Expected function ID after \"[native]\" annotation.", LookBack());
        return;
    }
    Token tknFunctionId = ConsumeToken(tknIntegerLiteral, -1, "Expected function ID after \"[native]\" annotation.");
    int functionId      = -1;
    if (!ScriptUtils::StringToInt(tknFunctionId.Value, functionId)) {
        AddError("Invalid function ID after \"[native]\" annotation.", tknFunctionId);
        return;
    }
    (void)ConsumeToken(tknRightSquareBracket, -1, "Expected \"]\" after \"[native]\" annotation.");

    // Expect the return type to appear next
    DataType returnType = dtVoid;
    u32 flags           = vfNormal;
    if (!MatchTypeDeclaration(returnType, flags)) {
        AddError("Expected return type for native function.", LookBack());
        return;
    }

    // Expect the function name to appear next
    Token tknFunction = ConsumeToken(tknIdentifier, -1, "Expected function name.");

    // Expect the left parenthesis for parameters
    (void)ConsumeToken(tknLeftParen, -1, "Expected \"(\" after function name.");
    // Parse parameters
    std::vector<DataType> params;
    while (!IsAtEnd() && !Check(tknRightParen)) {
        DataType paramType = dtVoid;
        u32 paramFlags     = vfNormal;
        if (!MatchTypeDeclaration(paramType, paramFlags)) {
            AddError("Expected parameter type", LookBack());
            return;
        }

        // If a variable name is provided, consume it
        if (Check(tknIdentifier)) {
            AdvanceToken();
        }

        params.push_back(paramType);

        // If there are more parameters, expect a comma
        if (!Match(tknComma)) {
            break;
        }
    }
    // Expect the right parenthesis to close parameters
    (void)ConsumeToken(tknRightParen, -1, "Expected \")\" after parameters.");

    // Expect the semicolon to end the declaration
    if (!Match(tknSemiColon)) {
        AddError("Expected \";\" to end native function declaration.", LookBack());
        return;
    }

    // Create the NativeFuncInfo and add it to the map
    NativeFuncInfo nativeFunc(functionId, returnType, params);
    nativeFunc.Name                  = tknFunction.Value;
    m_FunctionMap[tknFunction.Value] = nativeFunc;
}