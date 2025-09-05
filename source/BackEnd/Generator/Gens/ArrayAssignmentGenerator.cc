#include "ArrayAssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateArrayAssignment(ArrayAssignmentNode* ArrayAssign, AeroIR* IR, FunctionSymbols& Methods) {
    std::string StmtLocation = " at line " + std::to_string(ArrayAssign->token.line) + ", column " + std::to_string(ArrayAssign->token.column);
    
    llvm::Value* rvalue = GenerateExpression(ArrayAssign->value, IR, Methods);
    if (!rvalue) {
        Write("Block Generator", "Invalid assignment value expression for array assignment" + StmtLocation, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* arrayPtr = IR->getVar(ArrayAssign->identifier);
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
<<<<<<< HEAD
        llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
        
        if (ArrayAssign->indexExpr->type == NodeType::Array) {
            auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAssign->indexExpr.get());
            
            if (IndexArrayPtr->elements.size() == 1) {
                llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[0], Builder, SymbolStack, Methods);
                if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                    Write("Block Generator", "Invalid array index in heap array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::Type* elementType = Builder.getInt32Ty();
                if (rvalue->getType() != elementType) {
                    if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                        rvalue = Builder.CreateFPToSI(rvalue, elementType);
                    }
                }
                
                llvm::Value* elementPtr = Builder.CreateInBoundsGEP(elementType, loadedPtr, indexValue);
                Builder.CreateStore(rvalue, elementPtr);
                return rvalue;
                
            } else if (IndexArrayPtr->elements.size() == 2) {
                llvm::Value* rowIndex = GenerateExpression(IndexArrayPtr->elements[0], Builder, SymbolStack, Methods);
                llvm::Value* colIndex = GenerateExpression(IndexArrayPtr->elements[1], Builder, SymbolStack, Methods);
                
                if (!rowIndex || !rowIndex->getType()->isIntegerTy() || 
                    !colIndex || !colIndex->getType()->isIntegerTy()) {
                    Write("Block Generator", "Invalid array indices in 2D heap array assignment" + StmtLocation, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::Value* numCols = llvm::ConstantInt::get(Builder.getInt32Ty(), 2);
                llvm::Value* rowOffset = Builder.CreateMul(rowIndex, numCols);
                llvm::Value* flatIndex = Builder.CreateAdd(rowOffset, colIndex);
                
                llvm::Type* elementType = Builder.getInt32Ty();
                if (rvalue->getType() != elementType) {
                    if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                        rvalue = Builder.CreateFPToSI(rvalue, elementType);
                    }
                }
                
                llvm::Value* elementPtr = Builder.CreateInBoundsGEP(elementType, loadedPtr, flatIndex);
                Builder.CreateStore(rvalue, elementPtr);
                return rvalue;
=======
        llvm::Value* loadedPtr = IR->load(allocaInst);
        llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, IR, Methods);
        
        if (!indexValue || !indexValue->getType()->isIntegerTy()) {
            Write("Block Generator", "Invalid array index in heap array assignment" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Function* strlenFunc = IR->getRegisteredBuiltin("strlen");
        if (!strlenFunc) {
            IR->registerBuiltin("strlen", IR->i64(), {IR->i8ptr()});
            strlenFunc = IR->getRegisteredBuiltin("strlen");
        }
        
        llvm::Function* exitFunc = IR->getRegisteredBuiltin("exit");
        if (!exitFunc) {
            IR->registerBuiltin("exit", IR->void_t(), {IR->i32()});
            exitFunc = IR->getRegisteredBuiltin("exit");
        }
        
        llvm::Function* printfFunc = IR->getRegisteredBuiltin("printf");
        if (!printfFunc) {
            IR->registerBuiltin("printf", IR->i32(), {IR->i8ptr()});
            printfFunc = IR->getRegisteredBuiltin("printf");
        }
        
        llvm::Value* strLength = IR->call(strlenFunc, {loadedPtr});
        llvm::Value* strLengthTrunc = IR->intCast(strLength, IR->i32());
        llvm::Value* indexTrunc = indexValue;
        if (indexValue->getType() != IR->i32()) {
            indexTrunc = IR->intCast(indexValue, IR->i32());
        }
        
        llvm::Value* isOutOfBounds = IR->ge(indexTrunc, strLengthTrunc);
        
        llvm::BasicBlock* errorBB = IR->createBlock("bounds_error");
        llvm::BasicBlock* validBB = IR->createBlock("valid_access");
        
        IR->condBranch(isOutOfBounds, errorBB, validBB);
        
        IR->setInsertPoint(errorBB);
        llvm::Value* errorMsg = IR->constString("Segmentation fault: string index out of bounds\n");
        IR->call(printfFunc, {errorMsg});
        IR->call(exitFunc, {IR->constI32(139)});
        
        IR->setInsertPoint(validBB);
        
        llvm::Value* charValue = rvalue;
        if (rvalue->getType() != IR->i8()) {
            if (rvalue->getType()->isIntegerTy()) {
                charValue = IR->intCast(rvalue, IR->i8());
>>>>>>> main
            } else {
                Write("Block Generator", "Unsupported number of dimensions in heap array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        } else {
            llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, Builder, SymbolStack, Methods);
            
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Block Generator", "Invalid array index in heap array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
            
            if (allocatedType->isIntegerTy(8)) {
                llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
                if (!strlenFunc) {
                    llvm::FunctionType* strlenType = llvm::FunctionType::get(
                        llvm::Type::getInt64Ty(Builder.getContext()),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        false
                    );
                    strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
                }
                
                llvm::Function* exitFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("exit");
                if (!exitFunc) {
                    llvm::FunctionType* exitType = llvm::FunctionType::get(
                        llvm::Type::getVoidTy(Builder.getContext()),
                        {llvm::Type::getInt32Ty(Builder.getContext())},
                        false
                    );
                    exitFunc = llvm::Function::Create(exitType, llvm::Function::ExternalLinkage, "exit", Builder.GetInsertBlock()->getParent()->getParent());
                }
                
                llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
                if (!printfFunc) {
                    llvm::FunctionType* printfType = llvm::FunctionType::get(
                        llvm::Type::getInt32Ty(Builder.getContext()),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        true
                    );
                    printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Builder.GetInsertBlock()->getParent()->getParent());
                }
                
                llvm::Value* strLength = Builder.CreateCall(strlenFunc, {loadedPtr});
                llvm::Value* strLengthTrunc = Builder.CreateTrunc(strLength, Builder.getInt32Ty());
                llvm::Value* indexTrunc = indexValue;
                if (indexValue->getType() != Builder.getInt32Ty()) {
                    indexTrunc = Builder.CreateTrunc(indexValue, Builder.getInt32Ty());
                }
                
                llvm::Value* isOutOfBounds = Builder.CreateICmpUGE(indexTrunc, strLengthTrunc);
                
                llvm::BasicBlock* currentBB = Builder.GetInsertBlock();
                llvm::Function* function = currentBB->getParent();
                llvm::BasicBlock* errorBB = llvm::BasicBlock::Create(Builder.getContext(), "bounds_error", function);
                llvm::BasicBlock* validBB = llvm::BasicBlock::Create(Builder.getContext(), "valid_access", function);
                
                Builder.CreateCondBr(isOutOfBounds, errorBB, validBB);
                
                Builder.SetInsertPoint(errorBB);
                llvm::Value* errorMsg = Builder.CreateGlobalString("Segmentation fault: string index out of bounds\n", "seg_fault_msg");
                llvm::Value* errorMsgPtr = Builder.CreatePointerCast(errorMsg, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
                Builder.CreateCall(printfFunc, {errorMsgPtr});
                Builder.CreateCall(exitFunc, {llvm::ConstantInt::get(Builder.getInt32Ty(), 139)});
                Builder.CreateUnreachable();
                
                Builder.SetInsertPoint(validBB);
                
                llvm::Value* charValue = rvalue;
                if (rvalue->getType() != Builder.getInt8Ty()) {
                    if (rvalue->getType()->isIntegerTy()) {
                        charValue = Builder.CreateTrunc(rvalue, Builder.getInt8Ty());
                    } else {
                        Write("Block Generator", "Type mismatch in string assignment" + StmtLocation, 2, true, true, "");
                        return nullptr;
                    }
                }
                
                llvm::Value* elementPtr = Builder.CreateInBoundsGEP(Builder.getInt8Ty(), loadedPtr, indexValue);
                Builder.CreateStore(charValue, elementPtr);
                return rvalue;
            } else {
                llvm::Type* elementType = Builder.getInt32Ty();
                if (rvalue->getType() != elementType) {
                    if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                        rvalue = Builder.CreateFPToSI(rvalue, elementType);
                    }
                }
                
                llvm::Value* elementPtr = Builder.CreateInBoundsGEP(elementType, loadedPtr, indexValue);
                Builder.CreateStore(rvalue, elementPtr);
                return rvalue;
            }
        }
<<<<<<< HEAD
=======
        
        llvm::Value* elementPtr = IR->arrayAccess(loadedPtr, indexValue);
        IR->store(charValue, elementPtr);
        return rvalue;
>>>>>>> main
    }
    
    if (ArrayAssign->indexExpr->type == NodeType::Array) {
        ArrayNode* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAssign->indexExpr.get());
        if (!IndexArrayPtr) {
            Write("Block Generator", "Failed to cast to ArrayNode" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        std::vector<llvm::Value*> indices;
        indices.push_back(IR->constI32(0));
        
        llvm::Type* currentType = allocatedType;
        for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
            llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], IR, Methods);
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
            
            llvm::Function* printfFunc = IR->getRegisteredBuiltin("printf");
            if (!printfFunc) {
                IR->registerBuiltin("printf", IR->i32(), {IR->i8ptr()});
                printfFunc = IR->getRegisteredBuiltin("printf");
            }
            
            llvm::Function* exitFunc = IR->getRegisteredBuiltin("exit");
            if (!exitFunc) {
                IR->registerBuiltin("exit", IR->void_t(), {IR->i32()});
                exitFunc = IR->getRegisteredBuiltin("exit");
            }
            
            llvm::Value* arraySizeValue = IR->constI32(arraySize);
            llvm::Value* indexTrunc = indexValue;
            if (indexValue->getType() != IR->i32()) {
                indexTrunc = IR->intCast(indexValue, IR->i32());
            }
            
            llvm::Value* isOutOfBounds = IR->ge(indexTrunc, arraySizeValue);
            
            llvm::BasicBlock* errorBB = IR->createBlock("array_bounds_error");
            llvm::BasicBlock* validBB = IR->createBlock("valid_array_access");
            
            IR->condBranch(isOutOfBounds, errorBB, validBB);
            
            IR->setInsertPoint(errorBB);
            llvm::Value* errorMsg = IR->constString("Segmentation fault: array index out of bounds\n");
            IR->call(printfFunc, {errorMsg});
            IR->call(exitFunc, {IR->constI32(139)});
            
            IR->setInsertPoint(validBB);
            
            indices.push_back(indexValue);
            currentType = arrayType->getElementType();
        }
        
        if (rvalue->getType() != currentType) {
            if (currentType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                rvalue = IR->cast(rvalue, currentType);
            } else if (currentType->isFloatTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->cast(rvalue, currentType);
                } else if (rvalue->getType()->isDoubleTy()) {
                    rvalue = IR->floatCast(rvalue, currentType);
                }
            } else if (currentType->isDoubleTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->cast(rvalue, currentType);
                } else if (rvalue->getType()->isFloatTy()) {
                    rvalue = IR->floatCast(rvalue, currentType);
                }
            } else {
                Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
        
        llvm::Value* elementPtr = IR->arrayAccess(arrayPtr, indices[1]);
        for (size_t i = 2; i < indices.size(); ++i) {
            elementPtr = IR->arrayAccess(elementPtr, indices[i]);
        }
        IR->store(rvalue, elementPtr);
        return rvalue;
        
    } else {
        llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, IR, Methods);
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
        
        llvm::Function* printfFunc = IR->getRegisteredBuiltin("printf");
        if (!printfFunc) {
            IR->registerBuiltin("printf", IR->i32(), {IR->i8ptr()});
            printfFunc = IR->getRegisteredBuiltin("printf");
        }
        
        llvm::Function* exitFunc = IR->getRegisteredBuiltin("exit");
        if (!exitFunc) {
            IR->registerBuiltin("exit", IR->void_t(), {IR->i32()});
            exitFunc = IR->getRegisteredBuiltin("exit");
        }
        
        llvm::Value* arraySizeValue = IR->constI32(arraySize);
        llvm::Value* indexTrunc = indexValue;
        if (indexValue->getType() != IR->i32()) {
            indexTrunc = IR->intCast(indexValue, IR->i32());
        }
        
        llvm::Value* isOutOfBounds = IR->ge(indexTrunc, arraySizeValue);
        
        llvm::BasicBlock* errorBB = IR->createBlock("array_bounds_error2");
        llvm::BasicBlock* validBB = IR->createBlock("valid_array_access2");
        
        IR->condBranch(isOutOfBounds, errorBB, validBB);
        
        IR->setInsertPoint(errorBB);
        llvm::Value* errorMsg = IR->constString("Segmentation fault: array index out of bounds\n");
        IR->call(printfFunc, {errorMsg});
        IR->call(exitFunc, {IR->constI32(139)});
        
        IR->setInsertPoint(validBB);
        
        llvm::Type* elementType = arrayType->getElementType();
        if (rvalue->getType() != elementType) {
            if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                rvalue = IR->cast(rvalue, elementType);
            } else if (elementType->isFloatTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->cast(rvalue, elementType);
                } else if (rvalue->getType()->isDoubleTy()) {
                    rvalue = IR->floatCast(rvalue, elementType);
                }
            } else if (elementType->isDoubleTy()) {
                if (rvalue->getType()->isIntegerTy()) {
                    rvalue = IR->cast(rvalue, elementType);
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
}