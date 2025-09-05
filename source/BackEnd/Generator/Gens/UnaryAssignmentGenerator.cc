#include "UnaryAssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateUnaryAssignment(UnaryOpNode* UnaryOp, AeroIR* IR, FunctionSymbols& Methods) {
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
        llvm::Value* elementPtr = nullptr;
        llvm::Type* elementType = nullptr;
        
        if (allocatedType->isPointerTy()) {
            llvm::Value* heapArrayPtr = IR->load(allocaInst);
            llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, IR, Methods);
            
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in heap array unary assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            elementType = allocatedType;
            elementPtr = IR->arrayAccess(heapArrayPtr, indexValue);
        } else {
            if (ArrayAccess->expr->type == NodeType::Array) {
                auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAccess->expr.get());
                std::vector<llvm::Value*> indices;
                indices.push_back(IR->constI32(0));
                
                llvm::Type* currentType = allocatedType;
                for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                    llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], IR, Methods);
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
                elementPtr = IR->getBuilder()->CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            } else {
                llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, IR, Methods);
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
                    IR->constI32(0),
                    indexValue
                };
                
                elementPtr = IR->getBuilder()->CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            }
        }
        
        llvm::Value* currentValue = IR->load(elementPtr);
        
        llvm::Value* one;
        if (elementType->isFloatingPointTy()) {
            one = llvm::ConstantFP::get(elementType, 1.0);
        } else {
            one = llvm::ConstantInt::get(elementType, 1);
        }
        
        llvm::Value* result;
        if (UnaryOp->op == "++") {
            result = IR->add(currentValue, one);
        } else {
            result = IR->sub(currentValue, one);
        }
        
        IR->store(result, elementPtr);
        return result;
        
    } else if (UnaryOp->operand->type == NodeType::Identifier) {
        auto* Ident = static_cast<IdentifierNode*>(UnaryOp->operand.get());
        
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
        llvm::Value* currentValue = IR->load(allocaInst);
        
        llvm::Value* one;
        if (varType->isFloatingPointTy()) {
            one = llvm::ConstantFP::get(varType, 1.0);
        } else {
            one = llvm::ConstantInt::get(varType, 1);
        }
        
        llvm::Value* result;
        if (UnaryOp->op == "++") {
            result = IR->add(currentValue, one);
        } else {
            result = IR->sub(currentValue, one);
        }
        
        IR->store(result, allocaInst);
        return result;
        
    } else {
        Write("Block Generator", "Invalid unary assignment target" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
}