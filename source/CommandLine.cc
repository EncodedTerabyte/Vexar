#include "CommandLine.hh"

std::vector<std::string> VexarAssociations = {"vx", "vxr", "vexa", "vexar", "vxx", "va"};
std::unordered_map<std::string, std::string> target_map = {
    {"asm", "asm"}, {"assembly", "asm"}, {"s", "asm"},
    {"llvm", "llvm"}, {"ir", "llvm"},
    {"bitcode", "bitcode"}, {"bc", "bitcode"},
    {"obj", "obj"}, {"object", "obj"}, {"o", "obj"},

    {"win", "x86_64-pc-windows-msvc"}, {"exe", "x86_64-pc-windows-msvc"}, {"windows", "x86_64-pc-windows-msvc"}, {"win64", "x86_64-pc-windows-msvc"}, {"win-msvc", "x86_64-pc-windows-msvc"},

    {"win-gnu", "x86_64-pc-windows-gnu"}, {"mingw", "x86_64-pc-windows-gnu"},
    
    //{"linux", "x86_64-pc-linux-gnu"}, {"gnu", "x86_64-pc-linux-gnu"},

    //{"mac", "x86_64-apple-darwin"}, {"macos", "x86_64-apple-darwin"}, {"darwin", "x86_64-apple-darwin"},
    //{"mac-arm64", "arm64-apple-darwin"}, {"macos-arm64", "arm64-apple-darwin"},

    //{"wasm", "wasm32-unknown-unknown"}, {"webassembly", "wasm32-unknown-unknown"}, {"web", "wasm32-unknown-unknown"},
    //{"wasm-wasi", "wasm32-wasi"}, {"wasi", "wasm32-wasi"},
};

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
        else if (arg.find("--target=") != std::string::npos) {
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
