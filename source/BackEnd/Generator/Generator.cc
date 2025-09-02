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

void Generator::CompileTimeGarbageCollection() {
    llvm::Module* Module = this->GetModulePtr();
    llvm::LLVMContext& Context = Module->getContext();
    
    llvm::Function* freeFunc = Module->getFunction("free");
    if (!freeFunc) {
        llvm::Type* voidType = llvm::Type::getVoidTy(Context);
        llvm::Type* i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(Context), 0);
        llvm::FunctionType* freeFuncType = llvm::FunctionType::get(voidType, {i8PtrType}, false);
        freeFunc = llvm::Function::Create(freeFuncType, llvm::Function::ExternalLinkage, "free", Module);
        this->CInstance.FSymbolTable["free"] = freeFunc;
    }
    
    llvm::Function* reallocFunc = Module->getFunction("realloc");
    if (!reallocFunc) {
        llvm::Type* i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(Context), 0);
        llvm::Type* sizeType = llvm::Type::getInt64Ty(Context);
        llvm::FunctionType* reallocFuncType = llvm::FunctionType::get(i8PtrType, {i8PtrType, sizeType}, false);
        reallocFunc = llvm::Function::Create(reallocFuncType, llvm::Function::ExternalLinkage, "realloc", Module);
        this->CInstance.FSymbolTable["realloc"] = reallocFunc;
    }
    
    for (llvm::Function& F : *Module) {
        if (F.isDeclaration()) continue;
        
        std::map<llvm::Value*, std::set<llvm::Instruction*>> mallocToUsers;
        std::map<llvm::Value*, std::set<llvm::Value*>> aliasMap;
        std::map<llvm::Value*, llvm::Instruction*> lastUsageMap;
        std::map<llvm::Value*, llvm::BasicBlock*> mallocDominanceMap;
        std::set<llvm::Value*> explicitlyFreedPointers;
        std::set<llvm::Value*> escapedPointers;
        std::set<llvm::Value*> conditionallyEscapedPointers;
        std::vector<llvm::CallInst*> allMallocCalls;
        std::vector<llvm::CallInst*> allReallocCalls;
        std::map<llvm::BasicBlock*, std::set<llvm::Value*>> liveAtBlockEnd;
        std::map<llvm::Value*, std::vector<llvm::Instruction*>> usageChains;
        
        for (llvm::BasicBlock& BB : F) {
            for (llvm::Instruction& I : BB) {
                if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if (llvm::Function* calledFunc = CI->getCalledFunction()) {
                        if (calledFunc->getName() == "malloc") {
                            allMallocCalls.push_back(CI);
                            mallocToUsers[CI] = std::set<llvm::Instruction*>();
                            mallocDominanceMap[CI] = &BB;
                        } else if (calledFunc->getName() == "realloc") {
                            allReallocCalls.push_back(CI);
                            if (CI->getNumOperands() > 0) {
                                llvm::Value* oldPtr = CI->getOperand(0);
                                if (!llvm::isa<llvm::ConstantPointerNull>(oldPtr)) {
                                    explicitlyFreedPointers.insert(oldPtr);
                                }
                            }
                        } else if (calledFunc->getName() == "free") {
                            if (CI->getNumOperands() > 0) {
                                explicitlyFreedPointers.insert(CI->getOperand(0));
                            }
                        } else if (calledFunc->getName() == "calloc") {
                            allMallocCalls.push_back(CI);
                            mallocToUsers[CI] = std::set<llvm::Instruction*>();
                            mallocDominanceMap[CI] = &BB;
                        }
                    }
                }
                
                if (llvm::ReturnInst* RI = llvm::dyn_cast<llvm::ReturnInst>(&I)) {
                    if (RI->getReturnValue() && RI->getReturnValue()->getType()->isPointerTy()) {
                        escapedPointers.insert(RI->getReturnValue());
                    }
                }
                
                if (llvm::StoreInst* SI = llvm::dyn_cast<llvm::StoreInst>(&I)) {
                    llvm::Value* storedValue = SI->getValueOperand();
                    llvm::Value* storePtr = SI->getPointerOperand();
                    
                    if (storedValue->getType()->isPointerTy()) {
                        if (llvm::isa<llvm::GlobalVariable>(storePtr)) {
                            escapedPointers.insert(storedValue);
                        } else if (llvm::isa<llvm::Argument>(storePtr)) {
                            escapedPointers.insert(storedValue);
                        }
                    }
                }
                
                if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if (llvm::Function* calledFunc = CI->getCalledFunction()) {
                        if (calledFunc->isDeclaration()) {
                            for (unsigned i = 0; i < CI->getNumOperands() - 1; ++i) {
                                llvm::Value* arg = CI->getOperand(i);
                                if (arg->getType()->isPointerTy()) {
                                    std::string funcName = calledFunc->getName().str();
                                    if (funcName != "printf" && funcName != "sprintf" && 
                                        funcName != "strlen" && funcName != "strcpy" && 
                                        funcName != "strcat" && funcName != "strcmp" &&
                                        funcName != "strncpy" && funcName != "strncat" &&
                                        funcName != "memcpy" && funcName != "memset" &&
                                        funcName != "memmove" && funcName != "memcmp") {
                                        conditionallyEscapedPointers.insert(arg);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        for (llvm::CallInst* mallocCall : allMallocCalls) {
            std::set<llvm::Value*> visited;
            std::vector<llvm::Value*> worklist;
            worklist.push_back(mallocCall);
            
            while (!worklist.empty()) {
                llvm::Value* current = worklist.back();
                worklist.pop_back();
                
                if (visited.count(current)) continue;
                visited.insert(current);
                
                for (llvm::User* user : current->users()) {
                    if (llvm::Instruction* userInst = llvm::dyn_cast<llvm::Instruction>(user)) {
                        mallocToUsers[mallocCall].insert(userInst);
                        usageChains[mallocCall].push_back(userInst);
                        
                        if (llvm::isa<llvm::BitCastInst>(userInst) || 
                            llvm::isa<llvm::GetElementPtrInst>(userInst) ||
                            llvm::isa<llvm::PHINode>(userInst) ||
                            llvm::isa<llvm::SelectInst>(userInst)) {
                            worklist.push_back(userInst);
                            aliasMap[mallocCall].insert(userInst);
                        }
                    }
                }
            }
        }
        
        auto findSafeDominatedInsertionPoint = [&](llvm::Value* mallocVal, llvm::Instruction* lastUse) -> llvm::Instruction* {
            if (!lastUse) return nullptr;
            
            llvm::BasicBlock* insertBB = lastUse->getParent();
            llvm::Instruction* insertPoint = lastUse;
            
            if (llvm::isa<llvm::PHINode>(lastUse)) {
                insertPoint = &insertBB->front();
                while (llvm::isa<llvm::PHINode>(insertPoint)) {
                    insertPoint = insertPoint->getNextNode();
                }
            } else {
                insertPoint = lastUse->getNextNode();
            }
            
            while (insertPoint && insertPoint->getParent() == insertBB) {
                if (llvm::isa<llvm::ReturnInst>(insertPoint) || 
                    llvm::isa<llvm::BranchInst>(insertPoint) ||
                    llvm::isa<llvm::SwitchInst>(insertPoint) ||
                    llvm::isa<llvm::IndirectBrInst>(insertPoint)) {
                    break;
                }
                
                if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(insertPoint)) {
                    if (CI->getCalledFunction() && CI->getCalledFunction()->getName() == "free") {
                        break;
                    }
                }
                
                insertPoint = insertPoint->getNextNode();
            }
            
            if (!insertPoint || insertPoint->getParent() != insertBB) {
                if (insertBB->getTerminator()) {
                    insertPoint = insertBB->getTerminator();
                } else {
                    return nullptr;
                }
            }
            
            return insertPoint;
        };
        
        auto isInSameBasicBlock = [&](llvm::Instruction* inst1, llvm::Instruction* inst2) -> bool {
            return inst1 && inst2 && inst1->getParent() == inst2->getParent();
        };
        
        auto comesBefore = [&](llvm::Instruction* first, llvm::Instruction* second) -> bool {
            if (!isInSameBasicBlock(first, second)) return false;
            
            for (llvm::Instruction& I : *first->getParent()) {
                if (&I == first) return true;
                if (&I == second) return false;
            }
            return false;
        };
        
        auto isPointerStillLiveAfterPoint = [&](llvm::Value* ptr, llvm::Instruction* point) -> bool {
            for (llvm::User* user : ptr->users()) {
                if (llvm::Instruction* userInst = llvm::dyn_cast<llvm::Instruction>(user)) {
                    if (userInst == point) continue;
                    
                    llvm::BasicBlock* userBB = userInst->getParent();
                    llvm::BasicBlock* pointBB = point->getParent();
                    
                    if (userBB == pointBB) {
                        if (!comesBefore(point, userInst)) {
                            return true;
                        }
                    } else {
                        auto reachable = [&](llvm::BasicBlock* from, llvm::BasicBlock* to) -> bool {
                            std::set<llvm::BasicBlock*> visited;
                            std::vector<llvm::BasicBlock*> queue;
                            queue.push_back(from);
                            
                            while (!queue.empty()) {
                                llvm::BasicBlock* current = queue.back();
                                queue.pop_back();
                                
                                if (visited.count(current)) continue;
                                visited.insert(current);
                                
                                if (current == to) return true;
                                
                                for (llvm::BasicBlock* succ : llvm::successors(current)) {
                                    if (!visited.count(succ)) {
                                        queue.push_back(succ);
                                    }
                                }
                            }
                            return false;
                        };
                        
                        if (reachable(pointBB, userBB)) {
                            return true;
                        }
                    }
                }
            }
            return false;
        };
        
        for (llvm::CallInst* mallocCall : allMallocCalls) {
            bool isExplicitlyFreed = false;
            bool escapes = false;
            bool conditionallyEscapes = false;
            llvm::Instruction* lastUsage = nullptr;
            std::set<llvm::Instruction*> allUsages;
            
            std::set<llvm::Value*> visited;
            std::vector<llvm::Value*> worklist;
            worklist.push_back(mallocCall);
            
            while (!worklist.empty()) {
                llvm::Value* current = worklist.back();
                worklist.pop_back();
                
                if (visited.count(current)) continue;
                visited.insert(current);
                
                if (explicitlyFreedPointers.count(current)) {
                    isExplicitlyFreed = true;
                    break;
                }
                
                if (escapedPointers.count(current)) {
                    escapes = true;
                    break;
                }
                
                if (conditionallyEscapedPointers.count(current)) {
                    conditionallyEscapes = true;
                }
                
                for (llvm::User* user : current->users()) {
                    if (llvm::Instruction* userInst = llvm::dyn_cast<llvm::Instruction>(user)) {
                        allUsages.insert(userInst);
                        
                        if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(userInst)) {
                            if (llvm::Function* calledFunc = CI->getCalledFunction()) {
                                if (calledFunc->getName() == "free") {
                                    isExplicitlyFreed = true;
                                    break;
                                } else if (calledFunc->getName() == "realloc") {
                                    if (CI->getOperand(0) == current) {
                                        isExplicitlyFreed = true;
                                        break;
                                    }
                                }
                            }
                        }
                        
                        if (llvm::ReturnInst* RI = llvm::dyn_cast<llvm::ReturnInst>(userInst)) {
                            if (RI->getReturnValue() == current) {
                                escapes = true;
                                break;
                            }
                        }
                        
                        if (llvm::StoreInst* SI = llvm::dyn_cast<llvm::StoreInst>(userInst)) {
                            if (SI->getValueOperand() == current) {
                                llvm::Value* storePtr = SI->getPointerOperand();
                                if (llvm::isa<llvm::GlobalVariable>(storePtr) || 
                                    llvm::isa<llvm::Argument>(storePtr)) {
                                    escapes = true;
                                    break;
                                }
                            }
                        }
                        
                        if (llvm::isa<llvm::BitCastInst>(userInst) || 
                            llvm::isa<llvm::GetElementPtrInst>(userInst) ||
                            llvm::isa<llvm::PHINode>(userInst) ||
                            llvm::isa<llvm::SelectInst>(userInst)) {
                            worklist.push_back(userInst);
                        }
                        
                        if (!lastUsage || 
                            (isInSameBasicBlock(lastUsage, userInst) && comesBefore(lastUsage, userInst)) ||
                            (!isInSameBasicBlock(lastUsage, userInst))) {
                            lastUsage = userInst;
                        }
                    }
                }
            }
            
            if (!isExplicitlyFreed && !escapes && (!conditionallyEscapes || allUsages.size() < 5)) {
                llvm::Instruction* insertPoint = findSafeDominatedInsertionPoint(mallocCall, lastUsage);
                
                if (insertPoint && !llvm::isa<llvm::ReturnInst>(insertPoint) && 
                    !llvm::isa<llvm::UnreachableInst>(insertPoint)) {
                    
                    if (!isPointerStillLiveAfterPoint(mallocCall, insertPoint)) {
                        llvm::IRBuilder<> Builder(insertPoint);
                        
                        llvm::Value* ptr = mallocCall;
                        llvm::Type* i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(Context), 0);
                        if (ptr->getType() != i8PtrType) {
                            ptr = Builder.CreateBitCast(ptr, i8PtrType, "gc_cast");
                        }
                        
                        llvm::Value* nullCheck = Builder.CreateICmpNE(ptr, 
                            llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(i8PtrType)), 
                            "gc_null_check");
                        
                        llvm::BasicBlock* currentBB = insertPoint->getParent();
                        llvm::BasicBlock* freeBB = llvm::BasicBlock::Create(Context, "gc_free", &F);
                        llvm::BasicBlock* contBB = llvm::BasicBlock::Create(Context, "gc_cont", &F);
                        
                        Builder.CreateCondBr(nullCheck, freeBB, contBB);
                        
                        Builder.SetInsertPoint(freeBB);
                        Builder.CreateCall(freeFunc, {ptr});
                        Builder.CreateBr(contBB);
                        
                        std::vector<llvm::Instruction*> toMove;
                        llvm::Instruction* nextInst = insertPoint->getNextNode();
                        while (nextInst) {
                            toMove.push_back(nextInst);
                            nextInst = nextInst->getNextNode();
                        }
                        
                        for (llvm::Instruction* inst : toMove) {
                            inst->removeFromParent();
                            inst->insertInto(contBB, contBB->end());
                        }
                        
                        Builder.SetInsertPoint(contBB);
                    }
                }
            }
        }
        
        std::vector<llvm::Instruction*> instructionsToRemove;
        std::set<llvm::BasicBlock*> processedBlocks;
        
        for (llvm::BasicBlock& BB : F) {
            if (processedBlocks.count(&BB)) continue;
            
            for (llvm::Instruction& I : BB) {
                if (llvm::BranchInst* BI = llvm::dyn_cast<llvm::BranchInst>(&I)) {
                    if (BI->isUnconditional()) {
                        llvm::BasicBlock* target = BI->getSuccessor(0);
                        if (target == BI->getParent()) {
                            llvm::IRBuilder<> Builder(&I);
                            llvm::Type* returnType = F.getReturnType();
                            if (returnType->isVoidTy()) {
                                Builder.CreateRetVoid();
                            } else if (returnType->isIntegerTy()) {
                                Builder.CreateRet(llvm::ConstantInt::get(returnType, 0));
                            } else {
                                Builder.CreateRet(llvm::Constant::getNullValue(returnType));
                            }
                            instructionsToRemove.push_back(&I);
                            processedBlocks.insert(&BB);
                            break;
                        }
                    } else if (BI->isConditional()) {
                        if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(BI->getCondition())) {
                            llvm::BasicBlock* target = nullptr;
                            if (CI->isOne()) {
                                target = BI->getSuccessor(0);
                            } else {
                                target = BI->getSuccessor(1);
                            }
                            
                            if (target == BI->getParent()) {
                                llvm::IRBuilder<> Builder(&I);
                                llvm::Type* returnType = F.getReturnType();
                                if (returnType->isVoidTy()) {
                                    Builder.CreateRetVoid();
                                } else if (returnType->isIntegerTy()) {
                                    Builder.CreateRet(llvm::ConstantInt::get(returnType, 0));
                                } else {
                                    Builder.CreateRet(llvm::Constant::getNullValue(returnType));
                                }
                                instructionsToRemove.push_back(&I);
                                processedBlocks.insert(&BB);
                                break;
                            }
                        }
                    }
                }
                
                if (llvm::SwitchInst* SI = llvm::dyn_cast<llvm::SwitchInst>(&I)) {
                    for (auto Case : SI->cases()) {
                        if (Case.getCaseSuccessor() == SI->getParent()) {
                            llvm::IRBuilder<> Builder(&I);
                            llvm::Type* returnType = F.getReturnType();
                            if (returnType->isVoidTy()) {
                                Builder.CreateRetVoid();
                            } else if (returnType->isIntegerTy()) {
                                Builder.CreateRet(llvm::ConstantInt::get(returnType, 0));
                            } else {
                                Builder.CreateRet(llvm::Constant::getNullValue(returnType));
                            }
                            instructionsToRemove.push_back(&I);
                            processedBlocks.insert(&BB);
                            break;
                        }
                    }
                }
            }
        }
        
        for (llvm::Instruction* I : instructionsToRemove) {
            if (I->getParent()) {
                I->eraseFromParent();
            }
        }
        
        std::map<llvm::BasicBlock*, std::vector<llvm::CallInst*>> blockToFreeCalls;
        for (llvm::BasicBlock& BB : F) {
            for (llvm::Instruction& I : BB) {
                if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if (CI->getCalledFunction() && CI->getCalledFunction()->getName() == "free") {
                        blockToFreeCalls[&BB].push_back(CI);
                    }
                }
            }
        }
        
        for (auto& pair : blockToFreeCalls) {
            llvm::BasicBlock* BB = pair.first;
            auto& freeCalls = pair.second;
            
            if (freeCalls.size() > 1) {
                std::set<llvm::Value*> freedPointers;
                std::vector<llvm::CallInst*> redundantFrees;
                
                for (llvm::CallInst* freeCall : freeCalls) {
                    llvm::Value* ptr = freeCall->getOperand(0);
                    if (freedPointers.count(ptr)) {
                        redundantFrees.push_back(freeCall);
                    } else {
                        freedPointers.insert(ptr);
                    }
                }
                
                for (llvm::CallInst* redundantFree : redundantFrees) {
                    redundantFree->eraseFromParent();
                }
            }
        }
        
        for (llvm::BasicBlock& BB : F) {
            for (auto it = BB.begin(); it != BB.end();) {
                llvm::Instruction& I = *it;
                ++it;
                
                if (llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if (CI->getCalledFunction() && CI->getCalledFunction()->getName() == "malloc") {
                        if (CI->use_empty()) {
                            CI->eraseFromParent();
                        }
                    }
                }
            }
        }
    }
}

void Generator::ValidateModule(bool EmitWarnings) {
    llvm::Module* Module = this->GetModulePtr();
    FunctionSymbols& UserFunctions = this->CInstance.FSymbolTable;
    
    static const std::unordered_set<std::string> systemFunctions = {
        "main", "printf", "scanf", "malloc", "free", "sprintf", "strlen",
        "strcpy", "strcat", "strtod", "strcmp", "atoi", "atof", "exit", 
        "realloc", "atexit", "calloc", "memcpy", "memset", "memmove", 
        "memcmp", "strncpy", "strncat"
    };
    
    std::unordered_map<std::string, bool> declaredVariables;
    std::unordered_map<std::string, bool> initializedVariables;
    std::unordered_map<std::string, llvm::Type*> variableTypes;
    
    for (auto& Function : *Module) {
        std::unordered_map<std::string, bool> localVars;
        std::unordered_map<std::string, bool> localInitialized;
        std::unordered_map<std::string, llvm::Type*> localTypes;
        bool hasReturn = false;
        bool unreachableCodeWarned = false;
        
        if (systemFunctions.find(Function.getName().str()) == systemFunctions.end()) {
            auto userFuncIt = UserFunctions.find(Function.getName().str());
            if (userFuncIt == UserFunctions.end()) {
                Write("Validator", "Function '" + Function.getName().str() + "' is defined but not in symbol table", 2, true, true, "");
            }
        }
        
        for (auto& BasicBlock : Function) {
            for (auto& Instruction : BasicBlock) {
                
                if (hasReturn && !unreachableCodeWarned) {
                    if (!EmitWarnings)
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

                    if (CalledFunc && systemFunctions.find(CalledFunc->getName().str()) == systemFunctions.end()) {
                        auto userFuncIt = UserFunctions.find(CalledFunc->getName().str());
                        if (userFuncIt == UserFunctions.end()) {
                            Write("Validator", "Call to undefined function '" + CalledFunc->getName().str() + "'", 2, true, true, "");
                        } else {
                            llvm::Function* UserFunc = userFuncIt->second;
                            if (CallInst->getNumOperands() != UserFunc->arg_size() + 1) {
                                Write("Validator", "Function '" + CalledFunc->getName().str() + "' called with " + std::to_string(CallInst->getNumOperands() - 1) + " arguments, expected " + std::to_string(UserFunc->arg_size()), 2, true, true, "");
                            }

                            for (size_t i = 0; i < std::min((size_t)(CallInst->getNumOperands() - 1), (size_t)UserFunc->arg_size()); ++i) {
                                llvm::Type* ArgType = CallInst->getOperand(i)->getType();
                                llvm::Type* ParamType = UserFunc->getFunctionType()->getParamType(i);
                                if (ArgType != ParamType && !((ArgType->isIntegerTy() && ParamType->isFloatingPointTy()) || (ArgType->isFloatingPointTy() && ParamType->isIntegerTy()))) {
                                    Write("Validator", "Type mismatch in function call to '" + CalledFunc->getName().str() + "' at parameter " + std::to_string(i + 1), 2, true, true, "");
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
                                if (!EmitWarnings)
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
                        if (!EmitWarnings)
                            Write("Validator", "Potentially unsafe cast from string to number - may cause undefined behavior", 1, true, true, "");
                    }
                    
                    if (SrcType->isIntegerTy() && DstType->isPointerTy()) {
                        if (!EmitWarnings)
                            Write("Validator", "Potentially unsafe cast from number to string pointer - may cause memory corruption", 1, true, true, "");
                    }
                    
                    if (SrcType->isIntegerTy() && DstType->isIntegerTy() && 
                        SrcType->getIntegerBitWidth() > DstType->getIntegerBitWidth()) {
                        if (!EmitWarnings)
                            Write("Validator", "Potentially unsafe integer truncation cast - data loss may occur", 1, true, true, "");
                    }
                    
                    if (SrcType->isFloatingPointTy() && DstType->isIntegerTy()) {
                        if (!EmitWarnings)
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
            systemFunctions.find(Function.getName().str()) == systemFunctions.end()) {
            Write("Validator", "Function '" + Function.getName().str() + "' may not return a value on all code paths", 2, true, true, "");
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