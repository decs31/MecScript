// ScriptUtils

#include <iostream>
#include <fstream>
#include <filesystem>
#include <winerror.h>
#include "Console.h"
#include "Options.h"
#include "utils/ScriptUtils.h"
#include "lexer/Lexer.h"
#include "compiler/Compiler.h"
#include "preprocessor/PreProcessor.h"
#include "Disassembler.h"


int main(int argc, char* argv[])
{
    std::string inputFilePath;
    std::string outputFilePath;

    // Read args
    for(int i = 1; i < argc; i ++) {
        std::string arg = argv[i];
        // Verbose
        if (arg.substr(0, 2) == "-v") {
            MSG("Verbose Output = On");
            ScriptUtils::VerboseOutput = true;
        }
        // Input path
        else if (inputFilePath.empty()) {
            inputFilePath = arg;
        }
        // Output path
        else if (outputFilePath.empty()) {
            outputFilePath = arg;
        }
    }

    if (inputFilePath.empty()) {
        ERR("Incorrect usage!");
        ERR("Correct usage is: " << COMPILER_NAME << " <script." << SCRIPT_EXTENSION << ">");
        exit(ERROR_INVALID_FUNCTION);
    }

	MSG("Starting ScriptUtils...");

    std::ifstream input(inputFilePath, std::fstream::in);

    if (!input.good()) {
        ERR("File does not exist or cannot be opened: \"" << inputFilePath << "\"");
        exit(ERROR_FILE_NOT_FOUND);
    }

    MSG("Reading input file: \"" << inputFilePath << "\"");
    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string script = buffer.str();

    if (script.empty()) {
        ERR("Script is empty.");
        exit(ERROR_INVALID_DATA);
    }

    MSG("File length: " << script.length());

    // Start Compiler
    ErrorHandler errorHandler(script);
    Compiler compiler(&errorHandler, script);

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
    // If no output file path provided, make one from the input file.
    if (outputFilePath.empty()) {
        size_t i;
        for(i = inputFilePath.length() - 1; i > 0; i --) {
            if(inputFilePath.at(i) == '.')
                break;
        }
        outputFilePath = inputFilePath.substr(0, i) + "." + OUTPUT_EXTENSION;
    }

    StatusCode writeFile = compiler.WriteBinaryFile(outputFilePath);
    if (writeFile == stsBinaryFileDone) {
        MSG("Success!");
        MSG(compiler.Message());
    } else {
        ERR(compiler.Message());
        exit(ERROR_FILE_INVALID);
    }

	return 0;
}