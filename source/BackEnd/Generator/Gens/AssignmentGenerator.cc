#include "AssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateAssignment(AssignmentOpNode* Assign, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    std::string StmtLocation = " at line " + std::to_string(Assign->token.line) + ", column " + std::to_string(Assign->token.column);
    if (!Assign) {
        Write("Block Generator", "Failed to cast to AssignmentOpNode" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Value* rvalue = GenerateExpression(Assign->right, Builder, SymbolStack, Methods);
    if (!rvalue) {
        Write("Block Generator", "Invalid assignment value expression" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    if (Assign->left->type == NodeType::ArrayAccess) {
        auto* ArrayAccess = static_cast<ArrayAccessNode*>(Assign->left.get());
        
        llvm::Value* arrayPtr = nullptr;
        for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
            auto found = it->find(ArrayAccess->identifier);
            if (found != it->end()) {
                arrayPtr = found->second;
                break;
            }
        }
        
        if (!arrayPtr) {
            Write("Block Generator", "Undefined array identifier: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
        if (!allocaInst) {
            Write("Block Generator", "Array identifier is not an allocated variable: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* allocatedType = allocaInst->getAllocatedType();
        
        if (allocatedType->isPointerTy()) {
            llvm::Value* heapArrayPtr = Builder.CreateLoad(allocatedType, allocaInst);
            llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, Builder, SymbolStack, Methods);
            
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in heap array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            llvm::Type* elementType = allocatedType;
            if (rvalue->getType() != elementType) {
                if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                    rvalue = Builder.CreateFPToSI(rvalue, elementType);
                } else if (elementType->isFloatTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = Builder.CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isDoubleTy()) {
                        rvalue = Builder.CreateFPTrunc(rvalue, elementType);
                    }
                } else if (elementType->isDoubleTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = Builder.CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isFloatTy()) {
                        rvalue = Builder.CreateFPExt(rvalue, elementType);
                    }
                } else {
                    Write("Block Generator", "Type mismatch in heap array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(elementType, heapArrayPtr, indexValue);
            Builder.CreateStore(rvalue, elementPtr);
            return rvalue;
        }
        
        if (ArrayAccess->expr->type == NodeType::Array) {
            auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAccess->expr.get());
            std::vector<llvm::Value*> indices;
            indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
            
            llvm::Type* currentType = allocatedType;
            for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
                if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                    Write("Block Generator", "Invalid array index in assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                if (!currentType->isArrayTy()) {
                    Write("Block Generator", "Too many dimensions in array assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                if (!arrayType) {
                    Write("Block Generator", "Expected array type in assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                    uint64_t index = constIndex->getZExtValue();
                    if (index >= arraySize) {
                        Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                }
                
                indices.push_back(indexValue);
                currentType = arrayType->getElementType();
            }
            
            if (rvalue->getType() != currentType) {
                if (currentType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                    rvalue = Builder.CreateFPToSI(rvalue, currentType);
                } else if (currentType->isFloatTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = Builder.CreateSIToFP(rvalue, currentType);
                    } else if (rvalue->getType()->isDoubleTy()) {
                        rvalue = Builder.CreateFPTrunc(rvalue, currentType);
                    }
                } else if (currentType->isDoubleTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = Builder.CreateSIToFP(rvalue, currentType);
                    } else if (rvalue->getType()->isFloatTy()) {
                        rvalue = Builder.CreateFPExt(rvalue, currentType);
                    }
                } else {
                    Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            Builder.CreateStore(rvalue, elementPtr);
            return rvalue;
        } else {
            llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, Builder, SymbolStack, Methods);
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            if (!allocatedType->isArrayTy()) {
                Write("Block Generator", "Variable is not an array in assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
            if (!arrayType) {
                Write("Block Generator", "Expected array type in assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            uint64_t arraySize = arrayType->getNumElements();
            if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                uint64_t index = constIndex->getZExtValue();
                if (index >= arraySize) {
                    Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            llvm::Type* elementType = arrayType->getElementType();
            if (rvalue->getType() != elementType) {
                if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                    rvalue = Builder.CreateFPToSI(rvalue, elementType);
                } else if (elementType->isFloatTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = Builder.CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isDoubleTy()) {
                        rvalue = Builder.CreateFPTrunc(rvalue, elementType);
                    }
                } else if (elementType->isDoubleTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = Builder.CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isFloatTy()) {
                        rvalue = Builder.CreateFPExt(rvalue, elementType);
                    }
                } else {
                    Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                indexValue
            };
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            Builder.CreateStore(rvalue, elementPtr);
            return rvalue;
        }
    } else if (Assign->left->type == NodeType::Identifier) {
        auto* Ident = static_cast<IdentifierNode*>(Assign->left.get());
        
        llvm::Value* varPtr = nullptr;
        for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
            auto found = it->find(Ident->name);
            if (found != it->end()) {
                varPtr = found->second;
                break;
            }
        }
        
        if (!varPtr) {
            Write("Block Generator", "Undefined variable: " + Ident->name + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(varPtr);
        if (!allocaInst) {
            Write("Block Generator", "Variable is not modifiable: " + Ident->name + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* varType = allocaInst->getAllocatedType();
        if (rvalue->getType() != varType) {
            if (varType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                rvalue = Builder.CreateFPToSI(rvalue, varType);
            } else if (varType->isFloatTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = Builder.CreateSIToFP(rvalue, varType);
                } else if (rvalue->getType()->isDoubleTy()) {
                    rvalue = Builder.CreateFPTrunc(rvalue, varType);
                }
            } else if (varType->isDoubleTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = Builder.CreateSIToFP(rvalue, varType);
                } else if (rvalue->getType()->isFloatTy()) {
                    rvalue = Builder.CreateFPExt(rvalue, varType);
                }
            } else {
                Write("Block Generator", "Type mismatch in variable assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
        
        Builder.CreateStore(rvalue, allocaInst);
        return rvalue;
    } else {
        Write("Block Generator", "Invalid assignment target" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
}