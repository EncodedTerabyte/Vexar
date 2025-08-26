#include "UnaryAssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateUnaryAssignment(UnaryOpNode* UnaryOp, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    std::string StmtLocation = " at line " + std::to_string(UnaryOp->token.line) + ", column " + std::to_string(UnaryOp->token.column);
    
    if (!UnaryOp) {
        Write("Block Generator", "Failed to cast to UnaryOpNode" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    if (UnaryOp->op != "++" && UnaryOp->op != "--") {
        Write("Block Generator", "Unsupported unary assignment operator: " + UnaryOp->op + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    if (UnaryOp->operand->type == NodeType::ArrayAccess) {
        auto* ArrayAccess = static_cast<ArrayAccessNode*>(UnaryOp->operand.get());
        
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
                Write("Block Generator", "Invalid array index in heap array unary assignment" + StmtLocation, 2, true, true, "");
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
                        Write("Block Generator", "Invalid array index in unary assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                    
                    if (!currentType->isArrayTy()) {
                        Write("Block Generator", "Too many dimensions in array unary assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                    
                    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                    if (!arrayType) {
                        Write("Block Generator", "Expected array type in unary assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                    
                    uint64_t arraySize = arrayType->getNumElements();
                    if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                        uint64_t index = constIndex->getZExtValue();
                        if (index >= arraySize) {
                            Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in unary assignment" + StmtLocation, 2, true, true, "");
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
                    Write("Block Generator", "Invalid array index in unary assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                if (!allocatedType->isArrayTy()) {
                    Write("Block Generator", "Variable is not an array in unary assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                if (!arrayType) {
                    Write("Block Generator", "Expected array type in unary assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                    uint64_t index = constIndex->getZExtValue();
                    if (index >= arraySize) {
                        Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in unary assignment" + StmtLocation, 2, true, true, "");
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
        
        llvm::Value* one;
        if (elementType->isFloatingPointTy()) {
            one = llvm::ConstantFP::get(elementType, 1.0);
        } else {
            one = llvm::ConstantInt::get(elementType, 1);
        }
        
        llvm::Value* result;
        if (UnaryOp->op == "++") {
            if (elementType->isFloatingPointTy()) {
                result = Builder.CreateFAdd(currentValue, one);
            } else {
                result = Builder.CreateAdd(currentValue, one);
            }
        } else {
            if (elementType->isFloatingPointTy()) {
                result = Builder.CreateFSub(currentValue, one);
            } else {
                result = Builder.CreateSub(currentValue, one);
            }
        }
        
        Builder.CreateStore(result, elementPtr);
        return result;
        
    } else if (UnaryOp->operand->type == NodeType::Identifier) {
        auto* Ident = static_cast<IdentifierNode*>(UnaryOp->operand.get());
        
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
        
        llvm::Value* one;
        if (varType->isFloatingPointTy()) {
            one = llvm::ConstantFP::get(varType, 1.0);
        } else {
            one = llvm::ConstantInt::get(varType, 1);
        }
        
        llvm::Value* result;
        if (UnaryOp->op == "++") {
            if (varType->isFloatingPointTy()) {
                result = Builder.CreateFAdd(currentValue, one);
            } else {
                result = Builder.CreateAdd(currentValue, one);
            }
        } else {
            if (varType->isFloatingPointTy()) {
                result = Builder.CreateFSub(currentValue, one);
            } else {
                result = Builder.CreateSub(currentValue, one);
            }
        }
        
        Builder.CreateStore(result, allocaInst);
        return result;
        
    } else {
        Write("Block Generator", "Invalid unary assignment target" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
}