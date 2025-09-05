#include "PlatformBinary.hh"
#include <cstdlib>

#include "../../Miscellaneous/conf/TargetMap.hh"

void CreatePlatformBinary(std::unique_ptr<llvm::Module> Module, std::string Triple, bool RunAfterCompile, fs::path Output) {
    if (target_map.find(Triple) != target_map.end()) {
        Triple = target_map[Triple];
    }
    
    int clangCheck = std::system("clang --version > nul 2>&1");
    if (clangCheck != 0) {
        Write("Code Generation", "Vexar needs clang installed for creating binaries; Please install clang.", 2, true, true, "");
        return;
    }

    fs::path OutputDir = Output.parent_path();
    std::string OutputName = Output.stem().string();
    
    if (!Module || Module->empty()) {
        Write("Code Generation", "Module is empty or null", 2, true, true, "");
        return;
    }

#ifdef _WIN32
    std::vector<std::string> incompatibleTargets = {
        "x86_64-unknown-linux-gnu", "i686-unknown-linux-gnu", "aarch64-unknown-linux-gnu", 
        "arm-unknown-linux-gnueabihf", "riscv64-unknown-linux-gnu", "x86_64-unknown-linux-musl", 
        "aarch64-unknown-linux-musl", "x86_64-apple-darwin", "arm64-apple-darwin", 
        "x86_64-apple-ios", "arm64-apple-ios", "x86_64-unknown-freebsd", "aarch64-unknown-freebsd", 
        "x86_64-unknown-openbsd", "x86_64-unknown-netbsd", "x86_64-sun-solaris", 
        "x86_64-pc-linux-gnu", "i686-pc-linux-gnu", "aarch64-linux-gnu", "arm-linux-gnueabihf", 
        "mips64-unknown-linux-gnu", "mips-unknown-linux-gnu", "powerpc64-unknown-linux-gnu", 
        "powerpc64le-unknown-linux-gnu", "s390x-unknown-linux-gnu", "sparc64-unknown-linux-gnu", 
        "x86_64-unknown-haiku", "x86_64-fuchsia", "aarch64-fuchsia"
    };
    for (const auto& target : incompatibleTargets) {
        if (Triple == target) {
            Write("Code Generation", "Target " + Triple + " requires Unix/Linux toolchain not available on Windows", 2, true, true, "");
            return;
        }
    }
#endif
    
    if (Triple == "llvm") {
        fs::path LLFile = OutputDir / (OutputName + ".ll");
        std::error_code EC;
        llvm::raw_fd_ostream LLStream(LLFile.string(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            Write("Code Generation", "Error opening .ll file: " + EC.message(), 2, true, true, "");
            return;
        }
        Module->print(LLStream, nullptr);
        LLStream.flush();
        Write("Code Generation", "Generated LLVM IR", 3, true, true, "");
        return;
    }
    
    if (Triple == "bitcode") {
        fs::path BCFile = OutputDir / (OutputName + ".bc");
        std::error_code EC;
        llvm::raw_fd_ostream BCStream(BCFile.string(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            Write("Code Generation", "Error opening .bc file: " + EC.message(), 2, true, true, "");
            return;
        }
        llvm::WriteBitcodeToFile(*Module, BCStream);
        BCStream.flush();
        Write("Code Generation", "Generated bitcode", 3, true, true, "");
        return;
    }
    
    fs::path TempLLFile = OutputDir / (OutputName + ".ll");
    std::error_code EC;
    llvm::raw_fd_ostream LLStream(TempLLFile.string(), EC, llvm::sys::fs::OF_None);
    if (EC) {
        Write("Code Generation", "Error creating temporary .ll file: " + EC.message(), 2, true, true, "");
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
            Write("Code Generation", "Assembly generation failed", 2, true, true, "");
        }
        return;
    }

    if (Triple == "interpret" || RunAfterCompile) {
        fs::path LLFile = OutputDir / (OutputName + ".ll");
        std::error_code EC;
        llvm::raw_fd_ostream LLStream(LLFile.string(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            Write("Code Generation", "Error opening .ll file: " + EC.message(), 2, true, true, "");
            return;
        }
        Module->print(LLStream, nullptr);
        LLStream.flush();

        std::string LliCommand = "lli \"" + LLFile.string() + "\"";
        int Result = std::system(LliCommand.c_str());
        
        if (Result != 0) {
            Write("Code Generation", "Interpretation failed - lli not found or IR invalid", 2, true, true, "");
        } else {
            Write("Code Generation", "Program interpreted successfully", 3, true, true, "");
        }
        std::filesystem::remove(LLFile);
        
        return;
    }
    
    if (Triple == "asm-intel") {
        fs::path AsmFile = OutputDir / (OutputName + "_intel.s");
        ClangCommand = "clang -S -w -mllvm --x86-asm-syntax=intel -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "Intel assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "asm-att") {
        fs::path AsmFile = OutputDir / (OutputName + "_att.s");
        ClangCommand = "clang -S -w -mllvm --x86-asm-syntax=att -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "AT&T assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "asm-arm") {
        fs::path AsmFile = OutputDir / (OutputName + "_arm.s");
        ClangCommand = "clang -S -w -target arm -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "ARM assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "asm-arm64") {
        fs::path AsmFile = OutputDir / (OutputName + "_arm64.s");
        ClangCommand = "clang -S -w -target aarch64 -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "ARM64 assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "asm-riscv64") {
        fs::path AsmFile = OutputDir / (OutputName + "_riscv64.s");
        ClangCommand = "clang -S -w -target riscv64 -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "RISC-V 64 assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "asm-wasm") {
        fs::path AsmFile = OutputDir / (OutputName + "_wasm.wat");
        ClangCommand = "clang -S -w -target wasm32 -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "WebAssembly text generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "asm-ptx") {
        fs::path AsmFile = OutputDir / (OutputName + ".ptx");
        ClangCommand = "clang -S -w -target nvptx64 -nocudainc -nocudalib -o \"" + AsmFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "PTX assembly generation failed", 2, true, true, "");
        }
        return;
    }
    
    if (Triple == "obj") {
        fs::path ObjFile = OutputDir / (OutputName + ".o");
        ClangCommand = "clang -c -w -o \"" + ObjFile.string() + "\" \"" + TempLLFile.string() + "\"";
        
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        
        if (Result != 0) {
            Write("Code Generation", "Object file generation failed", 2, true, true, "");
        }
        return;
    }
    
    std::vector<std::string> TargetFlags;
    std::string ExtraFlags = "";
    fs::path FinalOutput = Output;
    
    if (Triple == "x86_64-pc-windows-msvc") {
        TargetFlags = {"--target=x86_64-pc-windows-msvc"};
        ExtraFlags = " -Xlinker /NODEFAULTLIB:libcmt -Xlinker /DEFAULTLIB:msvcrt.lib -Xlinker /DEFAULTLIB:legacy_stdio_definitions.lib -Xlinker /DEFAULTLIB:ucrt.lib -Xlinker /DEFAULTLIB:kernel32.lib";
    } else if (Triple == "x86_64-pc-windows-gnu") {
        TargetFlags = {"--target=x86_64-pc-windows-gnu"};
    } else if (Triple == "i686-pc-windows-msvc") {
        TargetFlags = {"--target=i686-pc-windows-msvc"};
        ExtraFlags = " -Xlinker /NODEFAULTLIB:libcmt -Xlinker /DEFAULTLIB:msvcrt.lib -Xlinker /DEFAULTLIB:legacy_stdio_definitions.lib -Xlinker /DEFAULTLIB:ucrt.lib -Xlinker /DEFAULTLIB:kernel32.lib";
    } else if (Triple == "aarch64-pc-windows-msvc") {
        TargetFlags = {"--target=aarch64-pc-windows-msvc"};
    } else if (Triple == "wasm32-unknown-unknown") {
        FinalOutput = OutputDir / (OutputName + ".wasm");
        TargetFlags = {"--target=wasm32-unknown-unknown", "-nostdlib", "-Wl,--no-entry"};
    } else if (Triple == "wasm64-unknown-unknown") {
        FinalOutput = OutputDir / (OutputName + ".wasm");
        TargetFlags = {"--target=wasm64-unknown-unknown", "-nostdlib", "-Wl,--no-entry"};
    } else if (Triple == "nvptx64-nvidia-cuda") {
        fs::path PtxFile = OutputDir / (OutputName + ".ptx");
        ClangCommand = "clang --cuda-device-only -S -w --target=nvptx64-nvidia-cuda -nocudainc -nocudalib -o \"" + PtxFile.string() + "\" \"" + TempLLFile.string() + "\"";
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        if (Result != 0) {
            Write("Code Generation", "CUDA PTX generation failed - CUDA toolkit may not be installed", 2, true, true, "");
        } else {
            Write("Code Generation", "Generated CUDA PTX", 3, true, true, "");
        }
        return;
    } else if (Triple == "nvptx-nvidia-cuda") {
        fs::path PtxFile = OutputDir / (OutputName + "_32.ptx");
        ClangCommand = "clang --cuda-device-only -S -w --target=nvptx-nvidia-cuda -nocudainc -nocudalib -o \"" + PtxFile.string() + "\" \"" + TempLLFile.string() + "\"";
        int Result = std::system(ClangCommand.c_str());
        std::filesystem::remove(TempLLFile);
        if (Result != 0) {
            Write("Code Generation", "CUDA PTX 32 generation failed - CUDA toolkit may not be installed", 2, true, true, "");
        } else {
            Write("Code Generation", "Generated CUDA PTX 32", 3, true, true, "");
        }
        return;
    } else if (Triple == "aarch64") {
        TargetFlags = {"-target", "aarch64", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "arm") {
        TargetFlags = {"-target", "arm", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "riscv64") {
        TargetFlags = {"-target", "riscv64", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "riscv32") {
        TargetFlags = {"-target", "riscv32", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "thumb") {
        TargetFlags = {"-target", "thumb", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "bpf") {
        TargetFlags = {"-target", "bpf", "-nostdlib", "-O2"};
    } else if (Triple == "bpfel") {
        TargetFlags = {"-target", "bpfel", "-nostdlib", "-O2"};
    } else if (Triple == "bpfeb") {
        TargetFlags = {"-target", "bpfeb", "-nostdlib", "-O2"};
    } else if (Triple == "aarch64_32") {
        TargetFlags = {"-target", "aarch64_32", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "arm64_32") {
        TargetFlags = {"-target", "arm64_32", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "aarch64_be") {
        TargetFlags = {"-target", "aarch64_be", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "armeb") {
        TargetFlags = {"-target", "armeb", "-nostdlib", "-ffreestanding"};
    } else if (Triple == "thumbeb") {
        TargetFlags = {"-target", "thumbeb", "-nostdlib", "-ffreestanding"};
    } else {
        Write("Code Generation", "Target " + Triple + " is not supported or requires missing toolchain", 2, true, true, "");
        std::filesystem::remove(TempLLFile);
        return;
    }
    
    std::string TargetFlag = "";
    if (!TargetFlags.empty()) {
        for (const auto& flag : TargetFlags) {
            TargetFlag += flag + " ";
        }
    }
    
    ClangCommand = "clang -w " + TargetFlag + "-o \"" + FinalOutput.string() + "\" \"" + TempLLFile.string() + "\"" + ExtraFlags;
    
    int Result = std::system(ClangCommand.c_str());
    
    if (Result != 0) {
        Write("Code Generation", "Compilation failed for target: " + Triple, 2, true, true, "");
        Write("Code Generation", "Target toolchain not installed or not available on this platform", 2, true, true, "");
        
        fs::path FallbackObj = OutputDir / (OutputName + ".o");
        std::string FallbackCommand = "clang -c -w " + TargetFlag + "-o \"" + FallbackObj.string() + "\" \"" + TempLLFile.string() + "\"";
        int FallbackResult = std::system(FallbackCommand.c_str());
        
        if (FallbackResult == 0) {
            Write("Code Generation", "Generated object file instead: " + FallbackObj.string(), 1, true, true, "");
        } else {
            Write("Code Generation", "Target not supported on this system", 2, true, true, "");
        }
        std::filesystem::remove(TempLLFile);
        return;
    }
    
    std::filesystem::remove(TempLLFile);
}