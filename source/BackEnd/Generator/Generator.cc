#include "Generator.hh"
#include <iostream>

#include "../../MiddleEnd/AST.hh"
#include "Gens/FunctionGenerator.hh"

Generator::Generator(GL_ASTPackage& pkg) {
    llvm::InitializeAllTargets();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllTargetMCs();

    this->ASTPkg.InputFile = pkg.InputFile;
    this->ASTPkg.OutputFile = pkg.OutputFile;
    this->ASTPkg.Optimisation = pkg.Optimisation;
    this->ASTPkg.Verbose = pkg.Verbose;
    this->ASTPkg.Debug = pkg.Debug;
    this->CInstance.ASTRoot = std::move(pkg.ASTRoot);

    llvm::LLVMContext& ctx = this->CInstance.IContext;

    const std::string LLVM_MODULE_NAME = this->ASTPkg.InputFile.stem().string() + ".vexar";
    this->CInstance.IModule = std::make_unique<llvm::Module>(LLVM_MODULE_NAME, ctx);
}

void Generator::BuildLLVM() {
    for (const auto& Statement : this->CInstance.ASTRoot->statements) {
        if (Statement->type == NodeType::ExpressionStatement) {
            auto* Expr = static_cast<ExpressionStatementNode*>(Statement.get());

            if (Expr->expression->type == NodeType::Function) {
                auto* Func = static_cast<FunctionNode*>(Expr->expression.get());

                llvm::Function* Symbol = GenerateFunction(Func, this->GetModulePtr(), this->CInstance.ASymbolTable);
                this->CInstance.FSymbolTable[Symbol->getName().str()] = Symbol;
            }
        }
    }
}

void Generator::PrintLLVM() {
    this->CInstance.IModule->print(llvm::outs(), nullptr);
}