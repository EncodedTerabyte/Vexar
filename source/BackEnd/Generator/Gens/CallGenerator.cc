#include "CallGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCall(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods, BuiltinSymbols& BuiltIns) {
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
        llvm::Value* Result = BuiltinIt->second(FuncCallNode->arguments, IR, Methods);
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
                llvm::Value* arrayPtr = IR->getVar(IdentNode->name);
                
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
                    
                    if (arrayType->getElementType()->isArrayTy()) {
                        llvm::ArrayType* innerArrayType = llvm::dyn_cast<llvm::ArrayType>(arrayType->getElementType());
                        uint64_t outerSize = arrayType->getNumElements();
                        uint64_t innerSize = innerArrayType->getNumElements();
                        
                        llvm::Value* outerArraySize = IR->constI64(outerSize * 8);
                        llvm::Value* heapArrayPtr = IR->malloc(IR->ptr(IR->i32()), outerArraySize);
                        heapArrayPtr = IR->cast(heapArrayPtr, IR->ptr(IR->ptr(IR->i32())));
                        
                        for (uint64_t i = 0; i < outerSize; ++i) {
                            llvm::Value* innerArraySize = IR->constI64(innerSize);
                            llvm::Value* innerHeapPtr = IR->malloc(IR->i32(), innerArraySize);
                            
                            for (uint64_t j = 0; j < innerSize; ++j) {
                                llvm::Value* stackElementPtr = IR->arrayAccess(arrayPtr, IR->constI32(0));
                                stackElementPtr = IR->arrayAccess(stackElementPtr, IR->constI32(i));
                                stackElementPtr = IR->arrayAccess(stackElementPtr, IR->constI32(j));
                                llvm::Value* stackValue = IR->load(stackElementPtr);
                                
                                llvm::Value* heapElementPtr = IR->arrayAccess(innerHeapPtr, IR->constI32(j));
                                IR->store(stackValue, heapElementPtr);
                            }
                            
                            llvm::Value* outerElementPtr = IR->arrayAccess(heapArrayPtr, IR->constI32(i));
                            IR->store(innerHeapPtr, outerElementPtr);
                        }
                        
                        Args.push_back(heapArrayPtr);
                    } else {
                        llvm::Value* arrayDecayPtr = IR->arrayAccess(arrayPtr, IR->constI32(0));
                        Args.push_back(arrayDecayPtr);
                    }
                } else if (allocatedType->isPointerTy()) {
                    if (allocatedType == expectedParamType) {
                        llvm::Value* loadedPtr = IR->load(allocaInst);
                        Args.push_back(loadedPtr);
                    } else {
                        llvm::Type* pointedType = allocatedType;
                        if (pointedType->isPointerTy() && expectedParamType->isPointerTy()) {
                            llvm::Value* loadedPtr = IR->load(allocaInst);
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
                llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[i], IR, Methods);
                if (!ArgValue) {
                    Write("Function Call", "Invalid argument expression for function: " + FuncCallNode->name + Location, 2, true, true, "");
                    return nullptr;
                }
                Args.push_back(ArgValue);
            }
        } else {
            llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[i], IR, Methods);
            if (!ArgValue) {
                Write("Function Call", "Invalid argument expression for function: " + FuncCallNode->name + Location, 2, true, true, "");
                return nullptr;
            }
            
            if (i < FuncType->getNumParams()) {
                llvm::Type* ExpectedType = FuncType->getParamType(i);
                if (ArgValue->getType() != ExpectedType) {
                    if (ExpectedType->isIntegerTy(32) && ArgValue->getType()->isFloatingPointTy()) {
                        ArgValue = IR->cast(ArgValue, ExpectedType);
                    } else if (ExpectedType->isFloatingPointTy() && ArgValue->getType()->isIntegerTy()) {
                        ArgValue = IR->cast(ArgValue, ExpectedType);
                    } else if (ExpectedType->isFloatTy() && ArgValue->getType()->isDoubleTy()) {
                        ArgValue = IR->floatCast(ArgValue, ExpectedType);
                    } else if (ExpectedType->isDoubleTy() && ArgValue->getType()->isFloatTy()) {
                        ArgValue = IR->floatCast(ArgValue, ExpectedType);
                    } else {
                        Write("Function Call", "Type mismatch for argument " + std::to_string(i) + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                        return nullptr;
                    }
                }
            }
            Args.push_back(ArgValue);
        }
    }
    
    return IR->call(Function, Args);
}