//
// Created by Declan Walsh on 13/03/2024.
//

#ifndef COMPILER_H_
#define COMPILER_H_

#include "CompilerData.h"
#include "MecScriptBase.h"
#include "ErrorHandler.h"
#include "Lexer.h"
#include "Rules.h"
#include "TypeSystem.h"
#include "Function.h"
#include "Variable.h"
#include "Class.h"
#include "../preprocessor/PreProcessor.h"


class Compiler : public MecScriptBase {
public:
    explicit Compiler(ErrorHandler *errorHandler, const string &script, const u8 flags = 0, const string &fileName = "");
    ~Compiler();

    StatusCode Compile();

    uint32_t CodeSizeInBytes();
    uint32_t ConstantsSizeInBytes();
    uint32_t StringsSizeInBytes();
    uint32_t GlobalsSizeInBytes();

    StatusCode WriteBinaryFile(const string &filePath);

    StatusCode Result();
    string Message();

    static string DataTypeToString(DataType dataType);

private:
    static Compiler *m_Compiler;
    u8 m_Flags = 0;
    string m_TopLevelFileName;
    Lexer m_Lexer;
    PreProcessor m_PreProcessor;
    size_t m_CurrentPos = 0;
    StatusCode m_Result = stsOk;
    Token m_TokenCurrent = {};
    Token m_TokenPrev = {};

    std::vector<ConstantInfo> m_ConstValues;

    /* Tokens */
    bool Expect(const Token &token, TokenType expect, int errorOffset = -2, const string &errorMsg = "");

    bool Check(TokenType tokenType);
    bool CheckAhead(TokenType tokenType, int num = 1);
    bool CheckNativeFunction(const Token &token);
    bool CheckFunction(const Token &token);
    bool CheckMethod(const Token &token, VariableInfo *parentVar);

    bool Match(TokenType tokenType);


    bool IsSkippable(const Token &token);

    /* Get the Token at the current position */
    Token CurrentToken();

    /* Get the Token at the current position then advance the position */
    Token ConsumeToken(TokenType expect = tknNone, int errorOffset = -2, const string &errorMsg = "");

    /* Get the Token at the current position then advance the position */
    void AdvanceToken(TokenType expect = tknNone, int errorOffset = -2, const string &error = "");

    /* Get the Token at the specified position */
    Token TokenAt(size_t pos);

    /* Get the Token at a number of positions ahead of the current position */
    Token LookAhead(int num = 1);

    Token LookBack(int num = 1);

    bool IsAtEnd();

    /* Classes */
    ClassInfo *m_CurrentClass = nullptr;
    ClassInfo *CreateClass(const string &name);
    void EndClass();
    std::vector<ClassInfo *> m_Classes;
    ClassInfo *CurrentClass();
    ClassInfo *m_CurrentClassInstance = nullptr;
    void ClassInstanceBegin(ClassInfo *classInstance);
    void ClassInstanceEnd();
    ClassInfo *CurrentClassInstance();
    bool InClassInitialiser();

    /* Functions */
    std::vector<ScriptFunction *> m_Functions;
    ScriptFunction *m_CurrentFunction = nullptr;
    ScriptFunction *CreateFunction(const string &name, FunctionType type, DataType returnType);
    ScriptFunction *CurrentFunction();
    ScriptFunction *FindFunctionById(int chunkId);
    FunctionInfo *FindFunction(const string &name);
    ScriptFunction *FindScriptFunction(const string &name);
    ScriptFunction *ResolveMethod(const string &name, VariableInfo *parentVar);
    int EndFunction();
    void ConditionalBegin();
    void ConditionalEnd();

    /* Strings */
    std::vector<StringData> m_ConstStrings;
    std::vector<char> m_StringData;

    /* Variables */
    std::vector<VariableInfo *> m_SharedGlobals;
    std::vector<VariableInfo *> m_Globals;
    int m_ScopeDepth = 0;
    u32 m_LocalsMax = 0;
    VariableInfo *m_CurrentArray = nullptr;
    VarScopeType CurrentScope() const;

    VariableInfo *CreateVariable(const string &name, VarScopeType scope, DataType dataType, u32 flags);

    TypeInfo *m_CurrentType = nullptr;
    DataType TypeBegin(TypeInfo *typeInfo);
    DataType TypeSetCurrent(DataType type, bool force = false);
    DataType CurrentType();
    TypeCompatibility TypeCheck(DataType type, const string &errorMessage = "");
    DataType TypeEnd();

    VariableInfo *AddGlobal(const Token &token, DataType dataType, u32 flags);
    VariableInfo *AddLocal(const Token &token, DataType dataType, u32 flags);
    VariableInfo *AddMember(const Token &token, DataType dataType, u32 flags);
    VariableInfo *AddClassMembers(VarScopeType scope, const string &className, const string &instanceName);
    ClassInfo *ResolveClass(const string &name);
    VariableInfo *ResolveGlobal(const string &name, const string &parent = "");
    VariableInfo *ResolveLocal(const string &name, const string &parent = "");
    VariableInfo *ResolveMember(ClassInfo *parentClass, const string &name);
    VariableInfo *ResolveVariable(const string &name, const string &parentInstance = "");
    void Destroy(VariableInfo *variable);
    u32 AddConstant(const ConstantInfo &constant);
    u32 AddString(const string &str);
    bool MatchClassInstance();
    bool MatchTypeDeclaration(DataType &outDataType, u32 &outFlags);
    VariableInfo *ParseVariable(DataType dataType, u32 flags, const string &errorMessage = "");
    void NamedVariable(const Token &token, bool canAssign);
    void NamedFunction(const Token &token);
    void NamedMethod(const Token &token, VariableInfo *parentVar);
    VariableInfo *DeclareVariable(DataType dataType, u32 flags);
    void DefineVariable(VariableInfo *variable, DataType inputType);
    void MarkInitialised(VarScopeType scope);
    bool MatchAssignment(TokenType &outAssignToken);
    void AssignVariable(VariableInfo *variable, TokenType assignToken);
    void AssignArrayIndex(DataType arrayType, TokenType assignToken);

    /* Scope */
    void ScopeBegin();
    int DiscardLocals(int depth);
    void ScopeEnd(bool pop = true);

    /* Parser Functions */
    void Declaration();
    void Statement();
    DataType Expression();
    ConstantInfo ParseNumericLiteral();
    void NumericLiteral();
    void String();
    void Variable(bool canAssign);
    void And();
    void Or();
    void Call();
    void PointerIndex(bool canAssign);
    int ArgumentList(FunctionInfo *func, VariableInfo *parentVar);
    void VariablePrefix();
    void VariablePostfix(bool canAssign);
    void Ternary();
    void Unary();
    void Binary();
    void Grouping();
    void Block();
    void ParsePrecedence(Precedence precedence);

    void RunParserFunction(ParseFunc func, bool canAssign);

    /* Statements */
    void ClassDeclaration();
    void TypeDeclaration(DataType dataType, u32 flags);
    void ArrayDeclaration(DataType dataType, u32 flags);
    void ClassInstanceDeclaration();
    void FunctionDeclaration(DataType dataType);
    void MethodDeclaration(DataType dataType);
    void VariableDeclaration(DataType dataType, u32 flags);
    void ConstructorDeclaration();
    void DestructorDeclaration();
    void IfStatement();
    void ReturnStatement();
    void WhileStatement();
    void ForStatement();
    void SwitchStatement();
    void BreakStatement();
    void ContinueStatement();
    void ExpressionStatement();

    SwitchInfo *m_CurrentSwitch = nullptr;
    void SwitchAsIfElse();
    void SwitchBegin(SwitchInfo *switchInfo);
    void SwitchBody();
    void SwitchEnd();


    /* Loops */
    LoopInfo *m_CurrentLoop = nullptr;
    void LoopBegin(LoopInfo *loop);
    void LoopBody();
    void LoopTestExit();
    void LoopEnd();

    /* Functions */
    NativeFuncInfo *ResolveNativeFunction(const string &name);
    ScriptFunction *Function(const string &name, FunctionType chunkType, DataType returnType);
    bool PatchFunctionOffset(const string &name, funcPtr_t functionId, u32 offset);
    void NativeFunction(const Token &token);

    /* Byte Code Output */
    void EmitByte(opCode_t byte);
    void EmitBytes(opCode_t byte0, opCode_t byte1);
    void EmitBytes(opCode_t byte0, opCode_t byte1, opCode_t byte2);
    void EmitBytes(opCode_t byte0, opCode_t byte1, opCode_t byte2, opCode_t byte3);
    void EmitShortArg(opCode_t code, int arg);
    void EmitIntArg(opCode_t code, int arg);
    void EmitPush(int count = 1);
    void EmitPop(int count = 1);
    void EmitConstant(const ConstantInfo &constant);
    void EmitGetVariable(VariableInfo *variable, DataType outputType);
    void EmitSetVariable(opCode_t assignOp, VariableInfo *variable, DataType inputType);
    void EmitPointer(VariableInfo *variable, bool isMember = false);
    void EmitAbsolutePointer(VariableInfo *variable);
    void EmitGetFromOffset(DataType dataType, DataType outputType);
    void EmitSetAtOffset(DataType dataType, DataType inputType);
    void EmitCast(TypeCompatibility castMode, bool previous = false);
    void EmitAdd(DataType type);
    void EmitSubtract(DataType type);
    void EmitMultiply(DataType type);
    void EmitDivide(DataType type);
    void EmitEqual(DataType type);
    void EmitNotEqual(DataType type);
    void EmitLessThan(DataType type);
    void EmitLessThanOrEqual(DataType type);
    void EmitGreaterThan(DataType type);
    void EmitGreaterThanOrEqual(DataType type);
    int EmitArray();
    void EmitString(const string &str);
    void PatchArray(int offset, int size);
    int EmitJump(opCode_t jumpOp);
    void PatchJump(int offset);
    int EmitShort(int value);
    void PatchShort(int offset, int value);
    int EmitInt(int value);
    void PatchInt(int offset, int value);
    void EmitLoop(int loopStart);
    void EmitCall(opCode_t callOp, int argsCount);
    void EmitCallDirect(ScriptFunction *function, VariableInfo *parentVar);
    void EmitReturn();

    void EndCompile();
    void SanityCheck();
    u32 GetCodeSize();
    u32 CalculateChecksum(const u8 *data, u32 length);
    static void GetBuildTimeStamp(uint16_t &outDays, uint16_t &outSeconds);

    /* Error Handling */
    bool m_PanicMode = false;
    void AddError(string errMsg, const Token &token);
    void AddError(string errMsg, size_t lineNum, size_t linePos);
    void AddWarning(string warningMsg, const Token &token);
    void AddWarning(string waringMsg, size_t lineNum, size_t linePos);
    void Synchronize();

    /* Result */
    StatusCode SetResult(StatusCode result, string message = "");

    /* Cleanup */
    void Cleanup();
};


#endif //COMPILER_H_
