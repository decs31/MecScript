//
// Created by Declan Walsh on 25/07/2025.
//

#include "Console.h"
#include "Disassembler.h"
#include "Options.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <winerror.h>

int main(int argc, char *argv[])
{
    std::string inputFilePath;

    // Read args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        // Verbose
        if (arg == "-v") {
            MSG("Verbose Output = On");
            Console::VerboseOutput = true;
        }
        // Input path
        else if (inputFilePath.empty()) {
            inputFilePath = arg;
        }
    }

    if (inputFilePath.empty()) {
        ERR("Incorrect usage!");
        ERR("Correct usage is: MecDecompile <file." << OUTPUT_EXTENSION << ">");
        exit(ERROR_INVALID_FUNCTION);
    }

    MSG_V("====== MecScript Decompiler ======");

    std::ifstream scriptFile(inputFilePath, std::fstream::binary);

    if (!scriptFile.good()) {
        ERR("File does not exist or cannot be opened: \"" << inputFilePath << "\"");
        exit(ERROR_FILE_NOT_FOUND);
    }

    MSG_V("Reading input file: \"" << inputFilePath << "\"");
    std::vector<unsigned char> scriptData(std::istreambuf_iterator<char>(scriptFile), {});

    if (scriptData.empty()) {
        ERR("Program binary is empty.");
        exit(ERROR_INVALID_DATA);
    }

    Disassembler disassembler(scriptData.data(), scriptData.size());
    disassembler.Disassemble();

    scriptFile.close();

    return 0;
}