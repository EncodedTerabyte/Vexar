#pragma once

#include "MiddleEnd/ProgramParser.hh"
#include "Token.hh"
#include "Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "Miscellaneous/LoggerHandler/ColorPrint.hh"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

struct CLIObject {
public:
    bool PrintTokens = false;
    bool PrintAST = false;

    std::map<unsigned int, std::string> FileContents;
    std::vector<Token> ProgramTokens;
    std::unique_ptr<ProgramNode> ProgramAST;
    int ExitCode;

    fs::path InputFile;
    fs::path OutputFile;
    int OptimizationLevel = 0;
    bool Debug = false;
    bool Verbose = false;
    bool RunAfterCompile = false;

    bool UsingMenu = false;
    bool HelpMenu = false;
    bool VersionMenu = false;
    
    std::string CompilerTarget = "";
};

std::unique_ptr<CLIObject> CommandLineHandler(int argc, char* argv[]);