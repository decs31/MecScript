//
// Created by Declan Walsh on 26/02/2024.
//

#include "ErrorHandler.h"
#include "Console.h"

ErrorHandler::ErrorHandler(const string &script)
{
    SetScript(script);
}

void ErrorHandler::SetScript(const string &script)
{
    m_Script = script;
}

void ErrorHandler::AddMessage(CompilerMessage &message)
{
    if (message.ShowImmediately) {
        PrintMessage(&message);
    }

    if (message.Code >= errError)
        m_Errors.push_back(message);
    else if (message.Code >= wrnWarning)
        m_Warnings.push_back(message);
    else
        m_Messages.push_back(message);
}

std::vector<CompilerMessage> ErrorHandler::Messages()
{
    return m_Messages;
}

std::vector<CompilerMessage> ErrorHandler::Warnings()
{
    return m_Warnings;
}

std::vector<CompilerMessage> ErrorHandler::Errors()
{
    return m_Errors;
}

size_t ErrorHandler::MessageCount()
{
    return m_Messages.size();
}

size_t ErrorHandler::WarningCount()
{
    return m_Warnings.size();
}

size_t ErrorHandler::ErrorCount()
{
    return m_Errors.size();
}

bool ErrorHandler::ErrorsOverwater()
{
    return m_Errors.size() >= Watermark;
}

string ErrorHandler::GetLine(size_t lineNum)
{
    // Fist line is always 1
    if (lineNum < 1)
        lineNum = 1;

    size_t length = m_Script.length();
    size_t i;
    size_t l = 1;

    for (i = 0; i < length; i++) {
        if (l == lineNum) {
            break;
        }
        if (m_Script[i] == '\n') {
            l ++;
        }
    }

    string line;
    while (i < length && m_Script[i] != '\n') {
        line += m_Script[i];
        i ++;
    }

    return line;
}

void ErrorHandler::PrintMessage(CompilerMessage *message)
{
    string msg = FormattedErrorMessage(GetLine(message->LineNum), message->LineNum, message->LinePos, message->Message);

    if (IsError(message->Code)) {
        ERR(msg);
    } else {
        MSG(msg);
    }

    message->Shown = true;
}

bool ErrorHandler::IsError(StatusCode code)
{
    return code >= errError;
}

bool ErrorHandler::IsWarning(StatusCode code)
{
    return code >= wrnWarning && code < errError;
}

string ErrorHandler::FormattedErrorMessage(const std::string &line, size_t lineNum, size_t errorPos, const std::string &message)
{
    std::string err = "Line " + std::to_string(lineNum) + " , Pos " + std::to_string(errorPos) + ":\n";
    err += line;
    if (err.at(err.length() - 1) != '\n')
        err += '\n';

    for (size_t i = 0; i < errorPos - 1; i ++) {
        err += ' ';
    }
    err += "^-- Here\n";
    err += message;
    err += '\n';
    return err;
}

void ErrorHandler::PrintErrors() {
    if (ErrorCount() == 0) {
        MSG("---- " << ErrorCount() << " Error(s) ----");
        return;
    }

    ERR("---- " << ErrorCount() << " Error(s) ----");
    for (auto &err: m_Errors) {
        PrintMessage(&err);
    }
    if (ErrorCount() > 0) {
        MSG("");
    }
}

void ErrorHandler::PrintWarnings() {
    MSG("---- " << WarningCount() << " Warnings(s) ----");
    for (auto &warn: m_Warnings) {
        PrintMessage(&warn);
    }
    if (WarningCount() > 0) {
        MSG("");
    }
}

void ErrorHandler::PrintMessages() {
    MSG("---- " << MessageCount() << " Message(s) ----");
    for (auto &msg: m_Messages) {
        PrintMessage(&msg);
    }
    if (MessageCount() > 0) {
        MSG("");
    }
}

void ErrorHandler::PrintAll()
{
    MSG("");
    PrintErrors();
    PrintWarnings();
    PrintMessages();
    MSG("");
}
