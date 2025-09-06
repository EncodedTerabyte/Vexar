#include "Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "Miscellaneous/LoggerHandler/ColorPrint.hh"

#include "FrontEnd/FileSerializer.hh"
#include "FrontEnd/Tokenizer.hh"

#include "MiddleEnd/ProgramParser.hh"

#include "BackEnd/Generator/ModuleAnalyser.hh"
#include "BackEnd/Generator/Generator.hh"

#include "CommandLine.hh"
#include "Token.hh"

#include <iomanip>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <ctime>
#include <chrono>

#include "Menu.hh"


int main(int argc, char* argv[]) {
    auto V_C_START = std::chrono::high_resolution_clock::now();
    Write("Vexar CLI", "Initialized Vexar CLI", 0, false, true);

    if (argc == 1) {
        Write("CLI", "No input file specified", 2, true, true);
    }

    auto Instructions = CommandLineHandler(argc, argv);

    if (Instructions->UsingMenu) {
        if (Instructions->HelpMenu) {
            PrintVesion();
            PrintHelpMenu();
        } else if (Instructions->TargetsMenu) {
            PrintVesion();
            PrintTargets();
        }else if (Instructions->VersionMenu) {
            PrintVesion();
        }
        return 0;
    }

    Instructions->FileContents = SerializeFile(Instructions->InputFile);
    Instructions->ProgramTokens = Tokenize(Instructions->FileContents);

    if (Instructions->Verbose) {   
        Write("CLI", "Serialization Complete", 3, true, true);
        Write("CLI", "Tokenization Complete", 3, true, true);
    }

    if (Instructions->DumpTokens) {
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

    if (Instructions->Verbose) {   
        Write("CLI", "Generating Program", 3, true, true);
    }

    Instructions->ProgramAST = ParseProgram(Instructions->ProgramTokens);

    if (Instructions->DumpAST) {
        Write("Parser", Instructions->ProgramAST->get(), 0, true);
    }

    GL_ASTPackage pkg;
    pkg.ASTRoot = std::move(Instructions->ProgramAST);
    pkg.InputFile = Instructions->InputFile;
    pkg.OutputFile = Instructions->OutputFile;
    pkg.Optimisation = Instructions->OptimizationLevel;
    pkg.Verbose = Instructions->Verbose;
    pkg.Debug = Instructions->Debug;
    pkg.RunAfterCompile = Instructions->RunAfterCompile;
    pkg.CompilerTarget = Instructions->CompilerTarget;

    if (Instructions->Verbose) {   
        std::ostringstream ss;
        ss << "Compiling with flags:\n"
        << "  Input File: "    << Instructions->InputFile << "\n"
        << "  Output File: "   << Instructions->OutputFile << "\n"
        << "  Optimization: O" << Instructions->OptimizationLevel << "\n"
        << "  Verbose: "       << (Instructions->Verbose ? "true" : "false") << "\n"
        << "  Debug: "         << (Instructions->Debug ? "true" : "false") << "\n";
        
        Write("CLI", ss.str(), 1, true, true);
    }

    Generator Gen(pkg);
    llvm::Module* Mode = Gen.GetModulePtr();
    Gen.BuildModule();

    auto V_C_END = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = V_C_END - V_C_START;

    ModuleAnalyzer Analyzer(Mode);

    if (Instructions->DumpBIN) {   
        Analyzer.RunFullAnalysis();
    } else {
        if (Instructions->DumpVec ) {
            Analyzer.AnalyzeVectorization();
        } 

        if (Instructions->DumpOp) {
            Analyzer.AnalyzeOptimizations();
        }

        if (Instructions->DumpSym) {
            Analyzer.AnalyzeSymbols();
        }

        if (Instructions->DumpMem) {
            Analyzer.AnalyzeMemoryUsage();
        }

        if (Instructions->DumpMod) {
            Analyzer.AnalyzeModuleStructure();
        }

        if (Instructions->DumpBC) {
            Analyzer.DumpBitcode();
        }

        if (Instructions->DumpVBC) {
            Analyzer.DumpVerboseBitcode();
        }

        if (Instructions->DumpVIR) {
            Analyzer.DisassembleIR();
        }
    }

    if (Instructions->DumpASM) {
        NativeAssemblyAnalyzer AsmAnalyzer(Mode);
        AsmAnalyzer.PrintNativeAssembly();
    }

    if (Instructions->DumpIR) {
        Gen.PrintModule();
    }

    if (Instructions->Check) {
        bool isValid = Gen.ValidateModule();
        if (isValid) {
            Write("CLI", "Module validation successful. No errors found.", 3, true, true);
        } else {
            Write("CLI", "Module validation failed. Errors were found.", 2, true, true);
        }
    } else {
        Gen.OptimiseModule();
        Gen.CompileTriple();
    }

    if (Instructions->Verbose) {
        Write("CLI", "Code Generation Complete In " + std::to_string(elapsed.count()) + "s", 3, true, true);
    }
    
    return 0;
}