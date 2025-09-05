#include "Miscellaneous/conf/FileAssociations.hh"
#include "Miscellaneous/conf/TargetMap.hh"
#include "CommandLine.hh"

std::unique_ptr<CLIObject> CommandLineHandler(int argc, char* argv[]) {
    auto split = [](const std::string& s, char delim) -> std::vector<std::string> {
        std::vector<std::string> tokens;
        std::istringstream ss(s);
        for (std::string token; std::getline(ss, token, delim); tokens.push_back(token));
        return tokens;
    };

    auto In = std::make_unique<CLIObject>();
    fs::path cwd = fs::current_path();

    if (argc > 1) {
        if (std::string(argv[1]) == "--h" || std::string(argv[1]) == "--help") {
            In->UsingMenu = true;
            In->HelpMenu = true;
        } else if (std::string(argv[1]) == "--v" || std::string(argv[1]) == "--version") {
            In->UsingMenu = true;
            In->VersionMenu = true;
        } else if (std::string(argv[1]) == "--t" || std::string(argv[1]) == "--targets") {
            In->UsingMenu = true;
            In->TargetsMenu = true;
        }
    }

    if (In->UsingMenu) return In;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool recognized = false;

        if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) In->OutputFile = argv[++i];
            recognized = true;
        } 

        else if (arg == "-f" || arg == "--full-analysis")   { In->DumpBIN = true; In->DumpIR = true; In->DumpASM = true; In->DumpIR; In->DumpVec = true; In->DumpOp = true; In->DumpSym = true; In->DumpMem = true; In->DumpMod = true; In->DumpTokens = true; In->DumpAST = true; In->DumpBC = true; In->DumpVBC = true; In->DumpVIR; recognized = true;}
        else if (arg == "-m" || arg == "--module")          { In->DumpMod = true; recognized = true;}
        else if (arg == "-s" || arg == "--symbols")         { In->DumpSym = true; recognized = true;}
        else if (arg == "-opt" || arg == "--optimizations") { In->DumpOp = true; recognized = true;}
        else if (arg == "-vec" || arg == "--vectorization") { In->DumpVec = true; recognized = true;}
        else if (arg == "-e" || arg == "--memory")          { In->DumpMem = true; recognized = true;}
        else if (arg == "-b" || arg == "--bin")             { In->DumpBIN = true; recognized = true;}
        else if (arg == "-d" || arg == "--dump")            { In->DumpASM = true; recognized = true;}
        else if (arg == "-bc" || arg == "--dump-bc")        { In->DumpBC = true; recognized = true;}
        else if (arg == "-vbc" || arg == "--dump-verbose-bc"){ In->DumpVBC = true; recognized = true;}
        else if (arg == "-ir" || arg == "--dump-ir")        { In->DumpIR = true; recognized = true;}
        else if (arg == "-vir" || arg == "--dump-verbose-ir"){ In->DumpVIR = true; recognized = true;}
        else if (arg == "-c" || arg == "--check")           { In->Check = true; recognized = true; }
        else if (arg == "-t" || arg == "--print_tokens")    { In->DumpTokens = true; recognized = true; }
        else if (arg == "-a" || arg == "--print_ast")       { In->DumpAST = true; recognized = true; }
        else if (arg == "-g" || arg == "--debug")           { In->Debug = true; recognized = true; }
        else if (arg == "-v" || arg == "--verbose")         { In->Verbose = true; recognized = true; }

        else if (arg == "-r" || arg == "--run")             { In->RunAfterCompile = true; recognized = true; }
        else if (arg == "-w" || arg == "--no-warnings")     { In->EmitWarnings = true; recognized = true; }
        
        else if (arg == "-I" || arg == "--include") {
            recognized = true;
        }
        
        else if ((arg.find("--target=") != std::string::npos) || (arg.find("-t=") != std::string::npos)) {
            recognized = true;
            auto target = split(arg, '=').back();

            if (target_map.find(target) != target_map.end()) {
                In->CompilerTarget = target_map[target];
            } else {
                Write("CLI", "Unrecognized target '" + target + "'", 2, true);
            }
        } 
        else if (arg.rfind("-O", 0) == 0) {
            std::string level = arg.substr(2);
            In->OptimizationLevel = (!level.empty() && isdigit(level[0])) ? std::clamp(level[0] - '0', 0, 5) : 0;
            recognized = true;
        } else {
            fs::path possibleFile = cwd / arg;
            if (fs::exists(possibleFile)) { In->InputFile = possibleFile; recognized = true; }
            else {
                for (auto& Tag : VexarAssociations) {
                    fs::path p = cwd / (arg + "." + Tag);
                    if (fs::exists(p)) { In->InputFile = p; recognized = true; break; }
                }
            }
        }

        if (!recognized) Write("CLI", "Unrecognized command-line option '" + arg + "'", 2, true);
    }

    if (In->InputFile.empty()) Write("CLI", "No input file specified", 2, true);
    if (!fs::exists(In->InputFile)) Write("CLI", "Input file does not exist: " + In->InputFile.string(), 2, true);

    if (In->OutputFile.empty()) In->OutputFile = In->InputFile.parent_path() / (In->InputFile.stem().string() + ".exe");
    else { fs::path p(In->OutputFile); In->OutputFile = p.is_absolute() ? p : cwd / p; }

    return In;
}
