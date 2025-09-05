#pragma once

#include "AeroIR/source/AeroIR.hh"
#include "LLVMHeader.hh"
#include "Helper/Types.hh"

struct GL_ASTPackage {
    fs::path InputFile;
    fs::path OutputFile;

    int Optimisation;  
    std::unique_ptr<ProgramNode> ASTRoot;

    bool Debug;
    bool Verbose;
    bool RunAfterCompile;

    std::string CompilerTarget;
};

class Generator {
private:
    void CreateEntry();

    struct ASTPackage {
        fs::path InputFile;
        fs::path OutputFile;

        int Optimisation = 0;
        bool Debug, Verbose, RunAfterCompile;

        std::string CompilerTarget;
    };

    struct CompilerInstance {
        FunctionSymbols FSymbolTable;
        std::unique_ptr<AeroIR> IR;
        std::unique_ptr<ProgramNode> ASTRoot;
        FunctionNode MainFunc;
        
        CompilerInstance() {}
    };

    CompilerInstance CInstance;
    ASTPackage ASTPkg;
public:
    llvm::Module* GetModulePtr() {
        return this->CInstance.IR->getModule();
    }

    std::unique_ptr<llvm::Module> TakeModule() {
        return std::unique_ptr<llvm::Module>(this->CInstance.IR->getModule());
    }

    llvm::LLVMContext& GetContext() {
        return *this->CInstance.IR->getContext();
    }

    llvm::IRBuilder<>& GetBuilder() {
        return *this->CInstance.IR->getBuilder();
    }

    AeroIR* GetIR() {
        return this->CInstance.IR.get();
    }

    ProgramNode* GetASTRoot() {
        return this->CInstance.ASTRoot.get();
    }

    FunctionNode* GetMainFunc() {
        return& this->CInstance.MainFunc;
    }

    Generator(GL_ASTPackage& pkg);

    void BuildModule();
    void PrintModule();
    bool ValidateModule();
    void OptimiseModule();
    void CompileTriple();
};