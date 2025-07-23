//
// Created by Declan Walsh on 26/02/2024.
//

#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include "ScriptUtils.h"
#include "Tokens.h"
#include <vector>

enum CompileStage {
    csNotSet = 0,
    csSourceFile,
    csLexer,
    csPreProcessor,
    csParser,
    csInterpreter,
    csAssembler,
    csRuntime,
};

enum StatusCode {
    stsOk = 0,
    stsLexEndOfFile,
    stsParserEndOfTokens,
    stsParserHasErrors,
    stsAsmDone,
    stsCompileDone,
    stsBinaryFileDone,

    // Warnings
    wrnWarning = 100,
    wrnConstOver255,
    wrnConstOver65k,
    errConstLimitReached,
    wrnLexEndOfFileWithErrors,

    // Errors
    errPanicSync = 200,
    errError,
    errFileError,
    errLexError,
    errPreProcessError,
    errParserError,
    errSyntaxError,
    errAsmError,
    errFunctionLinkingError
};

struct CompilerMessage {
    CompileStage Source  = csNotSet;
    StatusCode Code      = stsOk;
    bool ShowImmediately = false;
    bool Shown           = false;
    size_t FilePos       = 0;
    size_t LineNum       = 0;
    size_t LinePos       = 0;
    Token *token         = nullptr;
    std::string Message;
};

class ErrorHandler
{
  public:
    explicit ErrorHandler(const std::string &script);
    ~ErrorHandler() = default;

    static bool IsError(StatusCode code);
    static bool IsWarning(StatusCode code);

    size_t Watermark = 100;
    size_t MessageCount();
    size_t WarningCount();
    size_t ErrorCount();
    bool ErrorsOverwater();

    void SetScript(const std::string &script);
    void AddMessage(CompilerMessage &message);

    void PrintErrors();
    void PrintWarnings();
    void PrintMessages();
    void PrintAll();

    std::vector<CompilerMessage> Messages();
    std::vector<CompilerMessage> Warnings();
    std::vector<CompilerMessage> Errors();

  private:
    std::string_view m_Script;

    std::vector<CompilerMessage> m_Messages;
    std::vector<CompilerMessage> m_Warnings;
    std::vector<CompilerMessage> m_Errors;

    std::string GetLine(size_t lineNum);
    void PrintMessage(CompilerMessage *message);

    std::string FormattedErrorMessage(const std::string &line, size_t lineNum, size_t errorPos, const std::string &message);
};

#endif // ERRORHANDLER_H
