#include "ModuleAnalyser.hh"
#include "../../Miscellaneous/LoggerHandler/ColorPrint.hh"

#include <fstream>
#include <cstdlib>

void PrintSectionHeader(const std::string& title) {
    PrintRGB("\n", 0, 0, 0);
    PrintRGB(title, 0, 255, 255);
    PrintRGB("\n", 0, 0, 0);
    for (size_t i = 0; i < title.length(); i++) {
        PrintRGB("=", 100, 100, 100);
    }
    std::cout << "\n";
}

void PrintTreeItem(const std::string& prefix, const std::string& item, const std::string& details = "") {
    PrintRGB(prefix, 150, 150, 150);
    PrintRGB(item, 255, 255, 255);
    if (!details.empty()) {
        PrintRGB(" ", 0, 0, 0);
        PrintRGB(details, 100, 200, 100);
    }
    std::cout << "\n";
}

void PrintAttribute(const std::string& name, const std::string& value, bool highlight = false) {
    PrintRGB("  ", 0, 0, 0);
    PrintRGB(name, 200, 200, 255);
    PrintRGB(": ", 150, 150, 150);
    if (highlight) {
        PrintRGB(value, 255, 255, 0);
    } else {
        PrintRGB(value, 100, 255, 100);
    }
    std::cout << "\n";
}

ModuleAnalyzer::ModuleAnalyzer(llvm::Module* M) : Module(M) {}

void ModuleAnalyzer::AnalyzeSymbols() {
    PrintSectionHeader("Module Symbols");
    
    PrintRGB("|-", 150, 150, 150);
    PrintRGB("GlobalVariables\n", 255, 200, 100);
    
    for (auto& GV : Module->globals()) {
        std::string prefix = "  |-";
        std::string name = GV.getName().str();
        std::string details = "";
        
        if (GV.hasInitializer()) {
            details += "[init] ";
        }
        
        details += "linkage=";
        switch (GV.getLinkage()) {
            case llvm::GlobalValue::ExternalLinkage: details += "external"; break;
            case llvm::GlobalValue::InternalLinkage: details += "internal"; break;
            case llvm::GlobalValue::PrivateLinkage: details += "private"; break;
            default: details += "other"; break;
        }
        
        PrintTreeItem(prefix, name, details);
    }
    
    PrintRGB("|-", 150, 150, 150);
    PrintRGB("Functions\n", 255, 200, 100);
    
    for (auto& F : *Module) {
        std::string prefix = "  |-";
        std::string name = F.getName().str();
        std::string details = "";
        
        if (F.isDeclaration()) {
            details += "[decl] ";
        } else {
            details += "[def:" + std::to_string(F.size()) + "bb] ";
        }
        
        details += "linkage=";
        switch (F.getLinkage()) {
            case llvm::GlobalValue::ExternalLinkage: details += "external"; break;
            case llvm::GlobalValue::InternalLinkage: details += "internal"; break;
            case llvm::GlobalValue::PrivateLinkage: details += "private"; break;
            default: details += "other"; break;
        }
        
        PrintTreeItem(prefix, name, details);
    }
    
    if (Module->alias_begin() != Module->alias_end()) {
        PrintRGB("|-", 150, 150, 150);
        PrintRGB("Aliases\n", 255, 200, 100);
        
        for (auto& A : Module->aliases()) {
            PrintTreeItem("  |-", A.getName().str());
        }
    }
}

void ModuleAnalyzer::AnalyzeModuleStructure() {
    PrintSectionHeader("Module Structure");
    
    PrintAttribute("ModuleID", Module->getModuleIdentifier());
    PrintAttribute("TargetTriple", Module->getTargetTriple().str());
    PrintAttribute("DataLayout", Module->getDataLayoutStr());
    
    size_t GlobalVars = 0, Functions = 0, Declarations = 0, Aliases = 0;
    
    for (auto& GV : Module->globals()) GlobalVars++;
    for (auto& F : *Module) {
        Functions++;
        if (F.isDeclaration()) Declarations++;
    }
    for (auto& A : Module->aliases()) Aliases++;
    
    PrintRGB("\n|-", 150, 150, 150);
    PrintRGB("Statistics\n", 255, 200, 100);
    
    PrintAttribute("  GlobalVariables", std::to_string(GlobalVars));
    PrintAttribute("  Functions", std::to_string(Functions));
    PrintAttribute("  Declarations", std::to_string(Declarations));
    PrintAttribute("  Definitions", std::to_string(Functions - Declarations), true);
    PrintAttribute("  Aliases", std::to_string(Aliases));
}

void ModuleAnalyzer::DisassembleIR() {
    PrintSectionHeader("LLVM IR Disassembly");
    
    for (auto& F : *Module) {
        if (F.isDeclaration()) continue;
        
        PrintRGB("|-", 150, 150, 150);
        PrintRGB("Function: ", 255, 200, 100);
        PrintRGB(F.getName().str(), 255, 255, 255);
        std::cout << "\n";
        
        PrintAttribute("  ReturnType", "");
        std::cout << "    ";
        F.getReturnType()->print(llvm::outs());
        std::cout << "\n";
        
        PrintAttribute("  Arguments", std::to_string(F.arg_size()));
        
        for (auto& BB : F) {
            PrintRGB("  |-", 150, 150, 150);
            PrintRGB("BasicBlock: ", 200, 200, 255);
            
            if (BB.hasName()) {
                PrintRGB(BB.getName().str(), 255, 255, 100);
            } else {
                PrintRGB("(unnamed)", 150, 150, 150);
            }
            
            PrintRGB(" [" + std::to_string(BB.size()) + " instructions]", 100, 200, 100);
            std::cout << "\n";
            
            for (auto& I : BB) {
                PrintRGB("    ", 0, 0, 0);
                I.print(llvm::outs());
                std::cout << "\n";
            }
        }
    }
}

void ModuleAnalyzer::AnalyzeOptimizations() {
    PrintSectionHeader("Optimization Analysis");
    
    for (auto& F : *Module) {
        if (F.isDeclaration()) continue;
        
        size_t TotalInstructions = 0;
        size_t MemoryOps = 0;
        size_t Branches = 0;
        size_t Calls = 0;
        std::map<std::string, size_t> InstructionCounts;
        
        for (auto& BB : F) {
            for (auto& I : BB) {
                TotalInstructions++;
                std::string OpName = I.getOpcodeName();
                InstructionCounts[OpName]++;
                
                if (I.mayReadOrWriteMemory()) MemoryOps++;
                if (llvm::isa<llvm::BranchInst>(I) || llvm::isa<llvm::SwitchInst>(I)) Branches++;
                if (llvm::isa<llvm::CallInst>(I)) Calls++;
            }
        }
        
        PrintRGB("|-", 150, 150, 150);
        PrintRGB("Function: ", 255, 200, 100);
        PrintRGB(F.getName().str(), 255, 255, 255);
        std::cout << "\n";
        
        PrintAttribute("  Instructions", std::to_string(TotalInstructions), true);
        
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%zu (%.1f%%)", MemoryOps, 100.0 * MemoryOps / TotalInstructions);
        PrintAttribute("  MemoryOps", buffer);
        
        snprintf(buffer, sizeof(buffer), "%zu (%.1f%%)", Branches, 100.0 * Branches / TotalInstructions);
        PrintAttribute("  Branches", buffer);
        
        PrintAttribute("  FunctionCalls", std::to_string(Calls));
        
        PrintRGB("  |-", 150, 150, 150);
        PrintRGB("TopInstructions\n", 200, 200, 255);
        
        std::vector<std::pair<size_t, std::string>> SortedInsts;
        for (auto& P : InstructionCounts) {
            SortedInsts.push_back({P.second, P.first});
        }
        std::sort(SortedInsts.rbegin(), SortedInsts.rend());
        
        for (size_t I = 0; I < std::min(size_t(5), SortedInsts.size()); I++) {
            PrintAttribute("    " + SortedInsts[I].second, std::to_string(SortedInsts[I].first));
        }
    }
}

void ModuleAnalyzer::AnalyzeVectorization() {
    PrintSectionHeader("Vectorization Analysis");
    
    for (auto& F : *Module) {
        if (F.isDeclaration()) continue;
        
        bool HasVectorOps = false;
        bool HasLoops = false;
        size_t ArrayAccesses = 0;
        
        for (auto& BB : F) {
            for (auto& I : BB) {
                if (auto* BI = llvm::dyn_cast<llvm::BranchInst>(&I)) {
                    if (BI->isConditional()) {
                        HasLoops = true;
                    }
                }
                
                if (I.getType()->isVectorTy()) {
                    HasVectorOps = true;
                }
                
                if (llvm::isa<llvm::GetElementPtrInst>(I) || 
                    llvm::isa<llvm::LoadInst>(I) || 
                    llvm::isa<llvm::StoreInst>(I)) {
                    ArrayAccesses++;
                }
            }
        }
        
        PrintRGB("|-", 150, 150, 150);
        PrintRGB("Function: ", 255, 200, 100);
        PrintRGB(F.getName().str(), 255, 255, 255);
        std::cout << "\n";
        
        PrintAttribute("  VectorOps", HasVectorOps ? "yes" : "no");
        PrintAttribute("  PotentialLoops", HasLoops ? "yes" : "no");
        PrintAttribute("  MemoryAccesses", std::to_string(ArrayAccesses));
        
        if (HasLoops && ArrayAccesses > 0 && !HasVectorOps) {
            PrintRGB("  ", 0, 0, 0);
            PrintRGB("note: ", 255, 255, 0);
            PrintRGB("vectorization opportunity detected\n", 255, 200, 0);
        }
    }
}

void ModuleAnalyzer::AnalyzeMemoryUsage() {
    PrintSectionHeader("Memory Usage Analysis");
    
    size_t TotalGlobalSize = 0;
    llvm::DataLayout DL(Module->getDataLayoutStr());
    
    PrintRGB("|-", 150, 150, 150);
    PrintRGB("GlobalMemory\n", 255, 200, 100);
    
    for (auto& GV : Module->globals()) {
        if (GV.hasInitializer()) {
            auto Size = DL.getTypeAllocSize(GV.getValueType());
            TotalGlobalSize += Size;
            
            PrintAttribute("  " + GV.getName().str(), std::to_string(Size) + " bytes");
        }
    }
    
    PrintAttribute("TotalGlobal", std::to_string(TotalGlobalSize) + " bytes", true);
    
    PrintRGB("|-", 150, 150, 150);
    PrintRGB("StackMemory\n", 255, 200, 100);
    
    for (auto& F : *Module) {
        if (F.isDeclaration()) continue;
        
        size_t AllocaCount = 0;
        size_t EstimatedStackSize = 0;
        
        for (auto& BB : F) {
            for (auto& I : BB) {
                if (auto* AI = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
                    AllocaCount++;
                    auto Size = DL.getTypeAllocSize(AI->getAllocatedType());
                    EstimatedStackSize += Size;
                }
            }
        }
        
        if (AllocaCount > 0) {
            PrintAttribute("  " + F.getName().str(), "~" + std::to_string(EstimatedStackSize) + " bytes");
        }
    }
}

void ModuleAnalyzer::DumpBitcode() {
    PrintSectionHeader("Module Bitcode");
    
    llvm::SmallString<0> Buffer;
    llvm::raw_svector_ostream OS(Buffer);
    
    llvm::WriteBitcodeToFile(*Module, OS);
    
    const char* Data = Buffer.data();
    size_t Size = Buffer.size();
    
    PrintRGB("Binary Size: ", 200, 200, 255);
    PrintRGB(std::to_string(Size) + " bytes\n", 100, 255, 100);
    
    PrintRGB("Hex Dump:\n", 255, 200, 100);
    
    for (size_t i = 0; i < Size; i += 16) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%08x  ", (unsigned int)i);
        PrintRGB(buffer, 150, 150, 150);
        
        for (size_t j = 0; j < 16 && i + j < Size; j++) {
            unsigned char byte = static_cast<unsigned char>(Data[i + j]);
            snprintf(buffer, sizeof(buffer), "%02x ", byte);
            PrintRGB(buffer, 255, 255, 255);
        }
        
        PrintRGB("  |", 100, 100, 100);
        for (size_t j = 0; j < 16 && i + j < Size; j++) {
            unsigned char byte = static_cast<unsigned char>(Data[i + j]);
            char c = (byte >= 32 && byte <= 126) ? byte : '.';
            PrintRGB(std::string(1, c), 200, 200, 200);
        }
        PrintRGB("|\n", 100, 100, 100);
    }
}

void ModuleAnalyzer::DumpVerboseBitcode() {
    PrintSectionHeader("Module Bitcode");
    
    llvm::SmallString<0> Buffer;
    llvm::raw_svector_ostream OS(Buffer);
    
    llvm::WriteBitcodeToFile(*Module, OS);
    
    const char* Data = Buffer.data();
    size_t Size = Buffer.size();
    
    PrintRGB("Binary Size: ", 200, 200, 255);
    PrintRGB(std::to_string(Size) + " bytes\n", 100, 255, 100);
    
    PrintRGB("Binary Dump (first 1024 bytes):\n", 255, 200, 100);
    
    size_t MaxBytes = std::min(Size, size_t(1024));
    
    for (size_t i = 0; i < MaxBytes; i += 8) {
        PrintRGB(std::format("{:08x}  ", i), 150, 150, 150);
        
        for (size_t j = 0; j < 8 && i + j < MaxBytes; j++) {
            unsigned char byte = static_cast<unsigned char>(Data[i + j]);
            
            for (int bit = 7; bit >= 0; bit--) {
                char bitChar = ((byte >> bit) & 1) ? '1' : '0';
                PrintRGB(std::string(1, bitChar), 100, 255, 100);
            }
            PrintRGB(" ", 0, 0, 0);
        }
        std::cout << "\n";
    }
    
    if (Size > 1024) {
        PrintRGB("... truncated (", 150, 150, 150);
        PrintRGB(std::to_string(Size - 1024) + " more bytes)", 150, 150, 150);
        std::cout << "\n";
    }
}

void ModuleAnalyzer::RunFullAnalysis() {
    AnalyzeModuleStructure();
    AnalyzeSymbols();
    AnalyzeOptimizations();
    AnalyzeVectorization();
    DumpBitcode();
    DumpVerboseBitcode();
    DisassembleIR();
    AnalyzeMemoryUsage();
}

void AnalyzeGeneratedCode(llvm::Module* Module) {
    ModuleAnalyzer Analyzer(Module);
    Analyzer.RunFullAnalysis();
}

NativeAssemblyAnalyzer::NativeAssemblyAnalyzer(llvm::Module* M) : Module(M), TargetMachine(nullptr) {
    int result = system("llc --version >nul 2>&1");
    this->HasLLC = (result == 0);
    
    if (!this->HasLLC) {
        result = system("llc --version >/dev/null 2>&1");
        this->HasLLC = (result == 0);
    }
    
    if (this->HasLLC) {
        PrintRGB("info: ", 100, 255, 100);
        PrintRGB("found llc compiler, native assembly generation available\n", 200, 200, 200);
    } else {
        PrintRGB("info: ", 255, 255, 0);
        PrintRGB("llc compiler not found, native assembly generation disabled\n", 200, 200, 200);
    }
}

void NativeAssemblyAnalyzer::PrintNativeAssembly() {
    PrintSectionHeader("Native Assembly");
    
    if (!this->HasLLC) {
        PrintRGB("info: ", 255, 255, 0);
        PrintRGB("native assembly generation requires 'llc' tool\n", 200, 200, 200);
        PrintRGB("note: ", 150, 150, 150);
        PrintRGB("install LLVM tools or add llc to PATH\n", 200, 200, 200);
        return;
    }
    
    std::string TempBC = "temp_module.bc";
    std::string TempASM = "temp_module.s";
    
    std::error_code EC;
    llvm::raw_fd_ostream BCFile(TempBC, EC);
    if (EC) {
        PrintRGB("error: ", 255, 0, 0);
        PrintRGB("failed to create temporary file\n", 200, 100, 100);
        return;
    }
    
    llvm::WriteBitcodeToFile(*Module, BCFile);
    BCFile.close();
    
    std::string Command = "llc -filetype=asm " + TempBC + " -o " + TempASM;
    int Result = system(Command.c_str());
    
    if (Result != 0) {
        PrintRGB("error: ", 255, 0, 0);
        PrintRGB("llc failed to generate assembly\n", 200, 100, 100);
        remove(TempBC.c_str());
        return;
    }
    
    std::ifstream ASMFile(TempASM);
    if (!ASMFile.is_open()) {
        PrintRGB("error: ", 255, 0, 0);
        PrintRGB("failed to read generated assembly\n", 200, 100, 100);
        remove(TempBC.c_str());
        remove(TempASM.c_str());
        return;
    }
    
    std::string line;
    while (std::getline(ASMFile, line)) {
        if (line.empty()) continue;
        
        if (line[0] == '.') {
            PrintRGB(line, 100, 200, 255);
        } else if (line.find(':') != std::string::npos && line[0] != '\t' && line[0] != ' ') {
            PrintRGB(line, 255, 255, 100);
        } else if (line[0] == '\t' || line[0] == ' ') {
            PrintRGB(line, 200, 200, 200);
        } else {
            PrintRGB(line, 150, 150, 150);
        }
        std::cout << "\n";
    }
    
    ASMFile.close();
    remove(TempBC.c_str());
    remove(TempASM.c_str());
}

NativeAssemblyAnalyzer::~NativeAssemblyAnalyzer() {
    delete TargetMachine;
}