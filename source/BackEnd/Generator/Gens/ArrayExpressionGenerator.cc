#include "ArrayExpressionGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateArrayExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    if (Expr->type == NodeType::Array) {
        auto* ArrayNodePtr = static_cast<ArrayNode*>(Expr.get());
        if (!ArrayNodePtr) {
            Write("Expression Generation", "Failed to cast to ArrayNode" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArrayNodePtr->elements.empty()) {
            Write("Expression Generation", "Empty array literal" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ElementType = nullptr;
        if (!ArrayNodePtr->expectedType.empty() && ArrayNodePtr->expectedType != "auto") {
            ElementType = GetLLVMTypeFromString(ArrayNodePtr->expectedType, Builder.getContext());
            if (!ElementType) {
                Write("Expression Generation", "Invalid expected type: " + ArrayNodePtr->expectedType + Location, 2, true, true, "");
                return nullptr;
            }
        } else {
            llvm::Value* firstValue = GenerateExpression(ArrayNodePtr->elements[0], Builder, SymbolStack, Methods);
            if (!firstValue) {
                Write("Expression Generation", "Cannot deduce array element type" + Location, 2, true, true, "");
                return nullptr;
            }
            ElementType = firstValue->getType();
            
            if (ElementType->isFloatingPointTy()) {
                if (llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(firstValue)) {
                    double val = constFP->getValueAPF().convertToDouble();
                    if (val == floor(val)) {
                        ElementType = Builder.getInt32Ty();
                    } else {
                        ElementType = Builder.getDoubleTy();
                    }
                } else {
                    ElementType = Builder.getDoubleTy();
                }
            } else if (ElementType->isIntegerTy()) {
                ElementType = Builder.getInt32Ty();
            } else if (ElementType->isPointerTy()) {
                ElementType = llvm::PointerType::get(Builder.getInt8Ty(), 0);
            }
        }
        
        llvm::Module* module = Builder.GetInsertBlock()->getParent()->getParent();
        llvm::Function* mallocFunc = Malloc(module);
        
        uint64_t arraySize = ArrayNodePtr->elements.size();
        uint64_t elementSize = module->getDataLayout().getTypeAllocSize(ElementType);
        uint64_t totalSize = arraySize * elementSize;
        
        llvm::Value* sizeValue = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), totalSize);
        llvm::Value* rawPtr = Builder.CreateCall(mallocFunc, {sizeValue});
        
        if (!rawPtr) {
            Write("Expression Generation", "Failed to allocate memory for array" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* typedPtrType = llvm::PointerType::get(ElementType, 0);
        llvm::Value* typedArrayPtr = Builder.CreatePointerCast(rawPtr, typedPtrType);
        
        for (size_t i = 0; i < ArrayNodePtr->elements.size(); ++i) {
            llvm::Value* elementValue = GenerateExpression(ArrayNodePtr->elements[i], Builder, SymbolStack, Methods);
            if (!elementValue) {
                Write("Expression Generation", "Failed to generate array element at index " + std::to_string(i) + Location, 2, true, true, "");
                continue;
            }
            
            if (elementValue->getType() != ElementType) {
                if (ElementType->isIntegerTy(32) && elementValue->getType()->isFloatingPointTy()) {
                    elementValue = Builder.CreateFPToSI(elementValue, ElementType);
                } else if (ElementType->isFloatTy()) {
                    if (elementValue->getType()->isIntegerTy()) {
                        elementValue = Builder.CreateSIToFP(elementValue, ElementType);
                    } else if (elementValue->getType()->isDoubleTy()) {
                        elementValue = Builder.CreateFPTrunc(elementValue, ElementType);
                    }
                } else if (ElementType->isDoubleTy()) {
                    if (elementValue->getType()->isIntegerTy()) {
                        elementValue = Builder.CreateSIToFP(elementValue, ElementType);
                    } else if (elementValue->getType()->isFloatTy()) {
                        elementValue = Builder.CreateFPExt(elementValue, ElementType);
                    }
                }
            }
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(ElementType, typedArrayPtr, 
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), i));
            Builder.CreateStore(elementValue, elementPtr);
        }
        
        return typedArrayPtr;
    } else if (Expr->type == NodeType::ArrayAccess) {
        auto* AccessNodePtr = static_cast<ArrayAccessNode*>(Expr.get());
        if (!AccessNodePtr) {
            Write("Expression Generation", "Failed to cast to ArrayAccessNode" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* arrayPtr = nullptr;
        for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
            auto found = it->find(AccessNodePtr->identifier);
            if (found != it->end()) {
                arrayPtr = found->second;
                break;
            }
        }
        
        if (!arrayPtr) {
            Write("Expression Generation", "Undefined array identifier: " + AccessNodePtr->identifier + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
        if (!allocaInst) {
            Write("Expression Generation", "Array identifier is not an allocated variable: " + AccessNodePtr->identifier + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* allocatedType = allocaInst->getAllocatedType();
        
        if (allocatedType->isPointerTy()) {
            llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
            llvm::Value* indexValue = GenerateExpression(AccessNodePtr->expr, Builder, SymbolStack, Methods);
            
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Expression Generation", "Invalid array index" + Location, 2, true, true, "");
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
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(Builder.getInt8Ty(), loadedPtr, indexValue);
            return Builder.CreateLoad(Builder.getInt8Ty(), elementPtr);
        }
        
        if (AccessNodePtr->expr->type == NodeType::Array) {
            auto* IndexArrayPtr = static_cast<ArrayNode*>(AccessNodePtr->expr.get());
            std::vector<llvm::Value*> indices;
            indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
            
            llvm::Type* currentType = allocatedType;
            for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
                if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                    Write("Expression Generation", "Invalid array index" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                if (!currentType->isArrayTy()) {
                    Write("Expression Generation", "Too many dimensions in array access: " + AccessNodePtr->identifier + Location, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                if (!arrayType) {
                    Write("Expression Generation", "Expected array type" + Location, 2, true, true, "");
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
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            return Builder.CreateLoad(currentType, elementPtr);
            
        } else {
            llvm::Value* indexValue = GenerateExpression(AccessNodePtr->expr, Builder, SymbolStack, Methods);
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Expression Generation", "Invalid array index" + Location, 2, true, true, "");
                return nullptr;
            }
            
            if (!allocatedType->isArrayTy()) {
                Write("Expression Generation", "Variable is not an array: " + AccessNodePtr->identifier + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
            if (!arrayType) {
                Write("Expression Generation", "Expected array type" + Location, 2, true, true, "");
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
            
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                indexValue
            };
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            return Builder.CreateLoad(arrayType->getElementType(), elementPtr);
        }
    }
    
    return nullptr;
}