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
        llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
        llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, Builder, SymbolStack, Methods);
        
        if (!indexValue || !indexValue->getType()->isIntegerTy()) {
            Write("Block Generator", "Invalid array index in heap array assignment" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
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
    }
    
    if (ArrayAssign->indexExpr->type == NodeType::Array) {
        ArrayNode* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAssign->indexExpr.get());
        if (!IndexArrayPtr) {
            Write("Block Generator", "Failed to cast to ArrayNode" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
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
            
            llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
            if (!printfFunc) {
                llvm::FunctionType* printfType = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(Builder.getContext()),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    true
                );
                printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Builder.GetInsertBlock()->getParent()->getParent());
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
            
            llvm::Value* arraySizeValue = llvm::ConstantInt::get(Builder.getInt32Ty(), arraySize);
            llvm::Value* indexTrunc = indexValue;
            if (indexValue->getType() != Builder.getInt32Ty()) {
                indexTrunc = Builder.CreateTrunc(indexValue, Builder.getInt32Ty());
            }
            
            llvm::Value* isOutOfBounds = Builder.CreateICmpUGE(indexTrunc, arraySizeValue);
            
            llvm::BasicBlock* currentBB = Builder.GetInsertBlock();
            llvm::Function* function = currentBB->getParent();
            llvm::BasicBlock* errorBB = llvm::BasicBlock::Create(Builder.getContext(), "array_bounds_error", function);
            llvm::BasicBlock* validBB = llvm::BasicBlock::Create(Builder.getContext(), "valid_array_access", function);
            
            Builder.CreateCondBr(isOutOfBounds, errorBB, validBB);
            
            Builder.SetInsertPoint(errorBB);
            llvm::Value* errorMsg = Builder.CreateGlobalString("Segmentation fault: array index out of bounds\n", "array_seg_fault_msg");
            llvm::Value* errorMsgPtr = Builder.CreatePointerCast(errorMsg, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            Builder.CreateCall(printfFunc, {errorMsgPtr});
            Builder.CreateCall(exitFunc, {llvm::ConstantInt::get(Builder.getInt32Ty(), 139)});
            Builder.CreateUnreachable();
            
            Builder.SetInsertPoint(validBB);
            
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
        
        llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
        if (!printfFunc) {
            llvm::FunctionType* printfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Builder.GetInsertBlock()->getParent()->getParent());
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
        
        llvm::Value* arraySizeValue = llvm::ConstantInt::get(Builder.getInt32Ty(), arraySize);
        llvm::Value* indexTrunc = indexValue;
        if (indexValue->getType() != Builder.getInt32Ty()) {
            indexTrunc = Builder.CreateTrunc(indexValue, Builder.getInt32Ty());
        }
        
        llvm::Value* isOutOfBounds = Builder.CreateICmpUGE(indexTrunc, arraySizeValue);
        
        llvm::BasicBlock* currentBB = Builder.GetInsertBlock();
        llvm::Function* function = currentBB->getParent();
        llvm::BasicBlock* errorBB = llvm::BasicBlock::Create(Builder.getContext(), "array_bounds_error2", function);
        llvm::BasicBlock* validBB = llvm::BasicBlock::Create(Builder.getContext(), "valid_array_access2", function);
        
        Builder.CreateCondBr(isOutOfBounds, errorBB, validBB);
        
        Builder.SetInsertPoint(errorBB);
        llvm::Value* errorMsg = Builder.CreateGlobalString("Segmentation fault: array index out of bounds\n", "array_seg_fault_msg2");
        llvm::Value* errorMsgPtr = Builder.CreatePointerCast(errorMsg, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
        Builder.CreateCall(printfFunc, {errorMsgPtr});
        Builder.CreateCall(exitFunc, {llvm::ConstantInt::get(Builder.getInt32Ty(), 139)});
        Builder.CreateUnreachable();
        
        Builder.SetInsertPoint(validBB);
        
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