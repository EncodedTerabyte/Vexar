#include "PlatformBinary.hh"
#include <cstdlib>

void CreatePlatformBinary(std::unique_ptr<llvm::Module> Module, std::string Triple, fs::path Output) {
    int clangCheck = std::system("clang --version > nul 2>&1");
    if (clangCheck != 0) {
        Write("CodeGen", "Vexar needs clang installed for creating binaries; Please install clang.", 2, true, true, "");
        return;
    }

    fs::path OutputDir = Output.parent_path();
    std::string OutputName = Output.stem().string();
    
    if (!Module || Module->empty()) {
        Write("CodeGen", "Module is empty or null", 2, true, true, "");
        return;
    }
    
    if (Triple == "llvm") {
        fs::path LLFile = OutputDir / (OutputName + ".ll");
        std::error_code EC;
        llvm::raw_fd_ostream LLStream(LLFile.string(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            Write("CodeGen", "Error opening .ll file: " + EC.message(), 2, true, true, "");
            return;
        }
        Module->print(LLStream, nullptr);
        LLStream.flush();
        Write("CodeGen", "Generated LLVM IR", 3, true, true, "");
        return;
    }
    
    if (Triple == "bitcode") {
        fs::path BCFile = OutputDir / (OutputName + ".bc");
        std::error_code EC;
        llvm::raw_fd_ostream BCStream(BCFile.string(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            Write("CodeGen", "Error opening .bc file: " + EC.message(), 2, true, true, "");
            return;
        }
        llvm::WriteBitcodeToFile(*Module, BCStream);
        BCStream.flush();
        Write("CodeGen", "Generated bitcode", 3, true, true, "");
        return;
    }
    
    fs::path TempLLFile = OutputDir / (OutputName + "_temp.ll");
    std::error_code EC;
    llvm::raw_fd_ostream LLStream(TempLLFile.string(), EC, llvm::sys::fs::OF_None);
    if (EC) {
        Write("CodeGen", "Error creating temporary .ll file: " + EC.message(), 2, true, true, "");
        return;
    }
    
    Module->print(LLStream, nullptr);
    LLStream.flush();
    
    std::string ClangCommand;
    
    if (Triple == "asm") {
        fs::path AsmFile = OutputDir / (OutputName + ".s");
        ClangCommand = "clang -S -w -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("CodeGen", "Assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "obj") {
        fs::path ObjFile = OutputDir / (OutputName + ".o");
        ClangCommand = "clang -c -w -o \"" + ObjFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("CodeGen", "Object file generation failed", 2, true, true, "");
        }
        return;
    }
    
    std::vector<std::string> TargetFlags;
    if (Triple == "x86_64-pc-windows-msvc" || Triple == "win") {
        TargetFlags = {"--target=x86_64-pc-windows-msvc"};
    } else if (Triple == "x86_64-pc-windows-gnu" || Triple == "mingw") {
        TargetFlags = {"--target=x86_64-pc-windows-gnu"};
    } else if (Triple == "x86_64-pc-linux-gnu" || Triple == "linux") {
        TargetFlags = {"--target=x86_64-pc-linux-gnu"};
    } else if (Triple == "x86_64-apple-darwin" || Triple == "mac") {
        TargetFlags = {"--target=x86_64-apple-darwin"};
    } else if (Triple == "arm64-apple-darwin" || Triple == "mac-arm64") {
        TargetFlags = {"--target=arm64-apple-darwin"};
    } else if (Triple == "wasm32-unknown-unknown" || Triple == "wasm") {
        TargetFlags = {"--target=wasm32-unknown-unknown", "-nostdlib"};
    }
    
    std::string TargetFlag = "";
    if (!TargetFlags.empty()) {
        for (const auto& flag : TargetFlags) {
            TargetFlag += flag + " ";
        }
    }
    
    ClangCommand = "clang -w " + TargetFlag + "-o \"" + Output.string() + "\" \"" + TempLLFile.string() + "\"";
    
    if (Triple == "x86_64-pc-windows-msvc" || Triple == "win") {
        ClangCommand += " -Xlinker /NODEFAULTLIB:libcmt -Xlinker /DEFAULTLIB:msvcrt.lib -Xlinker /DEFAULTLIB:legacy_stdio_definitions.lib -Xlinker /DEFAULTLIB:ucrt.lib -Xlinker /DEFAULTLIB:kernel32.lib";
    }
    
    int Result = std::system(ClangCommand.c_str());
    std::filesystem::remove(TempLLFile);
    
    if (Result != 0) {
        Write("CodeGen", "Compilation failed for target: " + Triple, 2, true, true, "");
    }
}