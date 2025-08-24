#include "CommandLine.hh"

std::vector<std::string> VexarAssociations = {"vx", "vxr", "vexa", "vexar", "vxx", "va"};

std::unique_ptr<CLIObject> CommandLineHandler(int argc, char* argv[]) {
    auto In = std::make_unique<CLIObject>();
    fs::path cwd = fs::current_path();

    if (argc > 1) {
        if (std::string(argv[1]) == "--h" || std::string(argv[1]) == "--help") {
            In->UsingMenu = true;
            In->HelpMenu = true;
        } else if (std::string(argv[1]) == "--v" || std::string(argv[1]) == "--version") {
            In->UsingMenu = true;
            In->VersionMenu = true;
        }
    }

    if (In->UsingMenu) return In;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool recognized = false;

        if (arg == "-o") {
            if (i + 1 < argc) In->OutputFile = argv[++i];
            recognized = true;
        } else if (arg == "-g") { In->Debug = true; recognized = true; }
        else if (arg == "-v") { In->Verbose = true; recognized = true; }
        else if (arg == "--run") { In->RunAfterCompile = true; recognized = true; }
        else if (arg == "--print_tokens") { In->PrintTokens = true; recognized = true; }
        else if (arg == "--print_ast") { In->PrintAST = true; recognized = true; }
        else if (arg.rfind("-O", 0) == 0) {
            std::string level = arg.substr(2);
            In->OptimizationLevel = (!level.empty() && isdigit(level[0])) ? std::clamp(level[0] - '0', 0, 3) : 0;
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
