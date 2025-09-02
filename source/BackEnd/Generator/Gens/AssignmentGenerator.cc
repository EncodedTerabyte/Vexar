#include "AssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateAssignment(AssignmentOpNode* Assign, AeroIR* IR, FunctionSymbols& Methods) {
    std::string StmtLocation = " at line " + std::to_string(Assign->token.line) + ", column " + std::to_string(Assign->token.column);
    if (!Assign) {
        Write("Block Generator", "Failed to cast to AssignmentOpNode" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Value* rvalue = GenerateExpression(Assign->right, IR, Methods);
    if (!rvalue) {
        Write("Block Generator", "Invalid assignment value expression" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    if (Assign->left->type == NodeType::ArrayAccess) {
        auto* ArrayAccess = static_cast<ArrayAccessNode*>(Assign->left.get());
        
        llvm::Value* arrayPtr = IR->getVar(ArrayAccess->identifier);
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
            llvm::Value* heapArrayPtr = IR->load(allocaInst);
            llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, IR, Methods);
            
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in heap array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            llvm::Type* elementType = allocatedType;
            if (rvalue->getType() != elementType) {
                if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                    rvalue = IR->getBuilder()->CreateFPToSI(rvalue, elementType);
                } else if (elementType->isIntegerTy(1) && rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->getBuilder()->CreateTrunc(rvalue, elementType);
                } else if (elementType->isIntegerTy() && rvalue->getType()->isIntegerTy(1)) {
                    rvalue = IR->getBuilder()->CreateZExt(rvalue, elementType);
                } else if (elementType->isFloatTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = IR->getBuilder()->CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isDoubleTy()) {
                        rvalue = IR->floatCast(rvalue, elementType);
                    }
                } else if (elementType->isDoubleTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = IR->getBuilder()->CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isFloatTy()) {
                        rvalue = IR->floatCast(rvalue, elementType);
                    }
                } else {
                    Write("Block Generator", "Type mismatch in heap array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            llvm::Value* elementPtr = IR->arrayAccess(heapArrayPtr, indexValue);
            IR->store(rvalue, elementPtr);
            return rvalue;
        }
        
        if (ArrayAccess->expr->type == NodeType::Array) {
            auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAccess->expr.get());
            std::vector<llvm::Value*> indices;
            indices.push_back(IR->constI32(0));
            
            llvm::Type* currentType = allocatedType;
            for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], IR, Methods);
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
                    rvalue = IR->getBuilder()->CreateFPToSI(rvalue, currentType);
                } else if (currentType->isIntegerTy(1) && rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->getBuilder()->CreateTrunc(rvalue, currentType);
                } else if (currentType->isIntegerTy() && rvalue->getType()->isIntegerTy(1)) {
                    rvalue = IR->getBuilder()->CreateZExt(rvalue, currentType);
                } else if (currentType->isFloatTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = IR->getBuilder()->CreateSIToFP(rvalue, currentType);
                    } else if (rvalue->getType()->isDoubleTy()) {
                        rvalue = IR->floatCast(rvalue, currentType);
                    }
                } else if (currentType->isDoubleTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = IR->getBuilder()->CreateSIToFP(rvalue, currentType);
                    } else if (rvalue->getType()->isFloatTy()) {
                        rvalue = IR->floatCast(rvalue, currentType);
                    }
                } else {
                    Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            llvm::Value* elementPtr = IR->getBuilder()->CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            IR->store(rvalue, elementPtr);
            return rvalue;
        } else {
            llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, IR, Methods);
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
                    rvalue = IR->getBuilder()->CreateFPToSI(rvalue, elementType);
                } else if (elementType->isIntegerTy(1) && rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->getBuilder()->CreateTrunc(rvalue, elementType);
                } else if (elementType->isIntegerTy() && rvalue->getType()->isIntegerTy(1)) {
                    rvalue = IR->getBuilder()->CreateZExt(rvalue, elementType);
                } else if (elementType->isFloatTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = IR->getBuilder()->CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isDoubleTy()) {
                        rvalue = IR->floatCast(rvalue, elementType);
                    }
                } else if (elementType->isDoubleTy()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        rvalue = IR->getBuilder()->CreateSIToFP(rvalue, elementType);
                    } else if (rvalue->getType()->isFloatTy()) {
                        rvalue = IR->floatCast(rvalue, elementType);
                    }
                } else {
                    Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
            }
            
            llvm::Value* elementPtr = IR->arrayAccess(arrayPtr, indexValue);
            IR->store(rvalue, elementPtr);
            return rvalue;
        }
    } else if (Assign->left->type == NodeType::Identifier) {
        auto* Ident = static_cast<IdentifierNode*>(Assign->left.get());
        
        llvm::Value* varPtr = IR->getVar(Ident->name);
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
                rvalue = IR->getBuilder()->CreateFPToSI(rvalue, varType);
            } else if (varType->isIntegerTy(1) && rvalue->getType()->isIntegerTy()) {
                rvalue = IR->getBuilder()->CreateTrunc(rvalue, varType);
            } else if (varType->isIntegerTy() && rvalue->getType()->isIntegerTy(1)) {
                rvalue = IR->getBuilder()->CreateZExt(rvalue, varType);
            } else if (varType->isFloatTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->getBuilder()->CreateSIToFP(rvalue, varType);
                } else if (rvalue->getType()->isDoubleTy()) {
                    rvalue = IR->floatCast(rvalue, varType);
                }
            } else if (varType->isDoubleTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->getBuilder()->CreateSIToFP(rvalue, varType);
                } else if (rvalue->getType()->isFloatTy()) {
                    rvalue = IR->floatCast(rvalue, varType);
                }
            } else {
                Write("Block Generator", "Type mismatch in variable assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
        
        IR->store(rvalue, allocaInst);
        return rvalue;
    } else {
        Write("Block Generator", "Invalid assignment target" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
}