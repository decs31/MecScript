//
// Created by Declan Walsh on 13/03/2024.
//

#include "Compiler.h"
#include <fstream>
#include <map>
#include "Options.h"
#include "MathUtils.h"
#include "Console.h"
#include "JumpTable.hpp"
#include "Checksum.h"
#include "Disassembler.h"
#include "ScriptInfo.h"

#define CURRENT_TOKEN_POS       m_CurrentPos
#define CURRENT_CODE_POS        (int)CurrentFunction()->Code.size()
#define CURRENT_SCOPE           m_ScopeDepth

static std::map<string, NativeFuncInfo> NativeFunctionMap = {
        {"print",  NativeFuncInfo(nfPrint, dtVoid, std::vector<DataType>() = {dtString})},
        {"println",  NativeFuncInfo(nfPrintLn, dtVoid, std::vector<DataType>() = {dtString})},
        {"printi", NativeFuncInfo(nfPrintI, dtVoid, std::vector<DataType>() = {dtInt32})},
        {"printf", NativeFuncInfo(nfPrintF, dtVoid, std::vector<DataType>() = {dtFloat})},
        {"clock",  NativeFuncInfo(nfClock, dtInt32)}
};

Compiler::Compiler(ErrorHandler *errorHandler, const std::string &script)
        : MecScriptBase(errorHandler),
          m_Lexer(errorHandler, script),
          m_PreProcessor(errorHandler) {
    //
}

Compiler::~Compiler() {

    Cleanup();
}

VarScopeType Compiler::CurrentScope() const {

    switch (m_ScopeDepth) {
        case 0:
            return scopeGlobal;
        default:
            return scopeLocal;
    }
}

ScriptFunction *Compiler::CreateFunction(const string &name, FunctionType type, DataType returnType) {

    // In case the vector gets reallocated, we need to set the pointers by finding them again.
    int id = (int) m_Functions.size();
    auto newFunc = new ScriptFunction(type, id);
    newFunc->Enclosing = m_CurrentFunction;
    newFunc->Name = name;
    newFunc->ReturnType = returnType;
    newFunc->ParentClass = CurrentClass() ? CurrentClass()->Name : "";
    newFunc->Token = LookBack();

    m_Functions.push_back(newFunc);
    m_CurrentFunction = newFunc;

    return m_CurrentFunction;
}

int Compiler::EndFunction() {

    int completedId = m_CurrentFunction->Id;

    if (m_CurrentFunction->TotalLocalsHeight() > m_LocalsMax)
        m_LocalsMax = m_CurrentFunction->TotalLocalsHeight();

    m_CurrentFunction = m_CurrentFunction->Enclosing;

    return completedId;
}

ScriptFunction *Compiler::CurrentFunction() {

    return m_CurrentFunction;
}

ScriptFunction *Compiler::FindFunctionById(int chunkId) {

    for (auto chunk: m_Functions) {
        if (chunk->Id == chunkId) {
            return chunk;
        }
    }

    return nullptr;
}

FunctionInfo *Compiler::FindFunction(const string &name) {
    // Look up native functions
    FunctionInfo *func = ResolveNativeFunction(name);
    if (func != nullptr) {
        return func;
    }

    // Look up script functions
    for (auto &chunk: m_Functions) {
        if (chunk->Name == name) {
            return chunk;
        }
    }

    // Nothing found
    return nullptr;
}

ScriptFunction *Compiler::FindScriptFunction(const string &name) {

    FunctionInfo *func = FindFunction(name);
    if (func && func->Type != ftNative) {
        return (ScriptFunction *) func;
    }

    return nullptr;
}

void Compiler::ConditionalBegin() {

    CurrentFunction()->ConditionalDepth++;
}

void Compiler::ConditionalEnd() {

    if (CurrentFunction()->ConditionalDepth > 0)
        CurrentFunction()->ConditionalDepth--;
}

StatusCode Compiler::Compile() {

    if (m_Lexer.Status() >= errError) {
        // At this point the Lexer error can only be an empty script.
        return SetResult(errFileError);
    }

    if (m_ErrorHandler == nullptr) {
        return SetResult(errError, "No error handler!");
    }


    m_Status = stsOk;
    MSG("Starting Compiler...");

    auto top = CreateFunction("", ftScript, dtVoid);
    if (top == nullptr) {
        return SetResult(errError, "Failed to create top level function.");
    }

    // Tokenize and parse the script.
    if (m_Lexer.Tokenize() != stsLexEndOfFile) {
        m_Status = errLexError;
        return SetResult(errLexError);
    }

    // PreProcessor
    if (m_PreProcessor.Run(m_Lexer.Tokens()) != stsOk) {
        m_Status = errPreProcessError;
        return SetResult(errPreProcessError);
    }

    // Compile
    // Start by skipping any initial comments
    while (IsSkippable(CurrentToken())) {
        m_CurrentPos++;
    }

    // Churn through the script
    while (!IsAtEnd()) {
        Declaration();
    }

    // END!
    EmitByte(OP_END);

    // Sanity Check
    SanityCheck();

    // Done
    if (m_Status >= errError) {
        return SetResult(errSyntaxError);
    }

    return SetResult(stsCompileDone, "Compile Done");
}

bool Compiler::IsAtEnd() {

    if (CurrentToken().TokenType == tknEndOfFile) {
        MSG("End of file token reached.");
        return true;
    }

    if (m_CurrentPos >= m_Lexer.Tokens().size()) {
        MSG("End of tokens!");
        return true;
    }

    return false;
}

bool Compiler::IsSkippable(const Token &token) {

    switch (token.TokenType) {
        case tknComment:
        case tknPreProcessor:
        case tknEndLine:
            return true;
        default:
            return false;
    }
}

Token Compiler::CurrentToken() {

    return TokenAt(m_CurrentPos);
}

Token Compiler::ConsumeToken(TokenType expect, int errorOffset, const string &errorMsg) {

    AdvanceToken(expect, errorOffset, errorMsg);

    return m_TokenPrev;
}

void Compiler::AdvanceToken(TokenType expect, int errorOffset, const string &errorMsg) {

    m_TokenPrev = CurrentToken();

    // Advance
    do {
        m_CurrentPos++;
    } while (IsSkippable(CurrentToken()) && !IsAtEnd());

    m_TokenCurrent = CurrentToken();
    Expect(m_TokenPrev, expect, errorOffset, errorMsg);
}

bool Compiler::Expect(const Token &token, TokenType expect, int errorOffset, const string &errorMsg) {

    if ((expect > tknNone) && token.TokenType != expect) {
        size_t pos = m_CurrentPos + errorOffset;
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
            AddError("Expected '" + Lexer::TokenTypeToValue(expect) + "' after '"
                     + token.Value + "'.", errorToken);
        } else {
            AddError("Expected '" + Lexer::TokenTypeToValue(expect) + "' before '"
                     + token.Value + "'.", errorToken);
        }

        return false;
    }

    return true;
}

bool Compiler::Check(TokenType tokenType) {

    return CurrentToken().TokenType == tokenType;
}

bool Compiler::CheckAhead(TokenType tokenType, int num) {

    return LookAhead(num).TokenType == tokenType;
}

bool Compiler::Match(TokenType tokenType) {

    if (!Check(tokenType))
        return false;

    AdvanceToken();
    return true;
}

bool Compiler::CheckNativeFunction(const Token &token) {

    auto nf = NativeFunctionMap.find(token.Value);
    if (nf != NativeFunctionMap.end())
        return true;

    return false;
}

bool Compiler::CheckFunction(const Token &token) {

    string name = token.Value;

    ScriptFunction *func = FindScriptFunction(name);

    return func != nullptr && func->Type == ftFunction;
}

bool Compiler::CheckMethod(const Token &token, VariableInfo *parentVar) {

    if (parentVar == nullptr || !parentVar->IsClassHead()) {
        return false;
    }

    string name = token.Value;

    ClassInfo *klass = ResolveClass(parentVar->ParentClass);
    if (klass == nullptr)
        return false;

    for (auto &m: klass->Methods) {
        if (m == "__" + klass->Name + "__" + name)
            return true;
    }

    return false;
}

Token Compiler::LookAhead(int num) {

    size_t pos = m_CurrentPos + num;
    return TokenAt(pos);
}

Token Compiler::LookBack(int num) {

    if (m_CurrentPos < (size_t) num) {
        return TokenAt(0);
    }

    size_t pos = m_CurrentPos - num;
    return TokenAt(pos);
}

Token Compiler::TokenAt(size_t pos) {

    if (m_Lexer.Tokens().empty()) {
        return {};
    }

    if (pos >= m_Lexer.Tokens().size())
        return m_Lexer.Tokens().back();

    return m_Lexer.Tokens().at(pos);
}

void Compiler::AddError(std::string errMsg, const Token &token) {

    AddError(std::move(errMsg), token.Position.LineNum, token.Position.LinePos);
}

void Compiler::AddError(std::string errMsg, size_t lineNum, size_t linePos) {

    if (m_PanicMode) // Already in error, don't report more.
        return;

    m_PanicMode = true;
    m_Status = errPanicSync;

    CompilerMessage msg;
    msg.Source = csParser;
    msg.Code = errSyntaxError;
    msg.FilePos = 0;
    msg.LineNum = lineNum;
    msg.LinePos = linePos;
    msg.Message = std::move(errMsg);

    m_ErrorHandler->AddMessage(msg);
}

void Compiler::AddWarning(std::string warningMsg, const Token &token) {

    AddWarning(std::move(warningMsg), token.Position.LineNum, token.Position.LinePos);
}

void Compiler::AddWarning(std::string warningMsg, size_t lineNum, size_t linePos) {

    CompilerMessage msg;
    msg.Source = csParser;
    msg.Code = wrnWarning;
    msg.FilePos = 0;
    msg.LineNum = lineNum;
    msg.LinePos = linePos;
    msg.Message = std::move(warningMsg);

    m_ErrorHandler->AddMessage(msg);
}

void Compiler::Synchronize() {

    m_Status = stsParserHasErrors; // Maybe?
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

StatusCode Compiler::SetResult(StatusCode result, std::string message) {

    m_Result = result;
    if (!message.empty())
        m_Message = std::move(message);

    return m_Result;
}

StatusCode Compiler::Result() {

    return m_Result;
}

string Compiler::Message() {

    return m_Message;
}

void Compiler::RunParserFunction(ParseFunc func, bool canAssign) {

    switch (func) {
        case fnGrouping:
            Grouping();
            break;
        case fnLiteral:
            NumericLiteral();
            break;
        case fnUnary:
            Unary();
            break;
        case fnBinary:
            Binary();
            break;
        case fnString:
            String();
            break;
        case fnVariable:
            Variable(canAssign);
            break;
        case fnAnd:
            And();
            break;
        case fnOr:
            Or();
            break;
        case fnTernary:
            Ternary();
            break;
        case fnCall:
            Call();
            break;
        case fnArrayIndex:
            PointerIndex(canAssign);
            break;
        case fnVariablePrefix:
            VariablePrefix();
            break;
        case fnVariablePostfix:
            VariablePostfix(canAssign);
            break;

        default:
            break;
    }
}

void Compiler::EmitByte(opCode_t byteCode) {

    CurrentFunction()->Code.push_back(byteCode);
}

void Compiler::EmitBytes(opCode_t byte0, opCode_t byte1) {

    EmitByte(byte0);
    EmitByte(byte1);
}

void Compiler::EmitBytes(opCode_t byte0, opCode_t byte1, opCode_t byte2) {

    EmitByte(byte0);
    EmitByte(byte1);
    EmitByte(byte2);
}

void Compiler::EmitBytes(opCode_t byte0, opCode_t byte1, opCode_t byte2, opCode_t byte3) {

    EmitByte(byte0);
    EmitByte(byte1);
    EmitByte(byte2);
    EmitByte(byte3);
}

void Compiler::EmitShortArg(opCode_t code, int arg) {

    u16 shortArg = (u16) arg;
    EmitBytes(code, mByte0(shortArg), mByte1(shortArg));
}

void Compiler::EmitIntArg(opCode_t code, int arg) {

    EmitByte(code);
    EmitInt(arg);
}

void Compiler::EmitPush(int count) {

    if (count == 0) {
        return;
    }

    if (count == 1) {
        EmitByte(OP_PUSH);
        return;
    }

    while (count > 0xFF) {
        EmitBytes(OP_PUSH_N, 0xFF);
        count -= 0xFF;
    }
    EmitBytes(OP_PUSH_N, count);
}

void Compiler::EmitPop(int count) {

    if (count == 0) {
        return;
    }

    if (count == 1) {
        EmitByte(OP_POP);
        return;
    }

    while (count > 0xFF) {
        EmitBytes(OP_POP_N, 0xFF);
        count -= 0xFF;
    }
    EmitBytes(OP_POP_N, count);
}

int Compiler::EmitShort(int value) {

    u16 shortVal = (u16)value;
    EmitBytes(mByte0(shortVal), mByte1(shortVal));

    return CURRENT_CODE_POS - 2;
}

void Compiler::PatchShort(int offset, int value) {

    u16 shortVal = (u16)value;
    CurrentFunction()->Code[offset] = mByte0(shortVal);
    CurrentFunction()->Code[offset + 1] = mByte1(shortVal);
}

int Compiler::EmitInt(int value) {

    EmitBytes(mByte0(value), mByte1(value), mByte2(value), mByte3(value));

    return CURRENT_CODE_POS - 4;
}

void Compiler::PatchInt(int offset, int value) {

    CurrentFunction()->Code[offset] = mByte0(value);
    CurrentFunction()->Code[offset + 1] = mByte1(value);
    CurrentFunction()->Code[offset + 2] = mByte2(value);
    CurrentFunction()->Code[offset + 3] = mByte3(value);
}

u32 Compiler::AddConstant(const ConstantInfo &constant) {

    // Check if it already exists
    // Once the type matches we only need to compare one of the data types
    // TODO: This probably doesn't actually need to check the type at all
    for (size_t i = 0; i < m_ConstValues.size(); ++i) {
        if (constant.Type == m_ConstValues[i].Type &&
            constant.ConstValue.Int == m_ConstValues[i].ConstValue.Int) {
            return i;
        }
    }

    // Add new constant
    m_ConstValues.push_back(constant);
    return (m_ConstValues.size() - 1);
}

void Compiler::EmitConstant(const ConstantInfo &constant) {

    u32 pos = AddConstant(constant);

    if (pos > 0xFFFFFF) {
        AddError("Maximum number of constants reached.", CurrentToken());
        return;
    }

    if (pos <= 0xFF) {
        EmitBytes(OP_CONSTANT, (opCode_t) pos);
    } else if (pos <= 0xFFFF) {
        //AddWarning("Number of constants exceeds 255.", CurrentToken());
        EmitBytes(OP_CONSTANT_16, mByte0(pos), mByte1(pos));
    } else {
        //AddWarning("Number of constants exceeds 65535.", CurrentToken());
        EmitBytes(OP_CONSTANT_24, mByte0(pos), mByte1(pos), mByte2(pos));
    }
}

u32 Compiler::AddString(const string &str) {
    // Check if it already exists
    for (auto &s: m_ConstStrings) {
        if (s.String == str) {
            return s.Index;
        }
    }

    // Add new constant string
    u32 newIndex = (u32) m_StringData.size();
    StringData newString {
            .Index = newIndex,
            .Length = (uint32_t) str.length(),
            .String = str
    };

    for (auto c: str) {
        m_StringData.push_back(c);
    }
    // Add null char(s) until the size is divisible by 4.
    // This makes it easier to align values in the VM.
    do {
        m_StringData.push_back('\0');
    } while ((m_StringData.size() & 0x03) != 0);

    newString.Length = m_StringData.size() - newString.Index;

    m_ConstStrings.push_back(newString);

    return newIndex;
}

void Compiler::EmitString(const string &str) {

    u32 pos = AddString(str);

    if (pos > 0xFFFFFF) {
        AddError("Maximum string storage size reached.", CurrentToken());
        return;
    }

    if (pos <= 0xFF) {
        EmitBytes(OP_STRING, (opCode_t)pos);
    } else if (pos <= 0xFFFF) {
        EmitBytes(OP_STRING_16, mByte0(pos), mByte1(pos));
    } else {
        EmitBytes(OP_STRING_24, mByte0(pos), mByte1(pos), mByte2(pos));
    }
}

void Compiler::EndCompile() {

    EmitByte(OP_RETURN);
}

void Compiler::Declaration() {

    DataType varType;
    u32 varFlags;

    if (Match(tknClass)) {
        ClassDeclaration();
    } else if (MatchTypeDeclaration(varType, varFlags)) {
        TypeDeclaration(varType, varFlags);
    } else {
        Statement();
    }

    if (m_PanicMode || m_Status == errPanicSync) {
        Synchronize();
    }
}

void Compiler::TypeDeclaration(DataType dataType, u32 flags) {

    if (dataType == dtClass) {
        ClassInstanceDeclaration();
    } else if (CheckAhead(tknLeftParen)) {
        if (CurrentClass()) {
            MethodDeclaration(dataType);
        } else {
            FunctionDeclaration(dataType);
        }
    } else if (CheckAhead(tknLeftSquareBracket)) {
        ArrayDeclaration(dataType, flags);
    } else {
        VariableDeclaration(dataType, flags);
    }
}

// Compiles a simple statement. These can only appear at the top-level or
// within curly blocks. Simple statements exclude variable binding statements
// like "var" and "class" which are not allowed directly in places like the
// branches of an "if" statement.
//
// Unlike expressions, statements do not leave a value on the stack.
void Compiler::Statement() {

    if (Match(tknBreak)) {
        BreakStatement();
    } else if (Match(tknContinue)) {
        ContinueStatement();
    } else if (Match(tknFor)) {
        ForStatement();
    } else if (Match(tknIf)) {
        IfStatement();
    } else if (Match(tknReturn)) {
        ReturnStatement();
    } else if (Match(tknWhile)) {
        WhileStatement();
    } else if (Match(tknSwitch)) {
        SwitchStatement();
    } else if (Match(tknLeftCurly)) {
        ScopeBegin();
        Block();
        ScopeEnd();
    } else {
        ExpressionStatement();
    }
}

void Compiler::Block() {

    while (!Check(tknRightCurly) && !IsAtEnd()) {
        Declaration();
    }

    ConsumeToken(tknRightCurly, -2, "Expected '}' after block.");
}

/* Updates the current type and returns the previous expected type */
DataType Compiler::TypeBegin(TypeInfo *typeInfo) {

    DataType expecting = dtNone;

    if (m_CurrentType != nullptr) {
        expecting = m_CurrentType->Type;
        typeInfo->IgnoreExpectingOnSet = m_CurrentType->IgnoreExpectingOnSet;
    }

    typeInfo->Enclosing = m_CurrentType;

    m_CurrentType = typeInfo;

    return expecting;
}

DataType Compiler::TypeSetCurrent(const DataType type, bool force) {

    if (m_CurrentType == nullptr)
        return dtNone;

    if (force) {
        m_CurrentType->Type = type;
        return m_CurrentType->Type;
    }

    if (m_CurrentType->Type != dtNone && m_CurrentType->Type <= dtString)
        return m_CurrentType->Type;

    if (m_CurrentType->IgnoreExpectingOnSet) {
        m_CurrentType->Type = type;
    } else {
        m_CurrentType->Type = m_CurrentType->Expecting() == dtFloat ? dtFloat : type;
    }
    return m_CurrentType->Type;
}

DataType Compiler::CurrentType() {

    if (m_CurrentType == nullptr)
        return dtNone;

    return (m_CurrentType->Type == dtNone) ? m_CurrentType->Expecting() : m_CurrentType->Type;
}

TypeCompatibility Compiler::TypeCheck(DataType type, const string &errorMessage) {

    if (m_CurrentType == nullptr)
        return tcNotApplicable;

    DataType expecting = CurrentType();
    auto compat = TypeInfo::CheckCompatibility(expecting, type);
    if (compat == tcIncompatible) {
        if (!errorMessage.empty()) {
            AddError(errorMessage, LookBack());
        } else {
            AddError("Incompatible type. Expected '" + DataTypeToString(expecting) + "'.", LookBack());
        }
    }

    return compat;
}

DataType Compiler::TypeEnd() {

    DataType dataType = CurrentType();
    m_CurrentType = m_CurrentType->Enclosing;
    return dataType;
}

DataType Compiler::Expression() {

    TypeInfo type;
    TypeBegin(&type);
    ParsePrecedence(precAssignment);
    return TypeEnd();
}

ConstantInfo Compiler::ParseNumericLiteral() {
    // Get the token that got us here
    const Token &token = LookBack();

    ConstantInfo constant{};

    // Boolean
    if (token.TokenType == tknFalse) {
        constant.Type = dtBool;
        constant.ConstValue = BOOL_VAL(false);
        return constant;
    } else if (token.TokenType == tknTrue) {
        constant.Type = dtBool;
        constant.ConstValue = BOOL_VAL(true);
        return constant;
    }

        // Float
    else if (token.TokenType == tknFloatLiteral) {
        constant.Type = dtFloat;
        float fVal;
        if (ScriptUtils::StringToFloat(token.Value, fVal)) {
            if (fVal == 0) {
                constant.ConstValue = INT32_VAL(0);
                return constant;
            } else {
                constant.ConstValue = FLOAT_VAL(fVal);
                return constant;
            }
        } else {
            AddError("Failed to parse floating point literal", token);
        }
    }

        // Int
    else if (token.TokenType == tknIntegerLiteral) {
        constant.Type = dtInt32;
        int iVal;
        string str;
        int base;
        if (token.Value.starts_with("0b")) { // Binary
            str = token.Value.substr(2);
            base = 2;
        } else if (token.Value.starts_with("0o")) { // Octal
            str = token.Value.substr(2);
            base = 8;
        } else { // Decimal or Hex
            str = token.Value;
            base = 0; // Determined from string
        }

        if (ScriptUtils::StringToInt(str, iVal, base)) {
            constant.ConstValue = INT32_VAL(iVal);
            return constant;
        } else {
            AddError("Failed to parse integer literal", token);
        }
    }

        // Null / Nil
    else if (token.TokenType == tknNull) {
        constant.Type = dtInt32;
        constant.ConstValue = INT32_VAL(0);
        return constant;
    }

    constant.Type = dtInt32;
    constant.ConstValue = INT32_VAL(0);
    return constant;
}

void Compiler::NumericLiteral() {

    const ConstantInfo literal = ParseNumericLiteral();

    EmitConstant(literal);

    TypeSetCurrent(literal.Type);

    TypeCompatibility compat = TypeCheck(literal.Type);

    if (compat == tcCastSignedToFloat) {
        string msg = "Integer literal will be implicitly cast to surrounding float type.\n";
        msg += "Add decimal place(s) to specify a floating point literal.";
        AddWarning(msg, LookBack());
    } else if (compat == tcCastFloatToSigned) {
        string msg = "Floating point literal will be implicitly cast to surrounding integer type.\n";
        msg += "Remove decimal place(s) to specify a integer literal.";
        AddWarning(msg, LookBack());
    }

    EmitCast(compat);
}

void Compiler::String() {

    Token stringToken = LookBack();

    EmitString(stringToken.Value);
}

void Compiler::Variable(bool canAssign) {

    const Token &token = LookBack();

    if (CheckNativeFunction(token)) {
        NativeFunction(token);
    } else if (CheckFunction(token)) {
        NamedFunction(token);
    } else {
        NamedVariable(token, canAssign);
    }
}

void Compiler::And() {

    int endJump = EmitJump(OP_JUMP_IF_FALSE);

    EmitByte(OP_POP);
    ParsePrecedence(precAnd);
    PatchJump(endJump);
}

void Compiler::Or() {

    int endJump = EmitJump(OP_JUMP_IF_TRUE);

    EmitByte(OP_POP);
    ParsePrecedence(precOr);
    PatchJump(endJump);
}

void Compiler::VariablePrefix() {

    TokenType operatorType = LookBack().TokenType;
    // Make sure the next token is a variable
    if (!Match(tknIdentifier)) {
        AddError("Expected identifier after prefix operator.", LookBack(1));
        return;
    }

    const string &name = LookBack().Value;
    VariableInfo *variable = ResolveVariable(name);
    if (variable == nullptr) {
        return;
    }

    Token opToken = LookBack();

    EmitPointer(variable);

    switch (operatorType) {
        case tknPlusPlus:
            EmitByte(OP_PREFIX_INCREASE);
            break;
        case tknMinusMinus:
            EmitByte(OP_PREFIX_DECREASE);
            break;
        default:
            AddError("Invalid variable prefix operation.", opToken);
            break;
    }

    // Cast the value to the required type
    TypeCompatibility cast = TypeCheck(variable->Type());
    EmitCast(cast);
}

void Compiler::VariablePostfix(bool canAssign) {

    if (!canAssign) {
        AddError("Token is not assignable.", LookBack(2));
        return;
    }

    // The value is already on the stack, we only want to modify it's stored value.
    // The stack gets left alone.
    const string &name = LookBack(2).Value;
    VariableInfo *variable = ResolveVariable(name);

    if (variable == nullptr) {
        return;
    }

    Token opToken = LookBack();

    EmitPointer(variable);

    switch (opToken.TokenType) {
        case tknPlusPlus:
            EmitByte(OP_PLUS_PLUS);
            break;
        case tknMinusMinus:
            EmitByte(OP_MINUS_MINUS);
            break;
        default:
            AddError("Invalid variable postfix operation.", opToken);
            break;
    }
}

void Compiler::Unary() {

    TokenType operatorType = LookBack().TokenType;

    TypeInfo unaryType;
    TypeBegin(&unaryType);

    // Compile the operand
    ParsePrecedence(precUnary);

    switch (operatorType) {
        case tknMinus:
            EmitByte(unaryType.Type == dtFloat ? OP_NEGATE_F : OP_NEGATE_I);
            break;
        case tknExclamation:
            EmitByte(OP_NOT);
            break;
        case tknBitwiseNot: {
            EmitByte(OP_BIT_NOT);
            break;
        }

        default:
            break;
    }

    EmitCast(TypeInfo::CheckCompatibility(unaryType.Expecting(), unaryType.Type));

    TypeEnd();
}

void Compiler::Binary() {

    DataType lhsType = CurrentType();
    TypeInfo binType;
    TypeBegin(&binType);

    TokenType operatorType = LookBack().TokenType;
    ParseRule rule = Rules::Get(operatorType);
    ParsePrecedence((Precedence) (rule.Prec + 1));

    DataType rhsType = CurrentType();

    DataType binaryType = (lhsType == dtFloat || rhsType == dtFloat) ? dtFloat : dtInt32;

    // Override binary type to int for certain ops
    if (operatorType == tknBitwiseAnd ||
        operatorType == tknBitwiseAndEquals ||
        operatorType == tknBitwiseOr ||
        operatorType == tknBitwiseOrEquals ||
        operatorType == tknBitwiseXor ||
        operatorType == tknBitwiseXorEquals ||
        operatorType == tknShiftLeft ||
        operatorType == tknShiftRight) {
        if (binaryType == dtFloat) {
            AddError("Cannot use floating point numbers in binary operations.",
                     LookBack(lhsType == dtFloat ? 3 : 1));
        }
        binaryType = dtInt32;
    } else if (operatorType == tknPercent) {
        if (binaryType == dtFloat) {
            AddWarning("'%' operator with floating point values will be implicitly cast to integer type. "
                       "Data may be lost.",
                       LookBack(lhsType == dtFloat ? 3 : 1));
        }
        binaryType = dtInt32;
    }

    EmitCast(TypeInfo::CheckCompatibility(binaryType, lhsType), true);
    EmitCast(TypeInfo::CheckCompatibility(binaryType, rhsType));

    switch (operatorType) {
        // Term
        case tknPlus:
        case tknPlusEquals:
            EmitAdd(binaryType);
            break;
        case tknMinus:
        case tknMinusEquals:
            EmitSubtract(binaryType);
            break;

            // Factor
        case tknStar:
        case tknTimesEquals:
            EmitMultiply(binaryType);
            break;
        case tknSlash:
        case tknDivideEquals:
            EmitDivide(binaryType);
            break;
        case tknPercent:
            EmitByte(OP_MODULUS);
            break;

            // Comparison
        case tknEquals:
            EmitEqual(binaryType);
            break;
        case tknNotEqual:
            EmitNotEqual(binaryType);
            break;
        case tknLessThan:
            EmitLessThan(binaryType);
            break;
        case tknLessEqual:
            EmitLessThanOrEqual(binaryType);
            break;
        case tknGreaterThan:
            EmitGreaterThan(binaryType);
            break;
        case tknGreaterEqual:
            EmitGreaterThanOrEqual(binaryType);
            break;

            // Bitwise
        case tknBitwiseAnd:
        case tknBitwiseAndEquals:
            EmitByte(OP_BIT_AND);
            break;
        case tknBitwiseOr:
        case tknBitwiseOrEquals:
            EmitByte(OP_BIT_OR);
            break;
        case tknBitwiseXor:
        case tknBitwiseXorEquals:
            EmitByte(OP_BIT_XOR);
            break;
        case tknShiftLeft:
            EmitByte(OP_BIT_SHIFT_L);
            break;
        case tknShiftRight:
            EmitByte(OP_BIT_SHIFT_R);
            break;

        default:
            break;
    }

    TypeEnd();

    // If the resulting type does not match the required type, cast it.
    EmitCast(TypeCheck(binaryType));
}

void Compiler::EmitAdd(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_ADD_F);
    } else if (type == dtUint32) {
        EmitByte(OP_ADD_U);
    } else {
        EmitByte(OP_ADD_S);
    }
}

void Compiler::EmitSubtract(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_SUB_F);
    } else if (type == dtUint32) {
        EmitByte(OP_SUB_U);
    } else {
        EmitByte(OP_SUB_S);
    }
}

void Compiler::EmitMultiply(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_MULT_F);
    } else if (type == dtUint32) {
        EmitByte(OP_MULT_U);
    } else {
        EmitByte(OP_MULT_S);
    }
}

void Compiler::EmitDivide(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_DIV_F);
    } else if (type == dtUint32) {
        EmitByte(OP_DIV_U);
    } else {
        EmitByte(OP_DIV_S);
    }
}

void Compiler::EmitEqual(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_EQUAL_F);
    } else if (type == dtUint32) {
        EmitByte(OP_EQUAL_U);
    } else {
        EmitByte(OP_EQUAL_S);
    }
}

void Compiler::EmitNotEqual(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_NOT_EQUAL_F);
    } else if (type == dtUint32) {
        EmitByte(OP_NOT_EQUAL_U);
    } else {
        EmitByte(OP_NOT_EQUAL_S);
    }
}

void Compiler::EmitLessThan(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_LESS_F);
    } else if (type == dtUint32) {
        EmitByte(OP_LESS_U);
    } else {
        EmitByte(OP_LESS_S);
    }
}

void Compiler::EmitLessThanOrEqual(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_LESS_OR_EQUAL_F);
    } else if (type == dtUint32) {
        EmitByte(OP_LESS_OR_EQUAL_U);
    } else {
        EmitByte(OP_LESS_OR_EQUAL_S);
    }
}

void Compiler::EmitGreaterThan(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_GREATER_F);
    } else if (type == dtUint32) {
        EmitByte(OP_GREATER_U);
    } else {
        EmitByte(OP_GREATER_S);
    }
}

void Compiler::EmitGreaterThanOrEqual(DataType type) {
    if (type == dtFloat) {
        EmitByte(OP_GREATER_OR_EQUAL_F);
    } else if (type == dtUint32) {
        EmitByte(OP_GREATER_OR_EQUAL_U);
    } else {
        EmitByte(OP_GREATER_OR_EQUAL_S);
    }
}

void Compiler::Grouping() {

    // Group type needs to start fresh and ignore any surrounding type
    TypeInfo groupType;
    TypeBegin(&groupType);
    groupType.IgnoreExpectingOnSet = true;

    DataType exprType = Expression();

    TypeSetCurrent(exprType);
    TypeEnd();

    TypeSetCurrent(groupType.Type);

    AdvanceToken(tknRightParen);

    // Cast the group to match the surrounding type
    EmitCast(TypeCheck(exprType));
}

void Compiler::ParsePrecedence(Precedence precedence) {

    Token token = ConsumeToken();

    // Find the prefix operator
    bool canAssign = precedence <= precAssignment;
    ParseFunc prefixFunc = Rules::Get(token.TokenType).Prefix;
    if (prefixFunc == fnNone) {
        AddError("Expected expression.", token);
        return;
    }

    // Compile the prefix operation
    RunParserFunction(prefixFunc, canAssign);

    // Compile infix operations of higher or equal precedence
    while (precedence <= Rules::Get(CurrentToken().TokenType).Prec) {
        token = ConsumeToken();
        ParseFunc infixFunc = Rules::Get(token.TokenType).Infix;
        RunParserFunction(infixFunc, canAssign);
    }

    if (canAssign && Match(tknAssign)) { // Does this need to include += -= /= *= etc?
        AddError("Invalid assignment target.", LookBack());
    }
}

void Compiler::IfStatement() {

    ConditionalBegin();

    // Condition
    ConsumeToken(tknLeftParen, -2, "Expected '(' after 'if' statement.");
    Expression();
    ConsumeToken(tknRightParen, -2, "Expected ')' after condition.");

    // Jump over if false
    int thenJump = EmitJump(OP_JUMP_IF_FALSE);
    EmitByte(OP_POP);

    // Body
    Statement();

    // Else Jump
    int elseJump = EmitJump(OP_JUMP);

    PatchJump(thenJump);
    EmitByte(OP_POP);

    ConditionalEnd();

    if (Match(tknElse))
        Statement();

    PatchJump(elseJump);
}

void Compiler::Ternary() {

    // [condition] ? [trueExpr] : [falseExpr];
    // There should be a value on the stack now
    // Use it as the condition

    // Clear the current type as the boolean statement isn't relevant anymore
    TypeSetCurrent(dtNone, true);
    DataType expectingType = CurrentType();
    DataType valueType = dtNone;

    // If the condition is false we jump over to true expression
    int falseJump = EmitJump(OP_JUMP_IF_FALSE);

    // ==== True expression ====
    EmitByte(OP_POP); // Get the condition off the stack

    valueType = Expression();

    int exitJump = EmitJump(OP_JUMP);

    // :
    ConsumeToken(tknColon, -1, "Expected ':' after ternary true result expression.");

    // ==== False expression ====
    PatchJump(falseJump); // << False condition lands here
    EmitByte(OP_POP); // Get the condition off the stack

    valueType = Expression();

    // End
    PatchJump(exitJump);

    TypeCompatibility compat = TypeCheck(valueType);
    if (compat != tcMatch && compat > tcIncompatible) {
        AddWarning("Expression will be implicitly cast to assignee type: "
                   + DataTypeToString(expectingType), LookBack());
    }
    EmitCast(compat);

    // Set the type to the expected type so no further casting happens.
    TypeSetCurrent(expectingType);
}

void Compiler::ReturnStatement() {

    if (CurrentFunction()->Type == ftScript) {
        AddError("Can't return from top-level code.", LookBack());
    }

    DataType expectedReturn = CurrentFunction()->ReturnType;

    if (Match(tknSemiColon)) {
        if (CurrentFunction()->ReturnType > dtVoid) {
            AddError("Expected function return type of '" + DataTypeToString(expectedReturn) + "'.", LookBack());
        }
        EmitReturn();
    } else {
        u32 pos = CURRENT_TOKEN_POS;
        DataType returnType = Expression();

        TypeCompatibility returnCompat = TypeInfo::CheckCompatibility(returnType, expectedReturn);
        if (returnCompat == tcIncompatible) {
            AddError("Expected function return type of '" + DataTypeToString(expectedReturn) + "'.", TokenAt(pos));
        }
        ConsumeToken(tknSemiColon, -2, "Expected ';' after return value.");
        EmitCast(returnCompat);
        EmitByte(OP_RETURN);
    }

    // If we are not inside a conditional, mark the return statement as supplied.
    if (CurrentFunction()->ConditionalDepth == 0)
        CurrentFunction()->ReturnSupplied = true;
}

// Marks the beginning of a loop. Keeps track of the current instruction so we
// know what to loop back to at the end of the body.
void Compiler::LoopBegin(LoopInfo *loop) {

    loop->Enclosing = m_CurrentLoop;
    m_CurrentLoop = loop;

    loop->Start = CURRENT_CODE_POS;
    loop->ScopeDepth = CURRENT_SCOPE;
}

// Compiles the body of the loop and tracks its extent so that contained "break"
// statements can be handled correctly.
void Compiler::LoopBody() {

    m_CurrentLoop->Body = CURRENT_CODE_POS;
    Statement();
}

// Emits the [OP_JUMP_IF_FALSE] instruction used to test the loop condition and
// potentially exit the loop. Keeps track of the instruction so we can patch it
// later once we know where the end of the body is.
void Compiler::LoopTestExit() {

    m_CurrentLoop->ExitJump = EmitJump(OP_JUMP_IF_FALSE);
}

// Ends the current innermost loop. Patches up all jumps and breaks now that
// we know where the end of the loop is.
void Compiler::LoopEnd() {

    int currentCode = CURRENT_CODE_POS;
    int loopOffset = currentCode - m_CurrentLoop->Start + 3;
    EmitShortArg(OP_LOOP, loopOffset);

    // Exit jump not used by infinite for loops
    if (m_CurrentLoop->ExitJump != NOT_SET) {
        PatchJump(m_CurrentLoop->ExitJump);
        EmitByte(OP_POP);
    }

    // Find any break placeholder instructions (which will be OP_BREAK in the
    // bytecode) and replace them with real jumps.
    int i = m_CurrentLoop->Body;
    while (i < CURRENT_CODE_POS) {
        if (CurrentFunction()->Code[i] == OP_BREAK) {
            //m_CodeBytes[i] = OP_JUMP;
            PatchJump(i + 1);
            i += 3;
        } else {
            // Skip this instruction and its arguments.
            //i += 1 + getByteCountForArguments(compiler->fn->code.data, compiler->fn->constants.data, i);
            ++i;
        }
    }

    m_CurrentLoop = m_CurrentLoop->Enclosing;
}

void Compiler::WhileStatement() {

    LoopInfo loop;
    LoopBegin(&loop);

    ConsumeToken(tknLeftParen, -1, "Expected '(' after 'while' statement.");
    Expression();
    ConsumeToken(tknRightParen, -2, "Expect ')' after condition.");

    LoopTestExit();
    EmitByte(OP_POP);

    // Loop Body
    LoopBody();

    LoopEnd();
}

void Compiler::ForStatement() {

    ScopeBegin();
    ConsumeToken(tknLeftParen, -1, "Expected '(' after 'for' statement.");

    // Initializer
    DataType varType;
    u32 varFlags;
    if (Match(tknSemiColon)) {
        // No initializer.
    } else if (MatchTypeDeclaration(varType, varFlags)) {
        VariableDeclaration(varType, varFlags);
    } else {
        ExpressionStatement();
    }

    // Condition
    LoopInfo loop;
    loop.ExitJump = -1;
    LoopBegin(&loop);

    if (!Match(tknSemiColon)) {
        Expression();
        ConsumeToken(tknSemiColon, -1, "Expected ';' after 'for' loop condition.");

        // Jump out of the loop if the condition is false.
        LoopTestExit();
        EmitByte(OP_POP); // Condition.
    }

    // Post loop expression
    if (!Match(tknRightParen)) {
        int bodyJump = EmitJump(OP_JUMP);
        int incrementStart = CURRENT_CODE_POS;
        Expression();
        EmitByte(OP_POP);
        ConsumeToken(tknRightParen, -1, "Expected ')' after 'for' loop clauses.");

        EmitLoop(loop.Start);
        loop.Start = incrementStart;
        PatchJump(bodyJump);
    }

    // Loop Body
    LoopBody();

    // End
    LoopEnd();

    ScopeEnd();
}

// Marks the beginning of a switch
void Compiler::SwitchBegin(SwitchInfo *switchInfo) {

    switchInfo->Enclosing = m_CurrentSwitch;
    m_CurrentSwitch = switchInfo;

    switchInfo->ScopeDepth = CURRENT_SCOPE;
}

// Marks the body of a switch
void Compiler::SwitchBody() {

    m_CurrentSwitch->Body = CURRENT_CODE_POS;
}

// Ends the current innermost loop. Patches up all jumps and breaks now that
// we know where the end of the loop is.
void Compiler::SwitchEnd() {
    // Find any break placeholder instructions (which will be OP_BREAK in the
    // bytecode) and replace them with real jumps.
    int i = m_CurrentSwitch->Body;
    while (i < CURRENT_CODE_POS) {
        if (CurrentFunction()->Code[i] == OP_BREAK) {
            //m_CodeBytes[i] = OP_JUMP;
            PatchJump(i + 1);
            i += 3;
        } else {
            // Skip this instruction and its arguments.
            //i += 1 + getByteCountForArguments(compiler->fn->code.data, compiler->fn->constants.data, i);
            ++i;
        }
    }

    m_CurrentSwitch = m_CurrentSwitch->Enclosing;
}

void Compiler::SwitchStatement() {
    ScopeBegin();
    SwitchInfo switchInfo;
    SwitchBegin(&switchInfo);

    // Input expression
    ConsumeToken(tknLeftParen, -1, "Expected '(' after 'switch' statement.");

    Token switchToken = CurrentToken();

    // Parse the input expression and remember its type.
    DataType switchType = Expression();

    if (switchType == dtFloat) {
        AddError("Switch statement requires expression of integer type ('float' invalid).", LookBack());
    }

    ConsumeToken(tknRightParen, -1, "Expected ')' after 'switch' expression.");

    // Body
    ConsumeToken(tknLeftCurly, -1, "Expected '{' to start 'switch' body.");

    /* The input value will now be on the top of the stack.
     * Stack = [switchValue]
     * OpCode = [OP_SWITCH][JumpTableOffset][MinValue][MaxValue]
     */

    int switchJumpPos = EmitJump(OP_SWITCH);
    int minValuePos = EmitInt(0);
    int maxValuePos = EmitInt(0);

    SwitchBody();

    // Parse the case labels. There can be more than one before a case body.
    JumpTable<int, int> jumpTable;

    while (Match(tknCase)) {
        // Parse fall through case labels
        do {
            if (!Match(tknIntegerLiteral) && !Match(tknFloatLiteral)) {
                AddError("Expected numerical literal in case label.", LookBack());
            }

            const Token &caseToken = LookBack();
            ConstantInfo value = ParseNumericLiteral();

            // Check the type of the case label. Input values must be integers
            TypeCompatibility caseCompat = TypeInfo::CheckCompatibility(switchType, value.Type);
            if (caseCompat != tcMatch) {
                AddError("Case label type not compatible.", LookBack());
            }

            // Make sure it doesn't already exist
            if (!jumpTable.Add(value.ConstValue.Int, CURRENT_CODE_POS)){
                AddError("case label '" + caseToken.Value + "' already exists.",
                         caseToken);
            }

            ConsumeToken(tknColon, -1, "Expected ':' after case label.");
        } while (Match(tknCase));

        // Parse the case body, which can be multiple statements, not necessarily in a block.
        while (!Check(tknCase) && !Check(tknDefault) && !Check(tknRightCurly) && !IsAtEnd()) {
            Statement();
        }
    }

    // Default
    int defaultCase = CURRENT_CODE_POS;
    // This gets run no matter what, unless the user breaks out of the switch above
    if (Match(tknDefault)) {
        ConsumeToken(tknColon, -1, "Expected ':' after default label.");
        while (!Check(tknRightCurly) && !IsAtEnd()) {
            Statement();
        }
    }


    ConsumeToken(tknRightCurly, -1, "Expected '}' to end 'switch' body.");

    if (jumpTable.Count() > 0) {
        int caseMin = jumpTable.LowestValue();
        int caseMax = jumpTable.HighestValue();
        int caseRange = caseMax - caseMin;

        // Warn about big jump table
        if ((jumpTable.Count() * 2) <= caseRange) {
            AddWarning("Switch statement contains a large range and a small number of case labels. "
                       "Consider using multiple condensed switch statements or if/else statements instead.",
                       switchToken);
        }

        PatchInt(minValuePos, caseMin);
        PatchInt(maxValuePos, caseMax);

        // Build the jump table
        // If there's no break statement or default case provided, we need to jump over the table body.
        int jumpTableStart = EmitJump(OP_JUMP);

        // Put the default jump at the start so that it's always available to out-of-range values.
        int defaultJump = CURRENT_CODE_POS - defaultCase;
        EmitShort(defaultJump);

        // Build table containing the entire range. Empty cases get the default jump address.
        for (int i = caseMin; i <= caseMax; ++i) {
            int addr;
            // If a case exists the addr will be updated.
            if (!jumpTable.Find(i, addr)) {
                addr = defaultCase;
            }
            // Convert the address to an offset. The jump is backwards.
            int jumpBack = CURRENT_CODE_POS - addr;

            EmitShort(jumpBack);
        }

        // Patch the jump over the table body.
        PatchJump(jumpTableStart);
    }

    PatchJump(switchJumpPos);

    SwitchEnd(); // Takes care of break statements.

    ScopeEnd();
}

void Compiler::SwitchAsIfElse() {

    ScopeBegin();
    SwitchInfo switchInfo;
    SwitchBegin(&switchInfo);

    // Input expression
    ConsumeToken(tknLeftParen, -1, "Expected '(' after 'switch' statement.");
    // Parse the input expression and remember its type.
    DataType switchType = Expression();

    // The input value will now be on the top of the stack.
    // Create a local variable from it, so it's easy to access later.
    VmPointer inputPtr = VmPointer(CurrentFunction()->Locals.size(), switchType, scopeLocal);
    VariableInfo *swVar;
    swVar = new VariableInfo();
    swVar->Name = "<switch>";
    swVar->Depth = m_ScopeDepth;
    swVar->Pointer = inputPtr;
    CurrentFunction()->Locals.push_back(swVar);
    EmitSetVariable(OP_ASSIGN, swVar, switchType);

    ConsumeToken(tknRightParen, -1, "Expected ')' after 'switch' expression.");

    // Body
    ConsumeToken(tknLeftCurly, -1, "Expected '{' to start 'switch' body.");

    SwitchBody();

    // Parse the case labels. There can be more than one before a case body.
    std::vector<ConstantInfo> cases;

    while (Match(tknCase)) {
        std::vector<int> jumps;
        do {
            // Parse fall through case labels
            // Push the input expression onto the stack
            EmitGetVariable(swVar, switchType);

            if (!Match(tknIntegerLiteral) && !Match(tknFloatLiteral)) {
                AddError("Expected numerical literal in case label.", LookBack());
            }

            const Token &caseToken = LookBack();
            ConstantInfo value = ParseNumericLiteral();

            // Check the type of the case label
            TypeCompatibility caseCompat = TypeInfo::CheckCompatibility(switchType, value.Type);
            if (caseCompat == tcCastFloatToSigned) {
                value.ConstValue.Int = (int) value.ConstValue.Float;
                value.Type = dtInt32;
            } else if (caseCompat == tcCastSignedToFloat) {
                value.ConstValue.Float = (float) value.ConstValue.Int;
                value.Type = dtFloat;
            }

            // Make sure it doesn't already exist
            for (auto c: cases) {
                if (value.Type == c.Type && AS_INT32(value.ConstValue) == AS_INT32(c.ConstValue)) {
                    AddError("case label '" + caseToken.Value + "' already exists.",
                             caseToken);
                }
            }
            cases.push_back(value);

            EmitConstant(value);

            ConsumeToken(tknColon, -1, "Expected ':' after case label.");

            // Jump to the case statement if true.
            jumps.push_back(EmitJump(OP_JUMP_IF_EQUAL));
        } while (Match(tknCase));

        // If we made it here, no labels matched, so we jump the case body.
        int skipJump = EmitJump(OP_JUMP);

        // Patch the case match jumps
        for (auto jump: jumps) {
            PatchJump(jump);
        }

        // Parse the case body, which can be multiple statements, not necessarily in a block.
        while (!Check(tknCase) && !Check(tknDefault) && !Check(tknRightCurly) && !IsAtEnd()) {
            Statement();
        }

        // Patch the jump over the statement
        PatchJump(skipJump);
    }

    // This gets run no matter what, unless the user breaks out of the switch above
    if (Match(tknDefault)) {
        ConsumeToken(tknColon, -1, "Expected ':' after default label.");
        while (!Check(tknRightCurly) && !IsAtEnd()) {
            Statement();
        }
    }

    ConsumeToken(tknRightCurly, -1, "Expected '}' to end 'switch' body.");
    SwitchEnd(); // Takes care of break statements.
    EmitPop();
    ScopeEnd();
}

// Generates code to discard local variables at [depth] or greater. Does *not*
// actually undeclare variables or pop any scopes, though. This is called
// directly when compiling "break" statements to ditch the local variables
// before jumping out of the loop even though they are still in scope *past*
// the break instruction.
//
// Returns the number of local variables that were eliminated.
int Compiler::DiscardLocals(int depth) {
    //ASSERT(m_ScopeDepth > -1, "Cannot exit top-level scope.");

    int local = (int) (CurrentFunction()->Locals.size() - 1);
    int pops = 0;
    while (local >= 0 && CurrentFunction()->Locals[local]->Depth >= depth) {

        // Call destructors on classes going out of scope
        VariableInfo *var = CurrentFunction()->Locals[local];
        // TODO: Check this is required/correct
        // If not we need to check variable reads here for sanity checking
        Destroy(var);

        ++pops;

        local--;
    }

    EmitPop(pops);

    return pops;
}

void Compiler::BreakStatement() {

    ConsumeToken(tknSemiColon, -3, "Expected ';' after 'break'.");

    if (m_CurrentLoop == nullptr && m_CurrentSwitch == nullptr) {
        AddError("Cannot use 'break' outside of a loop or switch.", LookBack(3));
        return;
    }

    // Since we will be jumping out of the scope, make sure any locals in it are discarded first.
    if ((m_CurrentLoop == nullptr) ||
        (m_CurrentSwitch != nullptr && m_CurrentSwitch->ScopeDepth > m_CurrentLoop->ScopeDepth)) {
        DiscardLocals(m_CurrentSwitch->ScopeDepth + 1);
    } else {
        DiscardLocals(m_CurrentLoop->ScopeDepth + 1);
    }

    // Emit a placeholder instruction for the jump to the end of the body. When
    // we're done compiling the loop body and know where the end is, we'll
    // replace the address with appropriate offsets.
    EmitJump(OP_BREAK);
}

void Compiler::ContinueStatement() {

    ConsumeToken(tknSemiColon, -3, "Expected ';' after 'continue'.");

    if (m_CurrentLoop == nullptr) {
        AddError("Cannot use 'continue' outside of a loop.", LookBack(3));
        return;
    }

    // Since we will be jumping out of the scope, make sure any locals in it are discarded first.
    DiscardLocals(m_CurrentLoop->ScopeDepth + 1);

    // Emit a jump back to the top of the loop
    int loopOffset = CURRENT_CODE_POS - m_CurrentLoop->Start + 3;
    EmitShortArg(OP_LOOP, loopOffset);
}

void Compiler::ExpressionStatement() {

    Expression();
    ConsumeToken(tknSemiColon, -2, "Expected ';' after expression.");
    EmitByte(OP_POP);
}

void Compiler::ScopeBegin() {

    ++m_ScopeDepth;
}

void Compiler::ScopeEnd(bool pop) {

    --m_ScopeDepth;

    int popCount = 0;
    while ((!CurrentFunction()->Locals.empty()) &&
           (CurrentFunction()->Locals[CurrentFunction()->Locals.size() - 1]->Depth > m_ScopeDepth)) {

        VariableInfo *var = CurrentFunction()->Locals.back();
        Destroy(var);

        CurrentFunction()->Locals.pop_back();
        ++popCount;
    }

    if (pop) { // We don't need to pop the stack when returning from a function
        EmitPop(popCount);
    }
}

/* Calls the destructor on the variable if applicable */
void Compiler::Destroy(VariableInfo *variable) {

    if (variable == nullptr)
        return;

    if (variable->Reads < 1) {
        AddWarning("Variable '" + variable->Name + "' is never used.", variable->Token);
    }

    if (variable->IsClassHead()) {
        ScriptFunction *destructor = FindScriptFunction("__" + variable->ParentClass + "__Destructor");
        if (destructor) {
            EmitCallDirect(destructor, variable);
        }
    }
}

void Compiler::ClassDeclaration() {

    Token token = ConsumeToken(tknIdentifier, -1, "Expected class name.");
    string className = token.Value;

    if (ResolveClass(className) != nullptr) {
        AddError("class '" + className + "' already exists.", token);
        return;
    }

    ClassInfo *klass = CreateClass(className);

    ConsumeToken(tknLeftCurly, -2, "Expected '{' before class body.");

    ScriptFunction *initFunc = CreateFunction("__" + klass->Name + "__Init", ftClassInit, dtVoid);
    initFunc->IsParameterless = true;
    klass->InitFunctionId = initFunc->Id;
    initFunc->Args.emplace_back(dtPointer);

    auto thisVar = new VariableInfo();
    thisVar->Name = "this";
    thisVar->ParentClass = klass->Name;
    thisVar->Depth = m_ScopeDepth;
    thisVar->Pointer = VmPointer(CurrentFunction()->Locals.size(), dtPointer, scopeLocal);
    CurrentFunction()->Locals.push_back(thisVar);

    while (!Check(tknRightCurly) && !IsAtEnd()) {
        bool destructor = Match(tknBitwiseNot);
        DataType fieldType;
        u32 fieldFlags;
        if (MatchTypeDeclaration(fieldType, fieldFlags)) {
            if (fieldType == dtClass && LookBack().Value == className) {
                if (destructor) {
                    DestructorDeclaration();
                } else {
                    ConstructorDeclaration();
                }
            } else {
                TypeDeclaration(fieldType, fieldFlags);
            }
        } else {
            AddError("Invalid token inside class declaration.", CurrentToken());
            ConsumeToken();
        }
    }

    EmitReturn();

    EndFunction();


    ConsumeToken(tknRightCurly, -2, "Expected '}' after class body.");

    EndClass();
}

ClassInfo *Compiler::ResolveClass(const string &name) {

    for (auto klass: m_Classes) {
        if (klass->Name == name)
            return klass;
    }

    return nullptr;
}

ClassInfo *Compiler::CreateClass(const string &name) {

    auto *klass = new ClassInfo();
    klass->Token = LookBack();
    klass->Name = name;
    klass->Id = (int) m_Classes.size();
    klass->Enclosing = m_CurrentClass;
    klass->ParentFunctionId = CurrentFunction()->Id;

    m_Classes.push_back(klass);
    m_CurrentClass = klass;

    if (CurrentScope() >= scopeLocal) {
        AddError("Class types cannot be declared inside a local scope.", klass->Token);
    }

    return m_CurrentClass;
}

void Compiler::EndClass() {

    // Check the class has some members
    if (m_CurrentClass->Fields.empty()) {
        AddError("Class body must contain at least one field.", m_CurrentClass->Token);
        m_Classes.pop_back(); // No point keeping the empty class
    }

    // Roll back the current class to it's enclosing parent
    m_CurrentClass = m_CurrentClass->Enclosing;
}

ClassInfo *Compiler::CurrentClass() {

    return m_CurrentClass;
}

void Compiler::ClassInstanceBegin(ClassInfo *classInstance) {

    classInstance->Enclosing = m_CurrentClassInstance;
    m_CurrentClassInstance = classInstance;
}

void Compiler::ClassInstanceEnd() {

    m_CurrentClassInstance = m_CurrentClassInstance->Enclosing;
}

ClassInfo *Compiler::CurrentClassInstance() {

    return m_CurrentClassInstance;
}

/* When inside a class declaration, code emitted from within its parent function is considered
 * initialisation code of the class itself.
 */
bool Compiler::InClassInitialiser() {

    if (!CurrentClass() || CurrentClass()->InitFunctionId < 0)
        return false;

    return CurrentClass()->InitFunctionId == CurrentFunction()->Id;
}

bool Compiler::MatchClassInstance() {

    if (!Check(tknIdentifier)) {
        return false;
    }

    ClassInfo *klass = ResolveClass(CurrentToken().Value);
    if (klass == nullptr) {
        return false;
    }

    return Match(tknIdentifier);
}

void Compiler::ClassInstanceDeclaration() {

    const Token &token = LookBack();
    string className = token.Value;
    ClassInfo *klass = ResolveClass(className);
    ClassInstanceBegin(klass);
    if (klass == nullptr) {
        AddError("class '" + className + "' has not been defined in this scope.", token);
        return;
    }

    // Start Class
    VariableInfo *classVar = ParseVariable(dtClass, vfNormal, "Expected class instance name.");
    if (classVar == nullptr) {
        // Variable probably already exists.
        return;
    }

    MarkInitialised(CurrentScope());

    if (CurrentScope() == scopeLocal) {
        // Push the stack to accommodate the entire class.
        EmitPush(klass->Size());
    }

    /* Initializer */
    ScriptFunction *initFunc = FindScriptFunction("__" + klass->Name + "__Init");
    if (initFunc) {
        // Push the init function and call it
        ConstantInfo funcId(dtFunction, FUNCTION_VAL(initFunc->Id));
        EmitConstant(funcId);

        // Push a pointer to the class onto the stack as the first argument to the method
        EmitAbsolutePointer(classVar);

        EmitCall(OP_CALL, 1);
    } else {
        AddError("Failed to resolve class initialisation for '" + klass->Name + "'.", token);
    }

    /* Constructor */
    if (Match(tknLeftParen)) {
        ScriptFunction *ctorFunc = FindScriptFunction("__" + klass->Name + "__Constructor");
        if (ctorFunc != nullptr) {
            EmitCallDirect(ctorFunc, classVar);
        } else {
            AddError("No constructor provided for class '" + klass->Name + "'.", token);
        }
    } else if (klass->HasConstructor()) {
        AddWarning("Class '" + className + "' has a constructor but is initialized without it.", token);
    }

    ConsumeToken(tknSemiColon, -2, "Expected ';' after class instance declaration.");

    ClassInstanceEnd();
}

void Compiler::ArrayDeclaration(DataType dataType, u32 flags) {

    flags |= vfArray;
    VariableInfo *arrayVar = ParseVariable(dataType, flags);
    string name = arrayVar->Name;
    MarkInitialised(CurrentScope());

    if (arrayVar == nullptr) {
        return;
    }

    // Get the number of values that can be packed into a stack value.
    int packedValueCount = TypeInfo::GetPackedCount(arrayVar->Type());
    int count = NOT_SET;
    int initCount = NOT_SET;

    // Local only: Emit the array instruction to be patched with the array size
    // This pushes the local stack pointer to accommodate the array
    int arrayCodePos = -1;
    if (CurrentScope() == scopeLocal) {
        arrayCodePos = EmitArray();
    }

    ConsumeToken(tknLeftSquareBracket, -2, "Expected '[' after array name.");
    if (Match(tknIntegerLiteral)) {
        count = ScriptUtils::ParseInteger(LookBack().Value);
    } else if (!Check(tknRightSquareBracket)) {
        AddError("Array size must be an integer literal.", CurrentToken());
    }
    ConsumeToken(tknRightSquareBracket, -2, "Expected ']' after array size");

    // Initializer. The first value is already on the stack.
    if (Match(tknAssign) && Match(tknLeftCurly)) {
        initCount = 0;
        TypeInfo arrayType(dataType);
        TypeBegin(&arrayType);

        do {
            if ((initCount > 0) && (initCount % packedValueCount == 0)) {
                // Add a variable to the stack if we're past a pack size boundary
                CreateVariable("__" + name + "__" + std::to_string(initCount), CurrentScope(), dataType,
                                      vfNormal);
            }

            // Find the array pointer again in case the vector has reallocated.
            arrayVar = ResolveVariable(name);

            // Push the pointer to the start of the array onto the stack
            EmitAbsolutePointer(arrayVar);

            // Push the offset onto the stack
            ConstantInfo index(dtInt32, INT32_VAL(initCount));
            EmitConstant(index);

            // Array value expression
            DataType exprType = Expression();

            // Check the type is ok
            auto argCompat = arrayType.CheckCompatibleWith(exprType);
            if (argCompat == tcIncompatible) {
                AddError("Value of type '" + DataTypeToString(dataType) + "' expected.",
                         LookBack());
            } else if (argCompat != tcMatch) {
                AddWarning("Value will be implicitly cast to type '"
                           + DataTypeToString(dataType) + "'. Data may be lost.",
                           LookBack());
            }

            // Set the value
            EmitSetAtOffset(arrayVar->Type(), exprType);
            EmitPop(); // No need to leave a value on the stack when initialising an array.

            ++initCount;
        } while (Match(tknComma) && !IsAtEnd());

        TypeEnd();

        ConsumeToken(tknRightCurly, -2, "Expected '}' after array initialization.");

        if (count != NOT_SET && count != initCount) {
            AddError("Array explicit size and initialized size do not match.", LookBack());
        } else {
            count = initCount;
        }
    } else if (count > 0) {
        // Init the array with zero values
        EmitByte(OP_NIL);
        EmitSetVariable(OP_ASSIGN, arrayVar, dtInt32);
        EmitPop();
        for (int i = packedValueCount; i < count; i += packedValueCount) {
            // Find the array pointer again in case the vector has reallocated.
            arrayVar = ResolveVariable(name);

            // Push the pointer to the start of the array onto the stack
            EmitAbsolutePointer(arrayVar);

            VariableInfo *aVal = CreateVariable("__" + name + "__" + std::to_string(i),
                                                CurrentScope(), dataType, vfNormal);
            if (aVal == nullptr) { // Check the return value of the CreateVariable function
                AddError("Failed to create array value", LookBack());
            }

            EmitByte(OP_NIL);
            EmitSetAtOffset( arrayVar->Type(), dtInt32);
            EmitPop();
        }
    }

    // Ensure the sizes make sense
    if (count <= 0) {
        AddError("Cannot declare array with size of 0.", LookBack());
        return;
    }

    // Resolve the array head again as the vector may have resized
    arrayVar = ResolveVariable(name);
    if (arrayVar == nullptr)
        return;

    // Calculate the size in terms of stack values
    int size = (TypeInfo::GetByteSize(dataType) * count) / (int) sizeof(Value);
    arrayVar->Size = size;

    // Local Only: Patch the array size.
    if (arrayCodePos >= 0) {
        PatchArray(arrayCodePos, arrayVar->Size);
    }

    ConsumeToken(tknSemiColon, -2, "Expected ';' after array declaration.");
}

void Compiler::FunctionDeclaration(DataType dataType) {

    Token token = ConsumeToken(tknIdentifier, -2, "Expected method name.");
    string funcName = token.Value;
    Function(funcName, ftFunction, dataType);
}

void Compiler::MethodDeclaration(DataType dataType) {

    Token token = ConsumeToken(tknIdentifier, -2, "Expected method name.");
    string methodName = "__" + CurrentClass()->Name + "__" + token.Value;
    ScriptFunction *func = Function(methodName, ftClassMethod, dataType);
    CurrentClass()->Methods.push_back(func->Name);
}

void Compiler::ConstructorDeclaration() {

    ScriptFunction *func = Function("__" + CurrentClass()->Name + "__Constructor", ftClassMethod, dtVoid);
    CurrentClass()->Methods.push_back(func->Name);
    CurrentClass()->ConstructorFunctionId = func->Id;
}

void Compiler::DestructorDeclaration() {

    FunctionInfo *func = Function("__" + CurrentClass()->Name + "__Destructor", ftClassMethod, dtVoid);
    func->IsParameterless = true;
    CurrentClass()->Methods.push_back(func->Name);
}

ScriptFunction *Compiler::Function(const string &name, FunctionType chunkType, DataType returnType) {

    ScriptFunction *func = CreateFunction(name, chunkType, returnType);

    ScopeBegin();

    ConsumeToken(tknLeftParen, -2, "Expected '(' after function name.");

    // Insert the hidden 'this' argument for class methods
    if (chunkType == ftClassMethod) {
        VariableInfo *thisVar = CreateVariable("this", scopeLocal, dtPointer,
                                               (VariableFlags)(vfPointer | vfClass | vfConst));
        MarkInitialised(thisVar->Scope());
        CurrentFunction()->Args.push_back(thisVar->Type());
    }

    // Parse user arguments
    if (!Check(tknRightParen)) {
        do {
            if (CurrentFunction()->ArgCount() >= 255) {
                AddError("Can't have more than 255 parameters.", CurrentToken());
            }

            DataType dt;
            u32 flags;
            if (MatchTypeDeclaration(dt, flags)) {
                ClassInfo *classInfo = nullptr;

                if (dt == dtClass) {
                    flags |= vfClass;
                    classInfo = ResolveClass(LookBack().Value);
                    if (!(flags & vfPointer)) {
                        AddError("Classes should be passed by reference instead of value.", LookBack());
                    }
                }
                VariableInfo *arg = ParseVariable(dt, (VariableFlags)flags, "Expected parameter name.");
                if (arg != nullptr) {
                    if ((flags & vfClass) && classInfo != nullptr) {
                        arg->ParentClass = classInfo->Name;
                    }
                    CurrentFunction()->Args.push_back(arg->Type());
                    MarkInitialised(arg->Scope());
                }
            } else {
                AddError("Expected argument type.", CurrentToken());
            }
        } while (Match(tknComma));
    }
    ConsumeToken(tknRightParen, -2, "Expected ')' after parameters.");

    ConsumeToken(tknLeftCurly, -2, "Expected '{' before function body.");
    Block();

    // No need to actually pop the stack when returning from a function.
    ScopeEnd(false);

    // Add implicit return
    ScriptFunction *scriptFunction = func->Type != ftNative ? (ScriptFunction *) func : nullptr;
    if (scriptFunction && !scriptFunction->ReturnSupplied) {
        if (scriptFunction->ReturnType == dtVoid) {
            EmitReturn();
        } else {
            AddError("Function requires a return value.", func->Token);
        }
    }

    uint32_t functionId = EndFunction();
    ConstantInfo funcConst{};
    funcConst.Type = dtFunction;
    funcConst.ConstValue.FuncPointer = functionId;

    AddConstant(funcConst); // No stack effect.
    //EmitConstant(funcConst); // Pushes to stack.

    return func;
}

void Compiler::NativeFunction(const Token &token) {

    const string &name = token.Value;
    NativeFuncInfo *nativeFunc = ResolveNativeFunction(name);
    if (nativeFunc == nullptr) {
        AddError("Failed to resolve native function '" + name + "'.", token);
        return;
    }

    ConstantInfo native{};
    native.Type = dtNativeFunc;
    native.ConstValue.FuncPointer = nativeFunc->Id;

    EmitConstant(native);

    if (!Check(tknLeftParen)) {
        AddError("Expected '(' after " + name, token);
    }
}

int Compiler::ArgumentList(FunctionInfo *func, VariableInfo *parentVar) {

    if (func == nullptr) {
        return 0;
    }

    int expectedArgCount = func->ArgCount();
    int argCount = 0;
    int hiddenArgs = 0;

    // Add the hidden 'this' argument for class methods
    if (func->Type == ftClassMethod) {
        if (parentVar && !parentVar->ParentInstance.empty() && parentVar->MemberIndex == 0) {
            // Push a pointer to the class onto the stack as the first argument to the method
            EmitPointer(parentVar);
            EmitByte(OP_ABSOLUTE_POINTER);
            hiddenArgs++;
        } else {
            AddError("Can't call class method outside of class instance", LookBack());
        }
    }


    if (!func->IsParameterless && !Check(tknRightParen)) {
        do {
            if (argCount >= func->MaxArgs()) {
                AddError("Can't have more than " + std::to_string(func->MaxArgs()) + " arguments.", LookBack());
            }

            if (argCount >= expectedArgCount) {
                Expression(); // At this point the expression is in-valid but we compile it so the rest of the file can be parsed.
                argCount++;
                continue; // Don't bother with type checking.
            }

            TypeInfo argExpectingType(func->Args[argCount + hiddenArgs]);
            TypeBegin(&argExpectingType);

            // Argument expression
            DataType exprType = Expression();
            auto argCompat = argExpectingType.CheckCompatibleWith(exprType);

            if (argCompat == tcIncompatible) {
                AddError("Argument of type '" + DataTypeToString(func->Args.back()) + "' expected.",
                         LookBack());
            }

            // If required, cast the value on the stack to match the function input.
            EmitCast(argCompat);

            TypeEnd();

            argCount++;
        } while (Match(tknComma));
    }

    if (!func->IsParameterless) {
        ConsumeToken(tknRightParen, -2, "Expected ')' after arguments.");
    }

    if (argCount != func->ArgCount()) {
        string fType = func->Type == ftClassMethod ? "Method" : "Function";
        AddError(fType + " expects " + std::to_string(expectedArgCount) +
                 " argument(s), but "
                 + std::to_string(argCount) + " provided.",
                 LookBack());
    }

    return argCount + hiddenArgs;
}

void Compiler::Call() {

    Token token = LookBack(2);
    string name = token.Value;

    FunctionInfo *func = FindFunction(name);
    if (func == nullptr) {
        AddError("Failed to resolve called function.", token);
        return;
    }

    // Store the current call frame on the stack
    if (func->Type != ftNative) {
        EmitByte(OP_FRAME);
    }

    VariableInfo *parentVar = nullptr;
    if (func->Type == ftClassMethod) {
        parentVar = ResolveVariable(LookBack(4).Value);
    }

    int argCount = ArgumentList(func, parentVar);

    EmitBytes(func->Type == ftNative ? OP_CALL_NATIVE : OP_CALL, (u8) argCount);
}

void Compiler::PointerIndex(bool canAssign) {

    // '[' will already be consumed at this point
    DataType arrayType = CurrentType();
    if (arrayType == dtNone) {
        AddError("Unexpected type.", LookBack());
        return;
    }

    // Index expression
    TypeInfo indexTypeInfo(dtInt32);
    TypeBegin(&indexTypeInfo);
    DataType indexType = Expression();
    TypeEnd();

    ConsumeToken(tknRightSquareBracket, -2, "Expected ']' after index expression.");

    // Ensure the expression resolves to a pointer(integer)
    TypeCompatibility cast = TypeInfo::CheckCompatibility(dtInt32, indexType);
    EmitCast(cast);

    // Get or Set
    TokenType assignToken;
    if (canAssign && MatchAssignment(assignToken)) {
        AssignArrayIndex(arrayType, assignToken);
    } else {
        EmitGetFromOffset(arrayType, CurrentType());
    }
}

NativeFuncInfo *Compiler::ResolveNativeFunction(const string &name) {

    auto nf = NativeFunctionMap.find(name);
    if (nf != NativeFunctionMap.end()) {
        // Tag the function name onto the result
        nf->second.Name = name;
        return &nf->second;
    }

    return nullptr;
}

ScriptFunction *Compiler::ResolveMethod(const string &name, VariableInfo *parentVar) {

    if (parentVar == nullptr)
        return nullptr;

    ScriptFunction *method = FindScriptFunction("__" + parentVar->ParentClass + "__" + name);

    if (method != nullptr && method->Type == ftClassMethod)
        return method;

    return nullptr;
}

bool Compiler::MatchTypeDeclaration(DataType &outDataType, u32 &outFlags) {

    outFlags = vfNormal;

    if (Match(tknConst)) {
        outFlags |= vfConst;
    }

    if (Match(tknStar)) {
        outFlags |= vfPointer;
    }

    if (Match(tknVoid)) outDataType = dtVoid;
    else if (Match(tknBool)) outDataType = dtBool;
    else if (Match(tknChar)) outDataType = dtInt8;
    else if (Match(tknByte)) outDataType = dtUint8;
    else if (Match(tknShort)) outDataType = dtInt16;
    else if (Match(tknUShort)) outDataType = dtUint16;
    else if (Match(tknInt)) outDataType = dtInt32;
    else if (Match(tknUInt)) outDataType = dtUint32;
    else if (Match(tknFloat)) outDataType = dtFloat;
    else if (Match(tknString)) outDataType = dtString;
    else if (MatchClassInstance()) outDataType = dtClass;
    else {
        outDataType = dtNone;
        if (outFlags != vfNormal) {
            AddError("Expected type initializer.", LookBack());
        }
    }

    return (outDataType != dtNone);
}

void Compiler::VariableDeclaration(const DataType dataType, u32 flags) {

    TypeInfo varType(dataType);
    TypeBegin(&varType);

    VariableInfo *var = ParseVariable(dataType, flags);
    if (var == nullptr) {
        return;
    }

    DataType inputType = dtNone;
    if (Match(tknAssign)) {
        Token exprToken = CurrentToken();
        inputType = Expression();
        if (inputType != var->Type()) {
            AddWarning("Expression will be implicitly cast to assignee type: "
                        + DataTypeToString(var->Type()), exprToken);
        }
    } else {
        EmitByte(OP_NIL);
        inputType = dtInt32;
    }
    ConsumeToken(tknSemiColon, -2, "Expected ';' after variable declaration.");

    DefineVariable(var, inputType);
    TypeEnd();
}

VariableInfo *Compiler::ParseVariable(const DataType dataType, u32 flags, const string &errorMessage) {

    Token token = ConsumeToken(tknIdentifier, -2, errorMessage);
    VariableInfo *var = DeclareVariable(dataType, flags);
    var->Token = token;
    return var;
}

VariableInfo *Compiler::AddGlobal(const Token &token, const DataType dataType, u32 flags) {

    const string &name = token.Value;

    // Check if it already exists
    if (ResolveNativeFunction(name) != nullptr) {
        AddError("Native function with name '" + name + "' already exists.", token);
        return nullptr;
    }

    if (ResolveGlobal(name) != nullptr) {
        AddError("Variable '" + name + "' already exists.", token);
        return nullptr;
    }
    if (m_Globals.size() >= 0xFFFF) {
        AddError("Maximum global variable count reached (65535).", token);
        return nullptr;
    }

    VariableInfo *newGlobal = CreateVariable(name, scopeGlobal, dataType, flags);
    newGlobal->Token = token;

    return newGlobal;
}

VariableInfo *Compiler::AddLocal(const Token &token, const DataType dataType, u32 flags) {

    const string &name = token.Value;

    // Check if it already exists
    if (ResolveNativeFunction(name) != nullptr) {
        AddError("Native function with name '" + name + "' already exists.", token);
        return nullptr;
    }

    if ((ResolveGlobal(name) != nullptr) || (ResolveLocal(name) != nullptr)) {
        AddError("Variable '" + name + "' already exists.", token);
        return nullptr;
    }
    if (CurrentFunction()->Locals.size() >= 0xFFFF) {
        AddError("Maximum local variable count reached (65535).", token);
        return nullptr;
    }

    VariableInfo *newLocal = CreateVariable(name, scopeLocal, dataType, flags);
    newLocal->Token = token;

    // Record the max local height
    if (CurrentFunction()->Locals.size() > CurrentFunction()->LocalsMaxHeight) {
        CurrentFunction()->LocalsMaxHeight = CurrentFunction()->Locals.size();
    }

    return newLocal;
}

VariableInfo *Compiler::AddMember(const Token &token, DataType dataType, u32 flags) {

    if (CurrentClass() == nullptr) {
        AddError("Cannot add fields outside of a class.", token);
        return nullptr;
    }

    const string &name = token.Value;

    // Check if it already exists
    if (ResolveNativeFunction(name) != nullptr) {
        AddError("Native function with name '" + name + "' already exists.", token);
        return nullptr;
    }

    if ((ResolveGlobal(name) != nullptr) || (ResolveLocal(name) != nullptr)) {
        AddError("Field '" + name + "' already exists.", token);
        return nullptr;
    }
    if (CurrentClass()->Fields.size() >= 0xFFFF) {
        AddError("Class cannot have more than 65535 members.", token);
        return nullptr;
    }

    VariableInfo *newMember = CreateVariable(name, CurrentScope(), dataType, (flags | vfField));
    newMember->Token = token;

    return newMember;
}

VariableInfo *Compiler::AddClassMembers(VarScopeType scope, const string &className, const string &instanceName) {

    ClassInfo *classInfo = ResolveClass(className);
    if (classInfo == nullptr) {
        return nullptr;
    }

    std::vector<VariableInfo *> *varVector;

    if (scope == scopeGlobal) {
        varVector = &m_Globals;
    } else if (scope == scopeLocal) {
        varVector = &CurrentFunction()->Locals;
    } else {
        return nullptr;
    }

    int classAddress = (int) varVector->size();

    for (auto m: classInfo->Fields) {
        int address = (int) varVector->size();
        if (m->Type() == dtClass) {
            AddClassMembers(scope, m->ParentClass, m->Name);
        } else {
            varVector->push_back(m);
        }
        VariableInfo *member = varVector->back();
        member->ParentInstance = instanceName;
        member->ParentAddress = classAddress;
        member->Pointer.Address = address;
        member->Pointer.Scope = scope;
        member->Depth = m_ScopeDepth; // Class members get initialised by the class init function.
    }

    return varVector->at(classAddress);
}

VariableInfo *Compiler::CreateVariable(const string &name, VarScopeType scope, DataType dataType, u32 flags) {

    auto *var = new VariableInfo();
    var->Pointer = VmPointer(0xFFFF, dataType, scope);
    var->Name = name;
    var->Flags = flags;
    if (dataType == dtClass) {
        var->MemberIndex = 0;
    }
    if (InClassInitialiser()) {
        var->ParentClass = CurrentClass()->Name;
        var->Size = 1;
        var->MemberIndex = (int) CurrentClass()->Fields.size();
    } else if (CurrentClassInstance()) {
        var->ParentClass = CurrentClassInstance()->Name;
        var->Size = CurrentClassInstance()->Size(); // ?
    } else {
        var->Size = 1;
    }
    if (var->IsFunction() || dataType == dtFunction || dataType == dtNativeFunc) {
        var->Depth = 0;
    } else {
        var->Depth = NOT_SET; // Set after the variable is initialised.
    }

    VariableInfo *newVar = nullptr;
    std::vector<VariableInfo *> *varVector;

    if (InClassInitialiser()) {
        varVector = &CurrentClass()->Fields;
    } else if (scope == scopeGlobal) {
        varVector = &m_Globals;
    } else if (scope == scopeLocal) {
        varVector = &CurrentFunction()->Locals;
    } else {
        return nullptr;
    }

    // Add class members to the memory stack
    if (dataType == dtClass && CurrentClassInstance()) {
        delete(var); // TODO: Improve this
        newVar = AddClassMembers(scope, CurrentClassInstance()->Name, name);
    } else {
        var->Pointer.Address = varVector->size();
        varVector->push_back(var);
        newVar = var;
    }

    return newVar;
}

VariableInfo *Compiler::ResolveGlobal(const string &name, const string &parent) {

    for (int i = (int) (m_Globals.size() - 1); i >= 0; --i) {
        VariableInfo *var = m_Globals[i];
        if (var->Match(name, parent) || var->IsHeadMemberOf(name)) {
            if (var->Depth == NOT_SET) {
                AddError("Can't read global variable in its own initializer.", LookBack());
            }
            return var;
        }
    }

    return nullptr;
}

VariableInfo *Compiler::ResolveLocal(const string &name, const string &parent) {

    for (int i = (int) (CurrentFunction()->Locals.size() - 1); i >= 0; --i) {
        VariableInfo *var = CurrentFunction()->Locals[i];
        if (var->Match(name, parent) || var->IsHeadMemberOf(name)) {
            if (var->Depth == NOT_SET) {
                AddError("Can't read local variable in its own initializer.", LookBack());
            }
            return var;
        }
    }

    return nullptr;
}

VariableInfo *Compiler::ResolveMember(ClassInfo *parentClass, const string &name) {

    if (parentClass == nullptr) {
        return nullptr;
    }

    for (int i = (int) (parentClass->Fields.size() - 1); i >= 0; --i) {
        VariableInfo *var = parentClass->Fields[i];
        if (var->Name == name) {
            if (var->Depth == NOT_SET) {
                AddError("Can't read local variable in its own initializer.", LookBack());
            }
            return var;
        }
    }

    return nullptr;
}

VariableInfo *Compiler::ResolveVariable(const string &name, const string &parentInstance) {

    VariableInfo *var = nullptr;

    // When compiling a class declaration we want to find its fields
    if (CurrentClass()) {
        var = ResolveMember(CurrentClass(), name);
    }

    // Locals
    if (var == nullptr) {
        var = ResolveLocal(name, parentInstance);
    }

    // Globals
    if (var == nullptr) {
        var = ResolveGlobal(name, parentInstance);
    }

    if (var == nullptr) {
        if (parentInstance.empty()) {
            AddError("Variable '" + name + "' does not exist in the current scope.", LookBack());
        } else {
            AddError("Variable '" + name + "' is not a member of '" + parentInstance + "'.", LookBack());
        }
        return nullptr;
    }

    return var;
}

VariableInfo *Compiler::DeclareVariable(const DataType dataType, u32 flags) {

    Token token = LookBack();

    if (InClassInitialiser()) {
        return AddMember(token, dataType, flags);
    } else {
        switch (CurrentScope()) {
            case scopeGlobal:
                return AddGlobal(token, dataType, flags);
            case scopeLocal:
            case scopeField:
                return AddLocal(token, dataType, flags);
            default:
                return nullptr;
        }
    }
}

void Compiler::DefineVariable(VariableInfo *variable, DataType inputType) {

    if (variable == nullptr)
        return;

    bool global = CurrentScope() <= scopeGlobal;
    MarkInitialised(CurrentScope());
    EmitSetVariable(OP_ASSIGN, variable, inputType);

    if (global) {
        // Globals don't need to be left on the stack when defined.
        EmitByte(OP_POP);
    }
}

void Compiler::MarkInitialised(VarScopeType scope) {

    if (InClassInitialiser()) {
        VariableInfo *memberVar = CurrentClass()->Fields.back();
        if (memberVar->Depth == NOT_SET)
            memberVar->Depth = m_ScopeDepth;
    } else if (scope == scopeGlobal) {
        VariableInfo *globalVar = m_Globals.back();
        if (globalVar->Depth == NOT_SET)
            globalVar->Depth = 0;
    } else {
        VariableInfo *localVar = CurrentFunction()->Locals.back();
        localVar->Depth = m_ScopeDepth;
    }
}

bool Compiler::MatchAssignment(TokenType &outAssignToken) {

    const Token &token = CurrentToken();

    switch (token.TokenType) {
        case tknAssign:
        case tknPlusEquals:
        case tknMinusEquals:
        case tknTimesEquals:
        case tknDivideEquals:
        case tknBitwiseOrEquals:
        case tknBitwiseAndEquals:
            outAssignToken = token.TokenType;
            return Match(token.TokenType);
        default:
            outAssignToken = tknNone;
            return false;
    }
}

void Compiler::AssignVariable(VariableInfo *variable, TokenType assignToken) {

    if (variable == nullptr) {
        AddError("Failed to resolve variable.", LookBack());
        return;
    }

    DataType exprType = CurrentType();
    if (assignToken == tknAssign) {
        exprType = Expression();
    } else {
        // +=, -=, *=, /=, &=, |=, ^=
        EmitGetVariable(variable, CurrentType());
        Binary();
    }

    EmitSetVariable(OP_ASSIGN, variable, exprType);
}

void Compiler::AssignArrayIndex(DataType arrayType, TokenType assignToken) {

    DataType exprType = CurrentType();
    if (assignToken == tknAssign) {
        exprType = Expression();
    } else {
        // +=, -=, *=, /=, &=, |=, ^=

        // Duplicate the pointer and index as it will get popped and used twice.
        EmitByte(OP_DUPLICATE_2);

        EmitGetFromOffset(arrayType, CurrentType());
        Binary();
    }

    if (m_CurrentArray != nullptr) {
        m_CurrentArray->Writes++;
    }

    EmitSetAtOffset(arrayType, exprType);
}

void Compiler::NamedVariable(const Token &token, bool canAssign) {

    string name = token.Value;

    VariableInfo *variable = ResolveVariable(name);
    if (variable == nullptr) {
        return;
    }

    // Check for dot operator
    while (variable && variable->IsClassHead() && Match(tknDot)) {
        const Token &memberToken = ConsumeToken(tknIdentifier, -2, "Expected identifier after '.'.");
        string memberName = memberToken.Value;

        if (CheckMethod(memberToken, variable)) {
            NamedMethod(memberToken, variable);
            return;
        } else {
            variable = ResolveVariable(memberName, variable->ParentInstance);
        }
    }

    if (variable == nullptr) {
        return;
    }

    TypeSetCurrent(variable->Type());

    TypeInfo varType(variable->Type());

    // Check if the variable is an array
    if (variable->IsArray()) {
        EmitAbsolutePointer(variable);
        m_CurrentArray = variable;
        if (!Check(tknLeftSquareBracket)) {
            // Raw Pointer
        }
        return;
    }

    TypeBegin(&varType);

    // Regular variable
    TokenType assignToken;
    if (canAssign && MatchAssignment(assignToken)) { // Get
        AssignVariable(variable, assignToken);
    } else {
        EmitGetVariable(variable, varType.Expecting()); // Set
    }

    TypeEnd();
}

void Compiler::NamedFunction(const Token &token) {

    ConsumeToken(tknLeftParen, -2, "Expected '(' after function name");

    ScriptFunction *func = FindScriptFunction(token.Value);

    EmitCallDirect(func, nullptr);
}

void Compiler::NamedMethod(const Token &token, VariableInfo *parentVar) {

    ConsumeToken(tknLeftParen, -2, "Expected '(' after method name");

    string name = token.Value;

    if (parentVar == nullptr) {
        AddError("Parent of '" + name + "' not found.", token);
        return;
    }

    ScriptFunction *method = ResolveMethod(name, parentVar);

    if (method == nullptr) {
        AddError("Method '" + name + "' is not a member of '" + parentVar->ParentClass + "'.", token);
    }

    EmitCallDirect(method, parentVar);
}

/*
void Compiler::EmitPointer(const Pointer &pointer, u16 offset) {

    u16 address = pointer.Address + offset;
    EmitBytes(pointer.Info.AsByte(), mByte0(address), mByte1(address));
}
*/

void Compiler::EmitGetVariable(VariableInfo *variable, DataType outputType) {

    if (variable == nullptr)
        return;

    variable->Reads++;

    TypeCompatibility cast = TypeInfo::CheckCompatibility(outputType, variable->Type());

    // When referring to class members inside its own declaration, we set the scopes to member.
    // This allows the 'this' keyword to work.
    bool isMember = (CurrentClass() && (variable->ParentClass == CurrentClass()->Name));

    EmitPointer(variable, isMember);

    EmitByte(OP_GET_VARIABLE);

    EmitCast(cast);
}

void Compiler::EmitSetVariable(opCode_t opAssign, VariableInfo *variable, DataType inputType) {

    if (variable == nullptr)
        return;

    if (variable->IsConst() && variable->Writes > 0) {
        AddError("Cannot write to const variable after initialisation.", LookBack());
        return;
    }

    TypeCompatibility cast = TypeInfo::CheckCompatibility(variable->Type(), inputType);

    variable->Writes++;

    // When referring to class members inside its own declaration, we set the scopes to member.
    // This allows the 'this' keyword to work.
    bool isMember = (CurrentClass() && (variable->ParentClass == CurrentClass()->Name));

    EmitCast(cast);

    EmitPointer(variable, isMember);

    EmitByte(opAssign);
}

/*
 * Creates a const value (if required) for the variable pointer and pushes it onto the stack.
 */
void Compiler::EmitPointer(VariableInfo *variable, bool isMember) {

    Value pointer;
    pointer.Pointer = variable->Pointer;
    if (isMember) { // TODO: Check
        pointer.Pointer.Scope = scopeField;
        pointer.Pointer.Address = variable->MemberIndex;
    }
    EmitConstant({dtPointer, pointer});
}

void Compiler::EmitAbsolutePointer(VariableInfo *variable) {

    EmitPointer(variable);
    EmitByte(OP_ABSOLUTE_POINTER);
}

/*
 * Gets a value from an offset from a pointer.
 * Casts the output type if required.
 * Stack Before = ...[pointer][offset]
 */
void Compiler::EmitGetFromOffset(DataType dataType, DataType outputType) {

    TypeCompatibility cast = TypeInfo::CheckCompatibility(outputType, dataType);

    switch (dataType) {
        case dtInt8:
            EmitByte(OP_GET_INDEXED_S8);
            break;
        case dtUint8:
            EmitByte(OP_GET_INDEXED_U8);
            break;
        case dtInt16:
            EmitByte(OP_GET_INDEXED_S16);
            break;
        case dtUint16:
            EmitByte(OP_GET_INDEXED_U16);
            break;
        case dtUint32:
            EmitByte(OP_GET_INDEXED_U32);
            break;
        case dtFloat:
            EmitByte(OP_GET_INDEXED_FLOAT);
            break;
        default:
            EmitByte(OP_GET_INDEXED_S32);
            break;
    }

    if (m_CurrentArray != nullptr) {
        m_CurrentArray->Reads++;
    }

    EmitCast(cast);
}

/*
 * Sets a value at an offset from a pointer.
 * Stack = ...[pointer][offset][value]
 */
void Compiler::EmitSetAtOffset(DataType dataType, DataType inputType) {

    TypeCompatibility cast = TypeInfo::CheckCompatibility(dataType, inputType);
    EmitCast(cast);

    switch (dataType) {
        case dtInt8:
            EmitByte(OP_SET_INDEXED_S8);
            break;
        case dtUint8:
            EmitByte(OP_SET_INDEXED_U8);
            break;
        case dtInt16:
            EmitByte(OP_SET_INDEXED_S16);
            break;
        case dtUint16:
            EmitByte(OP_SET_INDEXED_U16);
            break;
        case dtUint32:
            EmitByte(OP_SET_INDEXED_U32);
            break;
        case dtFloat:
            EmitByte(OP_SET_INDEXED_FLOAT);
            break;
        default:
            EmitByte(OP_SET_INDEXED_S32);
            break;
    }
}

/*
 * Casts the value on top of the stack to a different type.
 * Previous causes the stack top -1 value to be cast.
 * */
void Compiler::EmitCast(TypeCompatibility castMode, bool previous) {

    switch (castMode) {
        case tcCastSignedToFloat:
            EmitByte(previous ? OP_CAST_PREV_INT_TO_FLOAT : OP_CAST_INT_TO_FLOAT);
            break;
        case tcCastFloatToSigned:
            EmitByte(previous ? OP_CAST_PREV_FLOAT_TO_INT : OP_CAST_FLOAT_TO_INT);
            break;
        default:
            break;
    }
}

int Compiler::EmitArray() {

    EmitShortArg(OP_ARRAY, 0xFFFF);

    return CURRENT_CODE_POS - 2;
}

void Compiler::PatchArray(int offset, int size) {

    CurrentFunction()->Code[offset] = mByte0(size);
    CurrentFunction()->Code[offset + 1] = mByte1(size);
}

int Compiler::EmitJump(opCode_t jumpOp) {

    EmitShortArg(jumpOp, 0xFFFF);

    return CURRENT_CODE_POS - 2;
}

void Compiler::PatchJump(int offset) {

    if (CurrentFunction()->Code[offset] != 0xFF && CurrentFunction()->Code[offset + 1] != 0xFF) {
        // Jump has already been patched
        return;
    }

    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = CURRENT_CODE_POS - offset - 2;

    if (jump > UINT16_MAX) {
        AddError("Too much code to jump over.", LookBack());
    }

    CurrentFunction()->Code[offset] = mByte0(jump);
    CurrentFunction()->Code[offset + 1] = mByte1(jump);
}

void Compiler::EmitLoop(int loopStart) {

    EmitByte(OP_LOOP);

    int offset = CURRENT_CODE_POS - loopStart + 2;
    if (offset > UINT16_MAX) AddError("Loop body too large.", LookBack());

    EmitByte(mByte0(offset));
    EmitByte(mByte1(offset));

    //MSG_V("Loop @ " + std::to_string(CURRENT_CODE_POS - 3) + " = " + std::to_string(offset));
}

void Compiler::EmitCall(opCode_t callOp, int argsCount) {

    EmitByte(callOp);
    EmitByte((u8) argsCount);
}

void Compiler::EmitCallDirect(ScriptFunction *function, VariableInfo *parentVar) {

    // Store the stack frame
    EmitByte(OP_FRAME);

    ConstantInfo funcId(dtFunction, FUNCTION_VAL(function->Id));
    EmitConstant(funcId);

    int argCount = ArgumentList(function, parentVar);

    EmitCall(OP_CALL, argCount);
}

void Compiler::EmitReturn() {

    EmitByte(OP_NIL);
    EmitByte(OP_RETURN);
}

u32 Compiler::GetCodeSize() {

    u32 size = 0;
    for (auto &func: m_Functions) {
        if (func == nullptr)
            continue;

        size += func->Code.size();
    }

    return size;
}

void Compiler::SanityCheck() {
    // TODO: Check for unused variables
    for(auto &var : m_Globals) {
        if (var->Name.starts_with("__") && var->Name.ends_with("__")) {
            continue;
        }

        if (var->Writes < 1) {
            AddWarning("Variable '" + var->Name + "' is never assigned.", var->Token);
        }
        if (var->Reads < 1) {
            AddWarning("Variable '" + var->Name + "' is never used.", var->Token);
        }
    }
}

u32 Compiler::CodeSizeInBytes() {

    return (GetCodeSize() * sizeof(opCode_t));
}

u32 Compiler::ConstantsSizeInBytes() {

    return (m_ConstValues.size() * sizeof(Value));
}

u32 Compiler::StringsSizeInBytes() {

    return 0;
}

u32 Compiler::GlobalsSizeInBytes() {

    return (m_Globals.size() * sizeof(Value));
}

void Compiler::GetBuildTimeStamp(uint16_t &outDays, uint16_t &outSeconds) {

    outDays = 8860;
    outSeconds = 4000;
}

/* Updates the function pointers to their actual position in the byte code output. */
bool Compiler::PatchFunctionOffset(const string &name, funcPtr_t functionId, u32 offset) {
    // Top level function doesn't need changing
    if (name.empty())
        return true;

    for (auto &cv: m_ConstValues) {
        if (cv.Type == dtFunction && cv.ConstValue.FuncPointer == functionId) {
            cv.ConstValue.FuncPointer = offset;
            return true;
        }
    }

    return false;
}

StatusCode Compiler::WriteBinaryFile(const string &filePath) {

#define WRITE_BYTE(data)    fileBytes.push_back(data)
#define FILE_POS            fileBytes.size()
#define PADD_BYTES \
while(FILE_POS % 4 > 0) { \
    fileBytes.push_back(0x00); \
}

    uint16_t buildDay;
    uint16_t buildSeconds;
    GetBuildTimeStamp(buildDay, buildSeconds);

    // Write Header
    ScriptBinaryHeader header{
            .HeaderSize = sizeof(ScriptBinaryHeader),
            .HeaderVersion = 0,
            .LangVersionMajor = LANG_VERSION_MAJOR,
            .LangVersionMinor = LANG_VERSION_MINOR,
            .BuildDay = buildDay,
            .BuildTime = buildSeconds,
            .CodePos = 0,
            .ConstantsPos = 0,
            .StringsPos = 0,
            .GlobalsSize = GlobalsSizeInBytes(),
            .TotalSize = 0,
            .CheckSum = 0 // Gets patched at the end
    };

    std::vector<uint8_t> fileBytes;
    fileBytes.reserve(header.HeaderSize + header.CodePos + header.ConstantsPos + header.StringsPos + 64);

    auto *headerBytes = (uint8_t *) &header;
    for (uint32_t i = 0; i < sizeof(ScriptBinaryHeader); ++i) {
        WRITE_BYTE(headerBytes[i]);
    }

    // Write Byte Code
    PADD_BYTES
    u32 codeStart = FILE_POS;
    std::map<funcPtr_t, string> functionsMap;
    for (auto func: m_Functions) {
        if (func == nullptr)
            continue;

        u32 funcPos = FILE_POS - codeStart;
        // Patch function pointers
        if (!PatchFunctionOffset(func->Name, func->Id, funcPos)) {
            AddWarning("Function '" + func->Name + "' is never used", func->Token);
            continue;
        }

        // Output the function to the function map for debugging
        functionsMap.emplace(funcPos, func->Name.empty() ? "<Script>" : func->Name);

        // Function header (skipped for top level)
        if (!func->Name.empty()) {
            WRITE_BYTE(OP_FUNCTION_START);
            WRITE_BYTE((uint8_t) func->ReturnType);
            WRITE_BYTE((uint8_t) func->TotalArgCount());
        }

        // Write function code
        for (auto &code: func->Code) {
            WRITE_BYTE((uint8_t) code);
        }
    }

    // Write Constants Code
    PADD_BYTES
    u32 codeSize = FILE_POS - codeStart;
    u32 constantsStart = FILE_POS;
    for (auto &constVal: m_ConstValues) {
        auto *bytes = (uint8_t *) &constVal.ConstValue;
        for (size_t b = 0; b < sizeof(Value); b++) {
            WRITE_BYTE(bytes[b]);
        }
    }

    // Write Strings Code
    PADD_BYTES
    u32 constantsSize = FILE_POS - constantsStart;
    u32 stringsStart = FILE_POS;
    for (auto &c: m_StringData) {
        WRITE_BYTE((uint8_t) c);
    }
    u32 stringsSize = FILE_POS - stringsStart;

    /* Globals do not need to be in the code */

    // All bytes written
    u32 totalSize = FILE_POS;

    // Patch the start positions and checksum value
    u32 checksum = Checksum::Calculate((fileBytes.data() + codeStart), fileBytes.size() - codeStart);
    auto *fileHeader = (ScriptBinaryHeader *) (fileBytes.data());
    fileHeader->CheckSum = checksum;
    fileHeader->CodePos = codeStart;
    fileHeader->ConstantsPos = constantsStart;
    fileHeader->StringsPos = stringsStart;
    fileHeader->TotalSize = totalSize;

    // Write the final binary file output
    std::ofstream outFile;
    outFile.open(filePath, std::fstream::binary);
    if (!outFile.is_open()) {
        return SetResult(errFileError, "Error writing file: " + filePath);
    }
    for (const auto &byte: fileBytes) {
        outFile << byte;
    }
    outFile.close();

    // Disassembly
    Disassembler disassembler(fileBytes.data(), fileBytes.size());
    disassembler.Disassemble();

    // TODO: Debug file output

    return SetResult(stsBinaryFileDone,
                     "Binary file written: " + filePath + "\n" +
                     "Header:         " + std::to_string(header.HeaderSize) + " bytes\n" +
                     "Code:           " + std::to_string(codeSize) + " bytes\n" +
                     "Constants:      " + std::to_string(constantsSize) + " bytes\n" +
                     "Strings:        " + std::to_string(stringsSize) + " bytes\n" +
                     "Globals:        " + std::to_string(header.GlobalsSize) + " bytes\n" +
                     "Total:          " + std::to_string(totalSize) + " bytes\n" +
                     "Min Slots Size: " + std::to_string((m_LocalsMax * sizeof(Value))) + " bytes\n\r"
    );

#undef WRITE_BYTE
#undef FILE_POS
#undef PADD_BYTES
}

void Compiler::Cleanup() {
    // Functions
    for (const auto func : m_Functions) {
        delete(func);
    }
    m_Functions.clear();

    // Classes
    for (const auto klass : m_Classes) {
        delete(klass);
    }
    m_Classes.clear();

    // Variables
    for (const auto global : m_Globals) {
        delete(global);
    }
    for (const auto shared : m_SharedGlobals) {
        delete(shared);
    }

    m_ConstValues.clear();
    m_ConstStrings.clear();
}

string Compiler::DataTypeToString(const DataType dataType) {

    switch (dataType) {
        case dtNone:
            return "none";
        case dtVoid:
            return "void";
        case dtInt32:
            return "int";
        case dtUint32:
            return "uint";
        case dtFloat:
            return "float";
        case dtBool:
            return "bool";
        case dtInt8:
            return "char";
        case dtUint8:
            return "byte";
        case dtInt16:
            return "short";
        case dtUint16:
            return "ushort";
        case dtString:
            return "string";
        case dtClass:
            return "class";
        case dtFunction:
            return "func";
        case dtNativeFunc:
            return "nativeFunc";
        case dtPointer:
            return "Pointer";
        case dtCppPointer:
            return "CppPointer";
        default:
            return "unknown";
    }
}
