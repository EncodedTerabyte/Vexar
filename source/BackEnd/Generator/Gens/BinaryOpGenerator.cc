#include "BinaryOpGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateBinaryOp(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Expr) {
        Write("Binary Expression", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);

    auto* BinOpNode = static_cast<BinaryOpNode*>(Expr.get());
    if (!BinOpNode) {
        Write("Binary Expression", "Failed to cast ASTNode to BinaryOpNode" + Location, 2, true, true, "");
        return nullptr;
    }
       
    llvm::Value* Left = GenerateExpression(BinOpNode->left, Builder, SymbolStack, Methods);
    if (!Left) {
        Write("Binary Expression", "Invalid left expression for operator " + BinOpNode->op + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* Right = GenerateExpression(BinOpNode->right, Builder, SymbolStack, Methods);
    if (!Right) {
        Write("Binary Expression", "Invalid right expression for operator " + BinOpNode->op + Location, 2, true, true, "");
        return nullptr;
    }

    auto promoteToCommonType = [&](llvm::Value*& L, llvm::Value*& R) -> llvm::Type* {
        if (L->getType()->isPointerTy() || R->getType()->isPointerTy()) {
            return nullptr;
        }
        
        llvm::Type* targetType = nullptr;
        if (L->getType()->isDoubleTy() || R->getType()->isDoubleTy()) {
            targetType = llvm::Type::getDoubleTy(Builder.getContext());
        } else if (L->getType()->isFloatTy() || R->getType()->isFloatTy()) {
            targetType = llvm::Type::getFloatTy(Builder.getContext());
        } else if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy()) {
            return L->getType();
        } else {
            return nullptr;
        }
        
        if (L->getType()->isIntegerTy()) {
            L = Builder.CreateSIToFP(L, targetType);
        } else if (L->getType() != targetType) {
            if (L->getType()->isFloatTy() && targetType->isDoubleTy()) {
                L = Builder.CreateFPExt(L, targetType);
            } else if (L->getType()->isDoubleTy() && targetType->isFloatTy()) {
                L = Builder.CreateFPTrunc(L, targetType);
            }
        }
        
        if (R->getType()->isIntegerTy()) {
            R = Builder.CreateSIToFP(R, targetType);
        } else if (R->getType() != targetType) {
            if (R->getType()->isFloatTy() && targetType->isDoubleTy()) {
                R = Builder.CreateFPExt(R, targetType);
            } else if (R->getType()->isDoubleTy() && targetType->isFloatTy()) {
                R = Builder.CreateFPTrunc(R, targetType);
            }
        }
        
        return targetType;
    };

    if (BinOpNode->op == "+") {
        if (Left->getType()->isIntegerTy(8) && Right->getType()->isIntegerTy(8)) {
            llvm::LLVMContext& ctx = Builder.getContext();
            llvm::Type* i8Type = llvm::Type::getInt8Ty(ctx);
            
            llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
            if (!mallocFunc) {
                llvm::FunctionType* mallocType = llvm::FunctionType::get(
                    llvm::PointerType::get(ctx, 0),
                    {llvm::Type::getInt64Ty(ctx)},
                    false
                );
                mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
            }
            
            llvm::Value* size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), 3);
            llvm::Value* buffer = Builder.CreateCall(mallocFunc, {size});
            
            llvm::Value* ptr0 = Builder.CreateInBoundsGEP(i8Type, buffer, llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 0));
            llvm::Value* ptr1 = Builder.CreateInBoundsGEP(i8Type, buffer, llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 1));
            llvm::Value* ptr2 = Builder.CreateInBoundsGEP(i8Type, buffer, llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 2));
            
            Builder.CreateStore(Left, ptr0);
            Builder.CreateStore(Right, ptr1);
            Builder.CreateStore(llvm::ConstantInt::get(i8Type, 0), ptr2);
            
            return buffer;
        }
        
        if (Left->getType()->isPointerTy() && Right->getType()->isPointerTy()) {
            llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
            if (!strlenFunc) {
                llvm::FunctionType* strlenType = llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(Builder.getContext()),
                    {llvm::PointerType::get(Builder.getContext(), 0)},
                    false
                );
                strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
            if (!mallocFunc) {
                llvm::FunctionType* mallocType = llvm::FunctionType::get(
                    llvm::PointerType::get(Builder.getContext(), 0),
                    {llvm::Type::getInt64Ty(Builder.getContext())},
                    false
                );
                mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
            if (!strcpyFunc) {
                llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                    llvm::PointerType::get(Builder.getContext(), 0),
                    {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                    false
                );
                strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
            if (!strcatFunc) {
                llvm::FunctionType* strcatType = llvm::FunctionType::get(
                    llvm::PointerType::get(Builder.getContext(), 0),
                    {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                    false
                );
                strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
            llvm::Value* rightLen = Builder.CreateCall(strlenFunc, {Right});
            llvm::Value* totalLen = Builder.CreateAdd(leftLen, rightLen);
            llvm::Value* totalLenPlusOne = Builder.CreateAdd(totalLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));

            llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {totalLenPlusOne});
            if (!resultPtr) {
                Write("Binary Expression", "Failed to allocate memory for string concatenation" + Location, 2, true, true, "");
                return nullptr;
            }
            
            Builder.CreateCall(strcpyFunc, {resultPtr, Left});
            Builder.CreateCall(strcatFunc, {resultPtr, Right});
            
            return resultPtr;
        }
        
        if (Left->getType()->isPointerTy() && !Right->getType()->isPointerTy()) {
            llvm::Value* rightStr = nullptr;
            if (Right->getType()->isIntegerTy()) {
                llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                if (!mallocFunc) {
                    llvm::FunctionType* mallocType = llvm::FunctionType::get(
                        llvm::PointerType::get(Builder.getContext(), 0),
                        {llvm::Type::getInt64Ty(Builder.getContext())},
                        false
                    );
                    mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
                if (!sprintfFunc) {
                    llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                        llvm::Type::getInt32Ty(Builder.getContext()),
                        {llvm::PointerType::get(Builder.getContext(), 0), 
                        llvm::PointerType::get(Builder.getContext(), 0)},
                        true
                    );
                    sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", Builder.GetInsertBlock()->getParent()->getParent());
                }

                rightStr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
                if (!rightStr) {
                    Write("Binary Expression", "Failed to allocate memory for integer to string conversion" + Location, 2, true, true, "");
                    return nullptr;
                }
                llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
                llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
                Builder.CreateCall(sprintfFunc, {rightStr, formatPtr, Right});
            } else if (Right->getType()->isFloatingPointTy()) {
                llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                if (!mallocFunc) {
                    llvm::FunctionType* mallocType = llvm::FunctionType::get(
                        llvm::PointerType::get(Builder.getContext(), 0),
                        {llvm::Type::getInt64Ty(Builder.getContext())},
                        false
                    );
                    mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
                if (!sprintfFunc) {
                    llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                        llvm::Type::getInt32Ty(Builder.getContext()),
                        {llvm::PointerType::get(Builder.getContext(), 0), 
                        llvm::PointerType::get(Builder.getContext(), 0)},
                        true
                    );
                    sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", Builder.GetInsertBlock()->getParent()->getParent());
                }

                rightStr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
                if (!rightStr) {
                    Write("Binary Expression", "Failed to allocate memory for floating-point to string conversion" + Location, 2, true, true, "");
                    return nullptr;
                }
                llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
                llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
                
                if (Right->getType()->isFloatTy()) {
                    Right = Builder.CreateFPExt(Right, llvm::Type::getDoubleTy(Builder.getContext()));
                }
                
                Builder.CreateCall(sprintfFunc, {rightStr, formatPtr, Right});
            }
            
            if (rightStr) {
                llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
                if (!strlenFunc) {
                    llvm::FunctionType* strlenType = llvm::FunctionType::get(
                        llvm::Type::getInt64Ty(Builder.getContext()),
                        {llvm::PointerType::get(Builder.getContext(), 0)},
                        false
                    );
                    strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
                if (!strcpyFunc) {
                    llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                        llvm::PointerType::get(Builder.getContext(), 0),
                        {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                        false
                    );
                    strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
                if (!strcatFunc) {
                    llvm::FunctionType* strcatType = llvm::FunctionType::get(
                        llvm::PointerType::get(Builder.getContext(), 0),
                        {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                        false
                    );
                    strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
                llvm::Value* rightLen = Builder.CreateCall(strlenFunc, {rightStr});
                llvm::Value* totalLen = Builder.CreateAdd(leftLen, rightLen);
                llvm::Value* totalLenPlusOne = Builder.CreateAdd(totalLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));

                llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {totalLenPlusOne});
                if (!resultPtr) {
                    Write("Binary Expression", "Failed to allocate memory for string concatenation" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                Builder.CreateCall(strcpyFunc, {resultPtr, Left});
                Builder.CreateCall(strcatFunc, {resultPtr, rightStr});
                
                return resultPtr;
            }
        }

        if (Left->getType()->isPointerTy() && Right->getType()->isIntegerTy(8)) {
            llvm::LLVMContext& ctx = Builder.getContext();
            
            llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
            if (!strlenFunc) {
                llvm::FunctionType* strlenType = llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(ctx),
                    {llvm::PointerType::get(ctx, 0)},
                    false
                );
                strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
            if (!mallocFunc) {
                llvm::FunctionType* mallocType = llvm::FunctionType::get(
                    llvm::PointerType::get(ctx, 0),
                    {llvm::Type::getInt64Ty(ctx)},
                    false
                );
                mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
            if (!strcpyFunc) {
                llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                    llvm::PointerType::get(ctx, 0),
                    {llvm::PointerType::get(ctx, 0), llvm::PointerType::get(ctx, 0)},
                    false
                );
                strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
            }

            // Get length of the string
            llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
            // Add 2: one for the char, one for null terminator
            llvm::Value* newLen = Builder.CreateAdd(leftLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), 2));
            
            // Allocate new buffer
            llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {newLen});
            if (!resultPtr) {
                Write("Binary Expression", "Failed to allocate memory for string + char concatenation" + Location, 2, true, true, "");
                return nullptr;
            }
            
            // Copy the original string
            Builder.CreateCall(strcpyFunc, {resultPtr, Left});
            
            // Append the character manually
            llvm::Value* charPos = Builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx), resultPtr, leftLen);
            Builder.CreateStore(Right, charPos);
            
            // Add null terminator
            llvm::Value* nullPos = Builder.CreateAdd(leftLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), 1));
            llvm::Value* nullPosPtr = Builder.CreateInBoundsGEP(llvm::Type::getInt8Ty(ctx), resultPtr, nullPos);
            Builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx), 0), nullPosPtr);
            
            return resultPtr;
        }
        
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFAdd(Left, Right, "addtmp");
        } else {
            return Builder.CreateAdd(Left, Right, "addtmp");
        }
    } else if (BinOpNode->op == "-") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFSub(Left, Right, "subtmp");
        } else {
            return Builder.CreateSub(Left, Right, "subtmp");
        }
    } else if (BinOpNode->op == "*") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFMul(Left, Right, "multmp");
        } else {
            return Builder.CreateMul(Left, Right, "multmp");
        }
    } else if (BinOpNode->op == "/") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFDiv(Left, Right, "divtmp");
        } else {
            return Builder.CreateSDiv(Left, Right, "divtmp");
        }
    } else if (BinOpNode->op == "%") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Modulo operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return Builder.CreateSRem(Left, Right, "srem");
    } else if (BinOpNode->op == "<<") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Shift left operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return Builder.CreateShl(Left, Right, "shltmp");
    } else if (BinOpNode->op == ">>") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Shift right operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return Builder.CreateAShr(Left, Right, "ashrtmp");
    } else if (BinOpNode->op == "&") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Bitwise AND operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return Builder.CreateAnd(Left, Right, "andtmp");
    } else if (BinOpNode->op == "|") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Bitwise OR operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return Builder.CreateOr(Left, Right, "ortmp");
    } else if (BinOpNode->op == "^") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Bitwise XOR operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return Builder.CreateXor(Left, Right, "xortmp");
    } else if (BinOpNode->op == ">=") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFCmpOGE(Left, Right, "fcmp_ge");
        } else {
            return Builder.CreateICmpSGE(Left, Right, "icmp_ge");
        }
    } else if (BinOpNode->op == "<=") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFCmpOLE(Left, Right, "fcmp_le");
        } else {
            return Builder.CreateICmpSLE(Left, Right, "icmp_le");
        }
    } else if (BinOpNode->op == ">") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFCmpOGT(Left, Right, "fcmp_gt");
        } else {
            return Builder.CreateICmpSGT(Left, Right, "icmp_gt");
        }
    } else if (BinOpNode->op == "<") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFCmpOLT(Left, Right, "fcmp_lt");
        } else {
            return Builder.CreateICmpSLT(Left, Right, "icmp_lt");
        }
    } else if (BinOpNode->op == "==") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFCmpOEQ(Left, Right, "fcmp_eq");
        } else {
            return Builder.CreateICmpEQ(Left, Right, "icmp_eq");
        }
    } else if (BinOpNode->op == "!=") {
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        if (commonType && commonType->isFloatingPointTy()) {
            return Builder.CreateFCmpONE(Left, Right, "fcmp_ne");
        } else {
            return Builder.CreateICmpNE(Left, Right, "icmp_ne");
        }
    } else if (BinOpNode->op == "&&") {
        llvm::Value* leftBool = Left;
        llvm::Value* rightBool = Right;
        
        if (!Left->getType()->isIntegerTy(1)) {
            if (Left->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Left->getType(), 0.0);
                leftBool = Builder.CreateFCmpONE(Left, zero, "tobool");
            } else if (Left->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Left->getType(), 0);
                leftBool = Builder.CreateICmpNE(Left, zero, "tobool");
            }
        }
        
        if (!Right->getType()->isIntegerTy(1)) {
            if (Right->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Right->getType(), 0.0);
                rightBool = Builder.CreateFCmpONE(Right, zero, "tobool");
            } else if (Right->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Right->getType(), 0);
                rightBool = Builder.CreateICmpNE(Right, zero, "tobool");
            }
        }
        
        return Builder.CreateAnd(leftBool, rightBool, "and");
    } else if (BinOpNode->op == "||") {
        llvm::Value* leftBool = Left;
        llvm::Value* rightBool = Right;
        
        if (!Left->getType()->isIntegerTy(1)) {
            if (Left->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Left->getType(), 0.0);
                leftBool = Builder.CreateFCmpONE(Left, zero, "tobool");
            } else if (Left->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Left->getType(), 0);
                leftBool = Builder.CreateICmpNE(Left, zero, "tobool");
            }
        }
        
        if (!Right->getType()->isIntegerTy(1)) {
            if (Right->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Right->getType(), 0.0);
                rightBool = Builder.CreateFCmpONE(Right, zero, "tobool");
            } else if (Right->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Right->getType(), 0);
                rightBool = Builder.CreateICmpNE(Right, zero, "tobool");
            }
        }
        
        return Builder.CreateOr(leftBool, rightBool, "or");
    } else {
        Write("Binary Expression", "Unsupported binary operator: " + BinOpNode->op + Location, 2, true, true, "");
        return nullptr;
    }
}