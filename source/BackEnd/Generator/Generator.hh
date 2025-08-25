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

    std::string Target;
};

class Generator {
private:
    void CreateEntry();

    struct ASTPackage {
        fs::path InputFile;
        fs::path OutputFile;

        int Optimisation = 0;
        bool Debug, Verbose;

        std::string Target;
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
    llvm::Module* GetModulePtr() {
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
    void ValidateModule();
    void OptimiseLLVM();
    void Compile();
    void Link(const std::string& ObjectFile);

};