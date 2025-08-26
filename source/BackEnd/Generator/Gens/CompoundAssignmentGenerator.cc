#include "CompoundAssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* performOperation(llvm::Value* left, llvm::Value* right, const std::string& op, llvm::IRBuilder<>& Builder, const std::string& location) {
    if (op == "+") {
        if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
            return Builder.CreateFAdd(left, right);
        } else {
            return Builder.CreateAdd(left, right);
        }
    } else if (op == "-") {
        if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
            return Builder.CreateFSub(left, right);
        } else {
            return Builder.CreateSub(left, right);
        }
    } else if (op == "*") {
        if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
            return Builder.CreateFMul(left, right);
        } else {
            return Builder.CreateMul(left, right);
        }
    } else if (op == "/") {
        if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
            return Builder.CreateFDiv(left, right);
        } else {
            return Builder.CreateSDiv(left, right);
        }
    } else if (op == "%") {
        if (left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy()) {
            return Builder.CreateFRem(left, right);
        } else {
            return Builder.CreateSRem(left, right);
        }
    }
    
    Write("Block Generator", "Unsupported compound assignment operator: " + op + location, 2, true, true, "");
    return nullptr;
}

llvm::Value* convertType(llvm::Value* value, llvm::Type* targetType, llvm::IRBuilder<>& Builder, const std::string& location) {
    if (value->getType() == targetType) {
        return value;
    }
    
    if (targetType->isIntegerTy(32) && value->getType()->isFloatingPointTy()) {
        return Builder.CreateFPToSI(value, targetType);
    } else if (targetType->isFloatTy()) {
        if (value->getType()->isIntegerTy()) {
            return Builder.CreateSIToFP(value, targetType);
        } else if (value->getType()->isDoubleTy()) {
            return Builder.CreateFPTrunc(value, targetType);
        }
    } else if (targetType->isDoubleTy()) {
        if (value->getType()->isIntegerTy()) {
            return Builder.CreateSIToFP(value, targetType);
        } else if (value->getType()->isFloatTy()) {
            return Builder.CreateFPExt(value, targetType);
        }
    }
    
    Write("Block Generator", "Type mismatch in compound assignment" + location, 2, true, true, "");
    return nullptr;
}

llvm::Value* GenerateCompoundAssignment(CompoundAssignmentOpNode* CompoundAssign, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    std::string StmtLocation = " at line " + std::to_string(CompoundAssign->token.line) + ", column " + std::to_string(CompoundAssign->token.column);
    
    if (!CompoundAssign) {
        Write("Block Generator", "Failed to cast to CompoundAssignmentOpNode" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Value* rvalue = GenerateExpression(CompoundAssign->right, Builder, SymbolStack, Methods);
    if (!rvalue) {
        Write("Block Generator", "Invalid compound assignment right-hand expression" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    std::string baseOp = CompoundAssign->op.substr(0, CompoundAssign->op.length() - 1);
    
    if (CompoundAssign->left->type == NodeType::ArrayAccess) {
        auto* ArrayAccess = static_cast<ArrayAccessNode*>(CompoundAssign->left.get());
        
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
        llvm::Value* elementPtr = nullptr;
        llvm::Type* elementType = nullptr;
        
        if (allocatedType->isPointerTy()) {
            llvm::Value* heapArrayPtr = Builder.CreateLoad(allocatedType, allocaInst);
            llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, Builder, SymbolStack, Methods);
            
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in heap array compound assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            elementType = allocatedType;
            elementPtr = Builder.CreateInBoundsGEP(elementType, heapArrayPtr, indexValue);
        } else {
            if (ArrayAccess->expr->type == NodeType::Array) {
                auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAccess->expr.get());
                std::vector<llvm::Value*> indices;
                indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
                
                llvm::Type* currentType = allocatedType;
                for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                    llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
                    if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                        Write("Block Generator", "Invalid array index in compound assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                    
                    if (!currentType->isArrayTy()) {
                        Write("Block Generator", "Too many dimensions in array compound assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                    
                    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                    if (!arrayType) {
                        Write("Block Generator", "Expected array type in compound assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                    
                    uint64_t arraySize = arrayType->getNumElements();
                    if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                        uint64_t index = constIndex->getZExtValue();
                        if (index >= arraySize) {
                            Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in compound assignment" + StmtLocation, 2, true, true, "");
                            return nullptr;
                        }
                    }
                    
                    indices.push_back(indexValue);
                    currentType = arrayType->getElementType();
                }
                
                elementType = currentType;
                elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            } else {
                llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, Builder, SymbolStack, Methods);
                if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                    Write("Block Generator", "Invalid array index in compound assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                if (!allocatedType->isArrayTy()) {
                    Write("Block Generator", "Variable is not an array in compound assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                if (!arrayType) {
                    Write("Block Generator", "Expected array type in compound assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                    uint64_t index = constIndex->getZExtValue();
                    if (index >= arraySize) {
                        Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in compound assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                }
                
                elementType = arrayType->getElementType();
                std::vector<llvm::Value*> indices = {
                    llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                    indexValue
                };
                
                elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            }
        }
        
        llvm::Value* currentValue = Builder.CreateLoad(elementType, elementPtr);
        
        llvm::Value* convertedRValue = convertType(rvalue, currentValue->getType(), Builder, StmtLocation);
        if (!convertedRValue) {
            return nullptr;
        }
        
        llvm::Value* result = performOperation(currentValue, convertedRValue, baseOp, Builder, StmtLocation);
        if (!result) {
            return nullptr;
        }
        
        llvm::Value* finalResult = convertType(result, elementType, Builder, StmtLocation);
        if (!finalResult) {
            return nullptr;
        }
        
        Builder.CreateStore(finalResult, elementPtr);
        return finalResult;
        
    } else if (CompoundAssign->left->type == NodeType::Identifier) {
        auto* Ident = static_cast<IdentifierNode*>(CompoundAssign->left.get());
        
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
        llvm::Value* currentValue = Builder.CreateLoad(varType, allocaInst);
        
        llvm::Value* convertedRValue = convertType(rvalue, currentValue->getType(), Builder, StmtLocation);
        if (!convertedRValue) {
            return nullptr;
        }
        
        llvm::Value* result = performOperation(currentValue, convertedRValue, baseOp, Builder, StmtLocation);
        if (!result) {
            return nullptr;
        }
        
        llvm::Value* finalResult = convertType(result, varType, Builder, StmtLocation);
        if (!finalResult) {
            return nullptr;
        }
        
        Builder.CreateStore(finalResult, allocaInst);
        return finalResult;
        
    } else {
        Write("Block Generator", "Invalid compound assignment target" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
}