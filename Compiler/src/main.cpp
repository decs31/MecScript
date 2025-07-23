// ScriptUtils

#include "Console.h"
#include "Disassembler.h"
#include "Native.h"
#include "Options.h"
#include "ScriptInfo.h"
#include "compiler/Compiler.h"
#include "lexer/Lexer.h"
#include "preprocessor/PreProcessor.h"
#include "utils/ScriptUtils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <winerror.h>

int main(int argc, char *argv[])
{
    std::filesystem::path inputFilePath;
    std::filesystem::path outputFilePath;
    std::filesystem::path nativeFuncFilePath;
    u8 flags = 0;

    { // Read args
        int i = 1;
        while (i < argc) {
            std::string arg = argv[i++];

            if (arg.substr(0, 2) == "-v") { // Verbose output
                MSG("Verbose Output = On");
                ScriptUtils::VerboseOutput = true;
            } else if (arg.substr(0, 2) == "-f") { // Embed the filename in the output binary
                MSG("Embed file name = On");
                flags |= cfEmbeddedFileName;
            } else if (arg.substr(0, 2) == "-n") { // Native function file
                if (i >= argc) {
                    ERR("Expected native function file path!");
                    exit(ERROR_INVALID_FUNCTION);
                }
                nativeFuncFilePath = argv[i++];
            } else if (inputFilePath.empty()) { // Input path
                inputFilePath = arg;
            } else if (outputFilePath.empty()) { // Output path
                outputFilePath = arg;
            }
        }
    }

    // If a native function file is provided, load it
    std::string nativeFuncScript;

    if (!nativeFuncFilePath.empty()) {
        MSG("Loading native functions from: " << nativeFuncFilePath);

        std::ifstream nfInput(nativeFuncFilePath, std::fstream::in);

        if (!nfInput.good()) {
            ERR("File does not exist or cannot be opened: " << nativeFuncFilePath);
            exit(ERROR_FILE_NOT_FOUND);
        }

        // Begin reading the input file
        MSG("Reading Native Function input file: " << nativeFuncFilePath);
        std::stringstream nfBuffer;
        nfBuffer << nfInput.rdbuf();
        nativeFuncScript = nfBuffer.str();

        if (nativeFuncScript.empty()) {
            ERR("Native Function Script is empty.");
            exit(ERROR_INVALID_DATA);
        }
    }

    ErrorHandler nativeErrorHandler(nativeFuncScript);
    NativeFunctionParser nativeFuncs(&nativeErrorHandler, nativeFuncScript);
    StatusCode nativeParseResult = nativeFuncs.Parse();

    if (nativeParseResult != stsOk) {
        ERR("Error parsing native functions");
        exit(ERROR_INVALID_DATA);
    }

    // Check Input File
    if (inputFilePath.empty()) {
        ERR("Incorrect usage!");
        ERR("Correct usage is: " << COMPILER_NAME << " <script." << SCRIPT_EXTENSION << ">");
        exit(ERROR_INVALID_FUNCTION);
    }

    std::ifstream input(inputFilePath, std::fstream::in);

    if (!input.good()) {
        ERR("File does not exist or cannot be opened: \"" << inputFilePath << "\"");
        exit(ERROR_FILE_NOT_FOUND);
    }

    // Begin reading the input file
    MSG("Reading input file: \"" << inputFilePath << "\"");
    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string script = buffer.str();

    if (script.empty()) {
        ERR("Script is empty.");
        exit(ERROR_INVALID_DATA);
    }

    MSG("File length: " << script.length());

    // If no output file path provided, make one from the input file.
    if (outputFilePath.empty()) {
        outputFilePath = inputFilePath;
        outputFilePath.replace_extension(OUTPUT_EXTENSION);
    }

    // Get the filename to embed in the output
    std::string outputFileName = outputFilePath.stem().string();

    // Start Compiler
    ErrorHandler errorHandler(script);
    Compiler compiler(&errorHandler, &nativeFuncs, script, flags, outputFileName);

    StatusCode compile = compiler.Compile();

    if (compile == stsCompileDone) {
        MSG("Compile complete!\n" << compiler.Message());
    } else {
        ERR("Compile failed!\n" << compiler.Message());
    }

    input.close();

    errorHandler.PrintAll();
    if (errorHandler.ErrorCount() > 0) {
        exit(ERROR_INVALID_DATA);
    }

    // Compile to Byte Code
    StatusCode writeFile = compiler.WriteBinaryFile(outputFilePath.string());
    if (writeFile == stsBinaryFileDone) {
        MSG("Success!");
        MSG(compiler.Message());
    } else {
        ERR(compiler.Message());
        exit(ERROR_FILE_INVALID);
    }

    return 0;
}