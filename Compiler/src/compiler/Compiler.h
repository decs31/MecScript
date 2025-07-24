//
// Created by Declan Walsh on 13/03/2024.
//

#ifndef COMPILER_H_
#define COMPILER_H_

#include "../preprocessor/PreProcessor.h"
#include "Class.h"
#include "CompilerBase.h"
#include "CompilerData.h"
#include "ErrorHandler.h"
#include "Function.h"
#include "Lexer.h"
#include "Native.h"
#include "Rules.h"
#include "TypeSystem.h"
#include "Variable.h"

class Compiler : public CompilerBase
{
  public:
    explicit Compiler(ErrorHandler *errorHandler,
                      NativeFunctionParser *nativeFuncs,
                      const std::string &script,
                      const u8 flags              = 0,
                      const std::string &fileName = "");
    ~Compiler();

    StatusCode Compile();

    uint32_t CodeSizeInBytes();
    uint32_t ConstantsSizeInBytes();
    uint32_t StringsSizeInBytes();
    uint32_t GlobalsSizeInBytes();

    StatusCode WriteBinaryFile(const std::string &filePath);

    StatusCode Result();
    std::string Message();

    static std::string DataTypeToString(DataType dataType);

  private:
    static Compiler *m_Compiler;
    NativeFunctionParser *m_NativeFuncs;
    u8 m_Flags;
    std::string m_TopLevelFileName;
    PreProcessor m_PreProcessor;
    StatusCode m_Result = stsOk;

    std::vector<ConstantInfo> m_ConstValues;

    bool CheckFunction(const Token &token);
    bool CheckMethod(const Token &token, VariableInfo *parentVar);
    bool CheckNativeFunction(const Token &token);

    /* Classes */
    ClassInfo *m_CurrentClass = nullptr;
    ClassInfo *CreateClass(const std::string &name);
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
    ScriptFunction *CreateFunction(const std::string &name, FunctionType type, DataType returnType);
    ScriptFunction *CurrentFunction();
    ScriptFunction *FindFunctionById(int chunkId);
    FunctionInfo *FindFunction(const std::string &name);
    ScriptFunction *FindScriptFunction(const std::string &name);
    ScriptFunction *ResolveMethod(const std::string &name, VariableInfo *parentVar);
    int EndFunction();
    void ConditionalBegin();
    void ConditionalEnd();

    /* Strings */
    std::vector<StringData> m_ConstStrings;
    std::vector<char> m_StringData;

    /* Variables */
    std::vector<VariableInfo *> m_SharedGlobals;
    std::vector<VariableInfo *> m_Globals;
    int m_ScopeDepth             = 0;
    u32 m_LocalsMax              = 0;
    VariableInfo *m_CurrentArray = nullptr;
    VarScopeType CurrentScope() const;

    VariableInfo *CreateVariable(const std::string &name, VarScopeType scope, DataType dataType, u32 flags);

    TypeInfo *m_CurrentType = nullptr;
    DataType TypeBegin(TypeInfo *typeInfo);
    DataType TypeSetCurrent(DataType type, bool force = false);
    DataType CurrentType();
    TypeCompatibility TypeCheck(DataType type, const std::string &errorMessage = "");
    DataType TypeEnd();

    VariableInfo *AddGlobal(const Token &token, DataType dataType, u32 flags);
    VariableInfo *AddLocal(const Token &token, DataType dataType, u32 flags);
    VariableInfo *AddMember(const Token &token, DataType dataType, u32 flags);
    VariableInfo *AddClassMembers(VarScopeType scope, const std::string &className, const std::string &instanceName);
    ClassInfo *ResolveClass(const std::string &name);
    VariableInfo *ResolveGlobal(const std::string &name, const std::string &parent = "");
    VariableInfo *ResolveLocal(const std::string &name, const std::string &parent = "");
    VariableInfo *ResolveMember(ClassInfo *parentClass, const std::string &name);
    VariableInfo *ResolveVariable(const std::string &name, const std::string &parentInstance = "");
    void Destroy(VariableInfo *variable);
    u32 AddConstant(const ConstantInfo &constant);
    u32 AddString(const std::string &str);
    bool MatchClassInstance();
    VariableInfo *ParseVariable(DataType dataType, u32 flags, const std::string &errorMessage = "");
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
    NativeFuncInfo *ResolveNativeFunction(const std::string &name);
    ScriptFunction *Function(const std::string &name, FunctionType chunkType, DataType returnType);
    bool PatchFunctionOffset(const std::string &name, funcPtr_t functionId, u32 offset);
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
    void EmitString(const std::string &str);
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

    /* Result */
    StatusCode SetResult(StatusCode result, std::string message = "");

    /* Cleanup */
    void Cleanup();
};

#endif // COMPILER_H_
