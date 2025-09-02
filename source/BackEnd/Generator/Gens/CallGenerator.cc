#include "CallGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCall(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods, BuiltinSymbols& BuiltIns) {
    if (!Expr) {
        Write("Function Call", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);

    auto* FuncCallNode = static_cast<FunctionCallNode*>(Expr.get());
    if (!FuncCallNode) {
        Write("Function Call", "Failed to cast ASTNode to FunctionCallNode" + Location, 2, true, true, "");
        return nullptr;
    }
    
    auto BuiltinIt = BuiltIns.find(FuncCallNode->name);
    if (BuiltinIt != BuiltIns.end()) {
        llvm::Value* Result = BuiltinIt->second(FuncCallNode->arguments, Builder, SymbolStack, Methods);
        if (!Result) {
            Write("Function Call", "Failed to execute builtin function: " + FuncCallNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    }
    
    auto it = Methods.find(FuncCallNode->name);
    if (it == Methods.end()) {
        Write("Function Call", "Function not found: " + FuncCallNode->name + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Function* Function = it->second;
    if (!Function) {
        Write("Function Call", "Invalid function pointer for: " + FuncCallNode->name + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::FunctionType* FuncType = Function->getFunctionType();
    
    std::vector<llvm::Value*> Args;
    for (size_t i = 0; i < FuncCallNode->arguments.size(); ++i) {
        if (i < FuncType->getNumParams() && FuncType->getParamType(i)->isPointerTy()) {
            if (FuncCallNode->arguments[i]->type == NodeType::Identifier) {
                auto* IdentNode = static_cast<IdentifierNode*>(FuncCallNode->arguments[i].get());
                llvm::Value* arrayPtr = nullptr;
                for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
                    auto found = it->find(IdentNode->name);
                    if (found != it->end()) {
                        arrayPtr = found->second;
                        break;
                    }
                }
                
                if (!arrayPtr) {
                    Write("Function Call", "Undefined array identifier: " + IdentNode->name + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
                if (!allocaInst) {
                    Write("Function Call", "Array identifier is not an allocated variable: " + IdentNode->name + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::Type* allocatedType = allocaInst->getAllocatedType();
                llvm::Type* expectedParamType = FuncType->getParamType(i);
                
                if (allocatedType->isArrayTy()) {
                    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                    if (!arrayType) {
                        Write("Function Call", "Expected array type" + Location, 2, true, true, "");
                        return nullptr;
                    }
                    
                    // Check if it's a multi-dimensional array
                    if (arrayType->getElementType()->isArrayTy()) {
                        // Multi-dimensional array - use complex heap allocation
                        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                        if (!mallocFunc) {
                            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                                {llvm::Type::getInt64Ty(Builder.getContext())},
                                false
                            );
                            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
                        }
                        
                        llvm::ArrayType* innerArrayType = llvm::dyn_cast<llvm::ArrayType>(arrayType->getElementType());
                        uint64_t outerSize = arrayType->getNumElements();
                        uint64_t innerSize = innerArrayType->getNumElements();
                        
                        llvm::Value* outerArraySize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), outerSize * 8);
                        llvm::Value* heapArrayPtr = Builder.CreateCall(mallocFunc, {outerArraySize});
                        heapArrayPtr = Builder.CreatePointerCast(heapArrayPtr, llvm::PointerType::get(llvm::PointerType::get(Builder.getInt32Ty(), 0), 0));
                        
                        for (uint64_t i = 0; i < outerSize; ++i) {
                            llvm::Value* innerArraySize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), innerSize * 4);
                            llvm::Value* innerHeapPtr = Builder.CreateCall(mallocFunc, {innerArraySize});
                            innerHeapPtr = Builder.CreatePointerCast(innerHeapPtr, llvm::PointerType::get(Builder.getInt32Ty(), 0));
                            
                            for (uint64_t j = 0; j < innerSize; ++j) {
                                std::vector<llvm::Value*> stackIndices = {
                                    llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                                    llvm::ConstantInt::get(Builder.getInt32Ty(), i),
                                    llvm::ConstantInt::get(Builder.getInt32Ty(), j)
                                };
                                llvm::Value* stackElementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, stackIndices);
                                llvm::Value* stackValue = Builder.CreateLoad(Builder.getInt32Ty(), stackElementPtr);
                                
                                llvm::Value* heapElementPtr = Builder.CreateInBoundsGEP(Builder.getInt32Ty(), innerHeapPtr, llvm::ConstantInt::get(Builder.getInt32Ty(), j));
                                Builder.CreateStore(stackValue, heapElementPtr);
                            }
                            
                            llvm::Value* outerElementPtr = Builder.CreateInBoundsGEP(llvm::PointerType::get(Builder.getInt32Ty(), 0), heapArrayPtr, llvm::ConstantInt::get(Builder.getInt32Ty(), i));
                            Builder.CreateStore(innerHeapPtr, outerElementPtr);
                        }
                        
                        Args.push_back(heapArrayPtr);
                    } else {
                        // 1D array - use single index [0] for array decay
                        std::vector<llvm::Value*> indices = {
                            llvm::ConstantInt::get(Builder.getInt32Ty(), 0)
                        };
                        llvm::Value* arrayDecayPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
                        Args.push_back(arrayDecayPtr);
                    }
                } else if (allocatedType->isPointerTy()) {
                    if (allocatedType == expectedParamType) {
                        llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
                        Args.push_back(loadedPtr);
                    } else {
                        llvm::Type* pointedType = allocatedType;
                        if (pointedType->isPointerTy() && expectedParamType->isPointerTy()) {
                            llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
                            Args.push_back(loadedPtr);
                        } else {
                            Write("Function Call", "Type mismatch for array argument: " + IdentNode->name + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                            return nullptr;
                        }
                    }
                } else {
                    Write("Function Call", "Argument is not an array type: " + IdentNode->name + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                    return nullptr;
                }
            } else {
                llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[i], Builder, SymbolStack, Methods);
                if (!ArgValue) {
                    Write("Function Call", "Invalid argument expression for function: " + FuncCallNode->name + Location, 2, true, true, "");
                    return nullptr;
                }
                Args.push_back(ArgValue);
            }
        } else {
            llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[i], Builder, SymbolStack, Methods);
            if (!ArgValue) {
                Write("Function Call", "Invalid argument expression for function: " + FuncCallNode->name + Location, 2, true, true, "");
                return nullptr;
            }
            
            if (i < FuncType->getNumParams()) {
                llvm::Type* ExpectedType = FuncType->getParamType(i);
                if (ArgValue->getType() != ExpectedType) {
                    if (ExpectedType->isIntegerTy(32) && ArgValue->getType()->isFloatingPointTy()) {
                        ArgValue = Builder.CreateFPToSI(ArgValue, ExpectedType);
                    } else if (ExpectedType->isFloatingPointTy() && ArgValue->getType()->isIntegerTy()) {
                        ArgValue = Builder.CreateSIToFP(ArgValue, ExpectedType);
                    } else if (ExpectedType->isFloatTy() && ArgValue->getType()->isDoubleTy()) {
                        ArgValue = Builder.CreateFPTrunc(ArgValue, ExpectedType);
                    } else if (ExpectedType->isDoubleTy() && ArgValue->getType()->isFloatTy()) {
                        ArgValue = Builder.CreateFPExt(ArgValue, ExpectedType);
                    } else {
                        Write("Function Call", "Type mismatch for argument " + std::to_string(i) + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                        return nullptr;
                    }
                }
            }
            Args.push_back(ArgValue);
        }
    }
    
    return Builder.CreateCall(Function, Args);
}