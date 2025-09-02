#include "Generator.hh"
#include <iostream>

#include "../../MiddleEnd/AST.hh"
#include "Gens/FunctionGenerator.hh"

#include "PlatformBinary.hh"

Generator::Generator(GL_ASTPackage& pkg) {
    llvm::InitializeAllTargets();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllTargetMCs();

    this->ASTPkg.InputFile = pkg.InputFile;
    this->ASTPkg.OutputFile = pkg.OutputFile;
    this->ASTPkg.Optimisation = pkg.Optimisation;
    this->ASTPkg.Verbose = pkg.Verbose;
    this->ASTPkg.Debug = pkg.Debug;
    this->ASTPkg.Target = pkg.Target;
    this->CInstance.ASTRoot = std::move(pkg.ASTRoot);

    const std::string LLVM_MODULE_NAME = this->ASTPkg.InputFile.stem().string() + ".vexar";
    this->CInstance.IR = std::make_unique<AeroIR>(LLVM_MODULE_NAME);
}

void Generator::CreateEntry() {
    auto* IR = this->CInstance.IR.get();

    llvm::Function* ExistingMain = IR->getModule()->getFunction("main");
    if (ExistingMain) {
        ExistingMain->setName("user_main");
        this->CInstance.FSymbolTable["user_main"] = ExistingMain;
        this->CInstance.FSymbolTable.erase("main");
    }

    IR->createFunction("main", IR->int_t(), {});

    llvm::Function* UserMainFunc = IR->getModule()->getFunction("user_main");
    if (UserMainFunc && UserMainFunc->arg_size() == 0) {
        llvm::Value* ReturnValue = IR->call("user_main", {});
        if (UserMainFunc->getReturnType()->isIntegerTy()) {
            IR->ret(ReturnValue);
        } else if (UserMainFunc->getReturnType()->isVoidTy()) {
            IR->ret(IR->constI32(0));
        } else {
            IR->ret(IR->constI32(1));
        }
    } else {
        IR->ret(IR->constI32(1));
    }
    
    IR->endFunction();
}

void Generator::BuildLLVM() {
    for (const auto& Statement : this->CInstance.ASTRoot->statements) {
        if (Statement->type == NodeType::Function) {
            auto* Func = static_cast<FunctionNode*>(Statement.get());
            GenerateFunction(Func, this->GetIR(), this->CInstance.FSymbolTable);
        }

        else if (Statement->type == NodeType::ExpressionStatement) {
            auto* Expr = static_cast<ExpressionStatementNode*>(Statement.get());
            if (Expr->expression && Expr->expression->type == NodeType::Function) {
                auto* Func = static_cast<FunctionNode*>(Expr->expression.get());
                GenerateFunction(Func, this->GetIR(), this->CInstance.FSymbolTable);
            }
        }
    }

    CreateEntry();
}

void Generator::PrintLLVM() {
    this->CInstance.IR->print();
}

void Generator::CompileTriple(std::string Triple) {
    std::string Target;

    if (Triple.empty()) {
        Target = llvm::sys::getDefaultTargetTriple();
    } else {
        Target = Triple;
    }

    CreatePlatformBinary(this->TakeModule(), Target, this->ASTPkg.OutputFile);
}