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

    llvm::LLVMContext& ctx = this->CInstance.IContext;

    const std::string LLVM_MODULE_NAME = this->ASTPkg.InputFile.stem().string() + ".vexar";
    this->CInstance.IModule = std::make_unique<llvm::Module>(LLVM_MODULE_NAME, ctx);
}

void Generator::BuildLLVM() {
    for (const auto& Statement : this->CInstance.ASTRoot->statements) {
        if (Statement->type == NodeType::Function) {
            auto* Func = static_cast<FunctionNode*>(Statement.get());
            /*llvm::Function* Symbol*/ GenerateFunction(Func, this->GetModulePtr(), this->CInstance.ASymbolTable, this->CInstance.FSymbolTable);
        }

        else if (Statement->type == NodeType::ExpressionStatement) {
            auto* Expr = static_cast<ExpressionStatementNode*>(Statement.get());
            if (Expr->expression && Expr->expression->type == NodeType::Function) {
                auto* Func = static_cast<FunctionNode*>(Expr->expression.get());
                /*llvm::Function* Symbol*/ GenerateFunction(Func, this->GetModulePtr(), this->CInstance.ASymbolTable, this->CInstance.FSymbolTable);
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
    this->GetModulePtr()->print(llvm::outs(), nullptr);
}

void Generator::ValidateModule() {
    llvm::Module* Module = this->GetModulePtr();
    FunctionSymbols& UserFunctions = this->CInstance.FSymbolTable;
    
    std::unordered_map<std::string, bool> declaredVariables;
    std::unordered_map<std::string, bool> initializedVariables;
    std::unordered_map<std::string, llvm::Type*> variableTypes;
    
    for (auto& Function : *Module) {
        std::unordered_map<std::string, bool> localVars;
        std::unordered_map<std::string, bool> localInitialized;
        std::unordered_map<std::string, llvm::Type*> localTypes;
        bool hasReturn = false;
        bool unreachableCodeWarned = false;
        
        if (Function.getName() != "main" && Function.getName() != "printf" && 
            Function.getName() != "scanf" && Function.getName() != "malloc" && 
            Function.getName() != "sprintf" && Function.getName() != "strlen" &&
            Function.getName() != "strcpy" && Function.getName() != "strcat" &&
            Function.getName() != "strtod" && Function.getName() != "strcmp" &&
            Function.getName() != "atoi" && Function.getName() != "atof" &&
            Function.getName() != "exit") {
            
            auto userFuncIt = UserFunctions.find(Function.getName().str());
            if (userFuncIt == UserFunctions.end()) {
                Write("Validator", "Function '" + Function.getName().str() + "' is defined but not in symbol table", 2, true, true, "");
            }
        }
        
        for (auto& BasicBlock : Function) {
            for (auto& Instruction : BasicBlock) {
                
                if (hasReturn && !unreachableCodeWarned) {
                    Write("Validator", "Unreachable code detected after return statement in function '" + Function.getName().str() + "'", 1, true, true, "");
                    unreachableCodeWarned = true;
                }
                
                if (auto* AllocaInst = llvm::dyn_cast<llvm::AllocaInst>(&Instruction)) {
                    std::string varName = AllocaInst->getName().str();
                    if (!varName.empty()) {
                        if (localVars.find(varName) != localVars.end()) {
                            Write("Validator", "Variable '" + varName + "' declared multiple times in function '" + Function.getName().str() + "'", 2, true, true, "");
                        }
                        localVars[varName] = true;
                        localTypes[varName] = AllocaInst->getAllocatedType();
                    }
                }
                
                if (auto* StoreInst = llvm::dyn_cast<llvm::StoreInst>(&Instruction)) {
                    if (auto* AllocaInst = llvm::dyn_cast<llvm::AllocaInst>(StoreInst->getPointerOperand())) {
                        std::string varName = AllocaInst->getName().str();
                        if (!varName.empty()) {
                            localInitialized[varName] = true;
                        }
                    }
                    
                    llvm::Value* Ptr = StoreInst->getPointerOperand();
                    if (llvm::isa<llvm::ConstantPointerNull>(Ptr)) {
                        Write("Validator", "Assignment to null pointer detected - check variable initialization", 2, true, true, "");
                    }
                }
                
                if (auto* LoadInst = llvm::dyn_cast<llvm::LoadInst>(&Instruction)) {
                    if (auto* AllocaInst = llvm::dyn_cast<llvm::AllocaInst>(LoadInst->getPointerOperand())) {
                        std::string varName = AllocaInst->getName().str();
                        if (!varName.empty() && !localInitialized.count(varName)) {
                            Write("Validator", "Variable '" + varName + "' used before initialization", 2, true, true, "");
                        }
                    }
                    
                    llvm::Value* Ptr = LoadInst->getPointerOperand();
                    if (llvm::isa<llvm::ConstantPointerNull>(Ptr)) {
                        Write("Validator", "Reading from null pointer detected - check variable declaration", 2, true, true, "");
                    }
                }
                
                if (auto* CallInst = llvm::dyn_cast<llvm::CallInst>(&Instruction)) {
                    llvm::Function* CalledFunc = CallInst->getCalledFunction();

                    if (CalledFunc && CalledFunc->getName() != "printf" &&
                        CalledFunc->getName() != "scanf" &&
                        CalledFunc->getName() != "malloc" &&
                        CalledFunc->getName() != "sprintf" &&
                        CalledFunc->getName() != "strlen" &&
                        CalledFunc->getName() != "strcpy" &&
                        CalledFunc->getName() != "strcat" &&
                        CalledFunc->getName() != "strtod" &&
                        CalledFunc->getName() != "strcmp" &&
                        CalledFunc->getName() != "atoi" &&
                        CalledFunc->getName() != "atof" &&
                        CalledFunc->getName() != "exit") {

                      auto userFuncIt =
                          UserFunctions.find(CalledFunc->getName().str());
                      if (userFuncIt == UserFunctions.end()) {
                        Write("Validator",
                              "Call to undefined function '" +
                                  CalledFunc->getName().str() + "'",
                              2, true, true, "");
                      } else {
                        llvm::Function *UserFunc = userFuncIt->second;
                        if (CallInst->getNumOperands() !=
                            UserFunc->arg_size() + 1) {
                          Write("Validator",
                                "Function '" + CalledFunc->getName().str() +
                                    "' called with " +
                                    std::to_string(CallInst->getNumOperands() -
                                                   1) +
                                    " arguments, expected " +
                                    std::to_string(UserFunc->arg_size()),
                                2, true, true, "");
                        }

                        for (size_t i = 0;
                             i <
                             std::min((size_t)(CallInst->getNumOperands() - 1),
                                      (size_t)UserFunc->arg_size());
                             ++i) {
                          llvm::Type *ArgType =
                              CallInst->getOperand(i)->getType();
                          llvm::Type *ParamType =
                              UserFunc->getFunctionType()->getParamType(i);
                          if (ArgType != ParamType &&
                              !((ArgType->isIntegerTy() &&
                                 ParamType->isFloatingPointTy()) ||
                                (ArgType->isFloatingPointTy() &&
                                 ParamType->isIntegerTy()))) {
                            Write("Validator",
                                  "Type mismatch in function call to '" +
                                      CalledFunc->getName().str() +
                                      "' at parameter " + std::to_string(i + 1),
                                  2, true, true, "");
                          }
                        }
                      }
                    }

                    if (CalledFunc && CalledFunc->getName() == "printf") {
                        if (CallInst->getNumOperands() < 2) {
                            Write("Validator", "print() or println() function call failed - invalid argument count in generated code", 2, true, true, "");
                        }
                    }
                    
                    if (CalledFunc && CalledFunc->getName() == "scanf") {
                        if (CallInst->getNumOperands() < 2) {
                            Write("Validator", "input() function call failed - invalid argument count in generated code", 2, true, true, "");
                        }
                    }
                    
                    if (CalledFunc && CalledFunc->getName() == "malloc") {
                        if (CallInst->getNumOperands() < 1) {
                            Write("Validator", "Memory allocation failed - invalid argument count in string operation", 2, true, true, "");
                        }
                        llvm::Value* SizeArg = CallInst->getOperand(0);
                        if (auto* ConstInt = llvm::dyn_cast<llvm::ConstantInt>(SizeArg)) {
                            if (ConstInt->getZExtValue() == 0) {
                                Write("Validator", "Memory allocation requested zero bytes - potential issue with string concatenation or str() function", 1, true, true, "");
                            }
                        }
                    }
                    
                    if (CalledFunc && CalledFunc->getName() == "sprintf") {
                        if (CallInst->getNumOperands() < 3) {
                            Write("Validator", "String conversion failed - str() function or type casting generated invalid code", 2, true, true, "");
                        }
                    }
                    
                    if (CalledFunc && CalledFunc->getName() == "strlen") {
                        llvm::Value* StrArg = CallInst->getOperand(0);
                        if (llvm::isa<llvm::ConstantPointerNull>(StrArg)) {
                            Write("Validator", "String length operation on null string - check your string variables and concatenation", 2, true, true, "");
                        }
                    }
                    
                    if ((CalledFunc && CalledFunc->getName() == "strcpy") || 
                        (CalledFunc && CalledFunc->getName() == "strcat")) {
                        if (CallInst->getNumOperands() < 2) {
                            Write("Validator", "String concatenation (+) operation failed - insufficient arguments", 2, true, true, "");
                        }
                        llvm::Value* DestArg = CallInst->getOperand(0);
                        llvm::Value* SrcArg = CallInst->getOperand(1);
                        if (llvm::isa<llvm::ConstantPointerNull>(DestArg) || llvm::isa<llvm::ConstantPointerNull>(SrcArg)) {
                            Write("Validator", "String concatenation (+) attempted with null string", 2, true, true, "");
                        }
                    }
                }
                
                if (auto* BranchInst = llvm::dyn_cast<llvm::BranchInst>(&Instruction)) {
                    if (BranchInst->isConditional()) {
                        llvm::Value* Condition = BranchInst->getCondition();
                        if (!Condition->getType()->isIntegerTy(1)) {
                            Write("Validator", "If statement condition is not boolean-compatible", 2, true, true, "");
                        }
                        
                        if (auto* ConstInt = llvm::dyn_cast<llvm::ConstantInt>(Condition)) {
                            if (ConstInt->isZero()) {
                                Write("Validator", "If statement condition is always false - branch will never be taken", 1, true, true, "");
                            } else {
                                Write("Validator", "If statement condition is always true - else branch will never be taken", 1, true, true, "");
                            }
                        }
                    }
                }
                
                if (auto* BinOp = llvm::dyn_cast<llvm::BinaryOperator>(&Instruction)) {
                    llvm::Value* LHS = BinOp->getOperand(0);
                    llvm::Value* RHS = BinOp->getOperand(1);
                    
                    if (BinOp->getOpcode() == llvm::Instruction::SDiv || 
                        BinOp->getOpcode() == llvm::Instruction::UDiv ||
                        BinOp->getOpcode() == llvm::Instruction::FDiv) {
                        
                        if (auto* ConstInt = llvm::dyn_cast<llvm::ConstantInt>(RHS)) {
                            if (ConstInt->getZExtValue() == 0) {
                                Write("Validator", "Division by zero detected in arithmetic expression", 2, true, true, "");
                            }
                        }
                        
                        if (auto* ConstFP = llvm::dyn_cast<llvm::ConstantFP>(RHS)) {
                            if (ConstFP->getValueAPF().isZero()) {
                                Write("Validator", "Division by zero detected in arithmetic expression", 2, true, true, "");
                            }
                        }
                    }
                    
                    if (BinOp->getOpcode() == llvm::Instruction::SRem ||
                        BinOp->getOpcode() == llvm::Instruction::URem) {
                        
                        if (auto* ConstInt = llvm::dyn_cast<llvm::ConstantInt>(RHS)) {
                            if (ConstInt->getZExtValue() == 0) {
                                Write("Validator", "Modulo by zero detected in arithmetic expression", 2, true, true, "");
                            }
                        }
                    }
                    
                    if (!LHS->getType()->isPointerTy() && !RHS->getType()->isPointerTy()) {
                        if (LHS->getType() != RHS->getType()) {
                            if (!((LHS->getType()->isIntegerTy() && RHS->getType()->isFloatingPointTy()) ||
                                  (LHS->getType()->isFloatingPointTy() && RHS->getType()->isIntegerTy()) ||
                                  (LHS->getType()->isFloatingPointTy() && RHS->getType()->isFloatingPointTy()) ||
                                  (LHS->getType()->isIntegerTy() && RHS->getType()->isIntegerTy()))) {
                                Write("Validator", "Type mismatch in binary operation", 1, true, true, "");
                            }
                        }
                    }
                }
                
                if (auto* GetElementPtrInst = llvm::dyn_cast<llvm::GetElementPtrInst>(&Instruction)) {
                    llvm::Value* Ptr = GetElementPtrInst->getPointerOperand();
                    if (llvm::isa<llvm::ConstantPointerNull>(Ptr)) {
                        Write("Validator", "Array or string access on null pointer detected", 2, true, true, "");
                    }
                    
                    for (auto& Index : GetElementPtrInst->indices()) {
                        if (auto* ConstInt = llvm::dyn_cast<llvm::ConstantInt>(Index)) {
                            if (ConstInt->isNegative()) {
                                Write("Validator", "Negative array index detected - potential buffer underrun", 1, true, true, "");
                            }
                        }
                    }
                }
                
                if (auto* CastInst = llvm::dyn_cast<llvm::CastInst>(&Instruction)) {
                    llvm::Type* SrcType = CastInst->getSrcTy();
                    llvm::Type* DstType = CastInst->getDestTy();
                    
                    if (SrcType->isPointerTy() && DstType->isIntegerTy()) {
                        Write("Validator", "Potentially unsafe cast from string to number - may cause undefined behavior", 1, true, true, "");
                    }
                    
                    if (SrcType->isIntegerTy() && DstType->isPointerTy()) {
                        Write("Validator", "Potentially unsafe cast from number to string pointer - may cause memory corruption", 1, true, true, "");
                    }
                    
                    if (SrcType->isIntegerTy() && DstType->isIntegerTy() && 
                        SrcType->getIntegerBitWidth() > DstType->getIntegerBitWidth()) {
                        Write("Validator", "Potentially unsafe integer truncation cast - data loss may occur", 1, true, true, "");
                    }
                    
                    if (SrcType->isFloatingPointTy() && DstType->isIntegerTy()) {
                        Write("Validator", "Potentially unsafe cast from float to integer - decimal part will be lost", 1, true, true, "");
                    }
                }
                
                if (auto* RetInst = llvm::dyn_cast<llvm::ReturnInst>(&Instruction)) {
                    hasReturn = true;
                    llvm::Type* FuncRetType = Function.getReturnType();
                    if (RetInst->getReturnValue()) {
                        llvm::Type* RetValType = RetInst->getReturnValue()->getType();
                        if (FuncRetType != RetValType && 
                            !((FuncRetType->isIntegerTy() && RetValType->isFloatingPointTy()) ||
                              (FuncRetType->isFloatingPointTy() && RetValType->isIntegerTy()))) {
                            Write("Validator", "Return type mismatch in function '" + Function.getName().str() + "'", 2, true, true, "");
                        }
                    } else if (!FuncRetType->isVoidTy()) {
                        Write("Validator", "Missing return value for non-void function '" + Function.getName().str() + "'", 2, true, true, "");
                    }
                }
            }
        }

        if (!Function.getReturnType()->isVoidTy() && !hasReturn &&
            Function.getName() != "printf" && Function.getName() != "scanf" &&
            Function.getName() != "malloc" && Function.getName() != "sprintf" &&
            Function.getName() != "strlen" && Function.getName() != "strcpy" &&
            Function.getName() != "strcat" && Function.getName() != "strtod" &&
            Function.getName() != "strcmp" && Function.getName() != "atoi" &&
            Function.getName() != "atof" && Function.getName() != "exit") {
          Write("Validator",
                "Function '" + Function.getName().str() +
                    "' may not return a value on all code paths",
                2, true, true, "");
        }

        for (const auto& var : localVars) {
            if (!localInitialized.count(var.first)) {
                Write("Validator", "Variable '" + var.first + "' declared but never initialized in function '" + Function.getName().str() + "'", 1, true, true, "");
            }
        }
    }
    
    Write("Validator", "Module validation completed", 3, true, true, "");
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

void Generator::CompileTriple(std::string Triple) {
    std::string Target;

    if (Triple.empty()) {
        Target = llvm::sys::getDefaultTargetTriple();
    } else {
        Target = Triple;
    }

    CreatePlatformBinary(this->TakeModule(), Target, this->ASTPkg.OutputFile);
}

void Generator::Link(fs::path ObjectFile) {}