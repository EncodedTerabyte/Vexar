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
    this->ASTPkg.Target = pkg.Target;
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

                llvm::Function* Symbol = GenerateFunction(Func, this->GetModulePtr(), this->CInstance.ASymbolTable, this->CInstance.FSymbolTable);
            }
        }
    }

    CreateEntry();
}

void Generator::CreateEntry() {
    llvm::LLVMContext& Context = this->GetModulePtr()->getContext();
    llvm::IRBuilder<> Builder(Context);

    llvm::Function* ExistingMain = this->GetModulePtr()->getFunction("main");
    if (ExistingMain) {
        ExistingMain->setName("user_main");
        this->CInstance.FSymbolTable["user_main"] = ExistingMain;
        this->CInstance.FSymbolTable.erase("main");
    }

    llvm::FunctionType* MainType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(Context),
        false
    );

    llvm::Function* MainFunc = llvm::Function::Create(
        MainType,
        llvm::Function::ExternalLinkage,
        "main",
        this->GetModulePtr()
    );

    llvm::BasicBlock* EntryBlock = llvm::BasicBlock::Create(Context, "entry", MainFunc);
    Builder.SetInsertPoint(EntryBlock);

    llvm::Function* UserMainFunc = this->GetModulePtr()->getFunction("user_main");
    if (UserMainFunc && UserMainFunc->arg_size() == 0) {
        llvm::Value* ReturnValue = Builder.CreateCall(UserMainFunc, {});
        if (UserMainFunc->getReturnType()->isIntegerTy()) {
            Builder.CreateRet(ReturnValue);
        } else if (UserMainFunc->getReturnType()->isVoidTy()) {
            Builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 0));
        } else {
            Builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 1));
        }
    } else {
        Builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), 1));
    }
}

void Generator::PrintLLVM() {
    this->CInstance.IModule->print(llvm::outs(), nullptr);
}

void Generator::OptimiseLLVM() {
   llvm::Module* Module = this->GetModulePtr();
   int Laps = this->ASTPkg.Optimisation;

   if (Laps == 0) return;

   llvm::LoopAnalysisManager LAM;
   llvm::FunctionAnalysisManager FAM;
   llvm::CGSCCAnalysisManager CGAM;
   llvm::ModuleAnalysisManager MAM;

   llvm::PassBuilder PB;
   
   PB.registerModuleAnalyses(MAM);
   PB.registerCGSCCAnalyses(CGAM);
   PB.registerFunctionAnalyses(FAM);
   PB.registerLoopAnalyses(LAM);
   PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

   llvm::ModulePassManager MPM;

   if (Laps == 1) {
       MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O1);
   } else if (Laps == 2) {
       MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
   } else if (Laps >= 3) {
       MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
   }

   MPM.run(*Module, MAM);
}

void Generator::Compile() {

}

void Generator::Link(const std::string& ObjectFile) {

}