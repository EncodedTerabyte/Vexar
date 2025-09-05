#include "LLVMHeader.hh"

class ModuleAnalyzer {
private:
    llvm::Module* Module;
    
public:
    ModuleAnalyzer(llvm::Module* M);
    void AnalyzeSymbols();
    void AnalyzeModuleStructure();
    void DisassembleIR();
    void AnalyzeOptimizations();
    void AnalyzeVectorization();
    void AnalyzeMemoryUsage();
    void DumpBitcode();
    void DumpVerboseBitcode();
    void RunFullAnalysis();
};

class NativeAssemblyAnalyzer {
private:
    llvm::Module* Module;
    llvm::TargetMachine* TargetMachine;
    bool HasLLC;
    
public:
    NativeAssemblyAnalyzer(llvm::Module* M);
    void PrintNativeAssembly();
    ~NativeAssemblyAnalyzer();
};

void AnalyzeGeneratedCode(llvm::Module* Module);