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
// front end
    std::map<unsigned int, std::string> FileContents;
    std::vector<Token> ProgramTokens;
    std::unique_ptr<ProgramNode> ProgramAST;
    int ExitCode;
// compiler info
    fs::path InputFile;
    fs::path OutputFile;
    int OptimizationLevel = 0;
    bool RunAfterCompile = false;
    bool EmitWarnings = false;
    std::string CompilerTarget = "";
// debug
    bool Debug = false;
    bool Verbose = false;
    bool Check = false;
    bool DumpIR = false;
    bool DumpASM = false;
    bool DumpBIN = false;
    bool DumpVec = false;
    bool DumpOp = false;
    bool DumpSym = false;
    bool DumpMem = false;
    bool DumpMod = false;
    bool DumpTokens = false;
    bool DumpAST = false;
    bool DumpVIR = false;
    bool DumpBC = false;
    bool DumpVBC = false;
// menu
    bool UsingMenu = false;
    bool HelpMenu = false;
    bool VersionMenu = false;
    bool TargetsMenu = false;
};

std::unique_ptr<CLIObject> CommandLineHandler(int argc, char* argv[]);