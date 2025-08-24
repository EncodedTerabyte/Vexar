#pragma once

#include "LLVMHeader.hh"
#include "Helper/Mapping.hh"
struct GL_ASTPackage {
    fs::path InputFile;
    fs::path OutputFile;

    int Optimisation;  
    std::unique_ptr<ProgramNode> ASTRoot;

    bool Debug;
    bool Verbose;
};

class Generator {
private:
    struct ASTPackage {
        fs::path InputFile;
        fs::path OutputFile;

        int Optimisation = 0;
        bool Debug, Verbose;
    };

    struct CompilerInstance {
        FunctionSymbols FSymbolTable;
        AllocaSymbols ASymbolTable;
        std::unique_ptr<llvm::Module> IModule;
        llvm::LLVMContext IContext;
        llvm::IRBuilder<> IBuilder;
        
        std::unique_ptr<ProgramNode> ASTRoot;
        FunctionNode MainFunc;
        
        CompilerInstance() : IBuilder(IContext) {}
    };

    CompilerInstance CInstance;
    ASTPackage ASTPkg;
public:
    llvm::Module* Generator::GetModulePtr() {
        return this->CInstance.IModule.get();
    }

    std::unique_ptr<llvm::Module> TakeModule() {
        return std::move(this->CInstance.IModule);
    }

    llvm::LLVMContext& GetContext() {
        return this->CInstance.IContext;
    }

    llvm::IRBuilder<>& GetBuilder() {
        return this->CInstance.IBuilder;
    }

    ProgramNode* GetASTRoot() {
        return this->CInstance.ASTRoot.get();
    }

    FunctionNode* GetMainFunc() {
        return& this->CInstance.MainFunc;
    }

    Generator(GL_ASTPackage& pkg);

    void BuildLLVM();
    void PrintLLVM();
};