#include "Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "Miscellaneous/LoggerHandler/ColorPrint.hh"

#include "FrontEnd/FileSerializer.hh"
#include "FrontEnd/Tokenizer.hh"

#include "MiddleEnd/ProgramParser.hh"

#include "BackEnd/Generator/Generator.hh"

#include "CommandLine.hh"
#include "Token.hh"

#include <iomanip>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <string>
#include <string>
#include <vector>
#include <map>
#include <sstream>

int main(int argc, char* argv[]) {
    Write("Vexar CLI", "Initialized Vexar CLI", 0, false, true);

    if (argc == 1) {
        Write("CLI", "No input file specified", 2, true, false);
    }

    auto Instructions = CommandLineHandler(argc, argv);

    if (Instructions->UsingMenu) {
        if (Instructions->HelpMenu) {
            std::cout << "Help Menu Placeholder!!" << std::endl;
        } else if (Instructions->VersionMenu) {
            std::cout << "Vexar (Built & Officially Owned by Vexar Project) 0.1.0 INDEV " << std::endl;
            std::cout << "Copyright (C) 2025 Free Software Foundation, Inc." << std::endl;
            std::cout << "This is free software; see the source for copying conditions.  There is NO" << std::endl;
            std::cout << "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << std::endl;
        }
        return 0;
    }

    Instructions->FileContents = SerializeFile(Instructions->InputFile);
    Instructions->ProgramTokens = Tokenize(Instructions->FileContents);
    if (Instructions->PrintTokens) {
        for (const auto& tok : Instructions->ProgramTokens) {
            std::string typeStr;
            switch (tok.type) {
                case TokenType::Identifier:  typeStr = "Identifier"; break;
                case TokenType::Number:      typeStr = "Number"; break;
                case TokenType::Float:       typeStr = "Float"; break;
                case TokenType::String:      typeStr = "String"; break;
                case TokenType::Character:   typeStr = "Character"; break;
                case TokenType::Keyword:     typeStr = "Keyword"; break;
                case TokenType::Operator:    typeStr = "Operator"; break;
                case TokenType::Delimiter:   typeStr = "Delimiter"; break;
                case TokenType::EndOfFile:   typeStr = "EndOfFile"; break;
                default:                     typeStr = "Unknown"; break;
            }

            std::ostringstream oss;
            oss << "Type: " << std::left << std::setw(12) << typeStr
                << " | Value: " << std::setw(20) << ("\"" + tok.value + "\"")
                << " | Line: " << std::setw(4) << tok.line
                << " | Column: " << std::setw(4) << tok.column;
            std::string FL = oss.str();
            Write("Tokenizer", FL, 3, true);
        }
    }

    Instructions->ProgramAST = ParseProgram(Instructions->ProgramTokens);

    if (Instructions->PrintAST) {
        Write("Parser", Instructions->ProgramAST->get(), 0, true);
    }

    GL_ASTPackage pkg;
    pkg.ASTRoot = std::move(Instructions->ProgramAST);
    pkg.InputFile = Instructions->InputFile;
    pkg.OutputFile = Instructions->OutputFile;
    pkg.Optimisation = Instructions->OptimizationLevel;
    pkg.Verbose = Instructions->Verbose;
    pkg.Debug = Instructions->Debug;

    Generator Gen(pkg);
    Gen.BuildLLVM();
    Gen.ValidateModule();
    Gen.OptimiseLLVM();
    Gen.PrintLLVM();
    //Gen.Compile();
    //Gen.BuildExecutable();

    return 0;
}