#include "ArrayAssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateArrayAssignment(ArrayAssignmentNode* ArrayAssign, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    std::string StmtLocation = " at line " + std::to_string(ArrayAssign->token.line) + ", column " + std::to_string(ArrayAssign->token.column);
    
    llvm::Value* rvalue = GenerateExpression(ArrayAssign->value, Builder, SymbolStack, Methods);
    if (!rvalue) {
        Write("Block Generator", "Invalid assignment value expression for array assignment" + StmtLocation, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* arrayPtr = nullptr;
    for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
        auto found = it->find(ArrayAssign->identifier);
        if (found != it->end()) {
            arrayPtr = found->second;
            break;
        }
    }
    
    if (!arrayPtr) {
        Write("Block Generator", "Undefined array identifier: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
    if (!allocaInst) {
        Write("Block Generator", "Array identifier is not an allocated variable: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Type* allocatedType = allocaInst->getAllocatedType();
    
    if (allocatedType->isPointerTy()) {
        llvm::Value* heapArrayPtr = Builder.CreateLoad(allocatedType, allocaInst);
        llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, Builder, SymbolStack, Methods);
        
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
    
    if (ArrayAssign->indexExpr->type == NodeType::Array) {
        auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAssign->indexExpr.get());
        std::vector<llvm::Value*> indices;
        indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
        
        llvm::Type* currentType = allocatedType;
        for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
            llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            if (!currentType->isArrayTy()) {
                Write("Block Generator", "Too many dimensions in array assignment: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
            if (!arrayType) {
                Write("Block Generator", "Expected array type in array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            uint64_t arraySize = arrayType->getNumElements();
            if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                uint64_t index = constIndex->getZExtValue();
                if (index >= arraySize) {
                    Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in array assignment" + StmtLocation, 2, true, true, "");
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
        llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, Builder, SymbolStack, Methods);
        if (!indexValue || !indexValue->getType()->isIntegerTy()) {
            Write("Block Generator", "Invalid array index in array assignment" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        if (!allocatedType->isArrayTy()) {
            Write("Block Generator", "Variable is not an array in array assignment: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
        if (!arrayType) {
            Write("Block Generator", "Expected array type in array assignment" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        uint64_t arraySize = arrayType->getNumElements();
        if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
            uint64_t index = constIndex->getZExtValue();
            if (index >= arraySize) {
                Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in array assignment" + StmtLocation, 2, true, true, "");
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
}