#include "BinaryOpGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateBinaryOp(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    auto* BinOpNode = static_cast<BinaryOpNode*>(Expr.get());
       
    llvm::Value* Left = GenerateExpression(BinOpNode->left, Builder, SymbolStack, Methods);
    llvm::Value* Right = GenerateExpression(BinOpNode->right, Builder, SymbolStack, Methods);

    if (BinOpNode->op == "+") {
        if (Left->getType()->isPointerTy() && Right->getType()->isPointerTy()) {
            llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
            if (!strlenFunc) {
                llvm::FunctionType* strlenType = llvm::FunctionType::get(
                    llvm::Type::getInt64Ty(Builder.getContext()),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    false
                );
                strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
            if (!mallocFunc) {
                llvm::FunctionType* mallocType = llvm::FunctionType::get(
                    llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                    {llvm::Type::getInt64Ty(Builder.getContext())},
                    false
                );
                mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
            if (!strcpyFunc) {
                llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                    llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    false
                );
                strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
            if (!strcatFunc) {
                llvm::FunctionType* strcatType = llvm::FunctionType::get(
                    llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    false
                );
                strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
            llvm::Value* rightLen = Builder.CreateCall(strlenFunc, {Right});
            llvm::Value* totalLen = Builder.CreateAdd(leftLen, rightLen);
            llvm::Value* totalLenPlusOne = Builder.CreateAdd(totalLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));

            llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {totalLenPlusOne});
            
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
                        llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                        {llvm::Type::getInt64Ty(Builder.getContext())},
                        false
                    );
                    mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
                if (!sprintfFunc) {
                    llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                        llvm::Type::getInt32Ty(Builder.getContext()),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                        llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        true
                    );
                    sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", Builder.GetInsertBlock()->getParent()->getParent());
                }

                rightStr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
                llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
                llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
                Builder.CreateCall(sprintfFunc, {rightStr, formatPtr, Right});
            } else if (Right->getType()->isFloatingPointTy()) {
                llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                if (!mallocFunc) {
                    llvm::FunctionType* mallocType = llvm::FunctionType::get(
                        llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                        {llvm::Type::getInt64Ty(Builder.getContext())},
                        false
                    );
                    mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
                if (!sprintfFunc) {
                    llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                        llvm::Type::getInt32Ty(Builder.getContext()),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                        llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        true
                    );
                    sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", Builder.GetInsertBlock()->getParent()->getParent());
                }

                rightStr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
                llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
                llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
                
                if (Right->getType()->isFloatTy()) {
                    Right = Builder.CreateFPExt(Right, Builder.getDoubleTy());
                }
                
                Builder.CreateCall(sprintfFunc, {rightStr, formatPtr, Right});
            }
            
            if (rightStr) {
                llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
                if (!strlenFunc) {
                    llvm::FunctionType* strlenType = llvm::FunctionType::get(
                        llvm::Type::getInt64Ty(Builder.getContext()),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        false
                    );
                    strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
                if (!strcpyFunc) {
                    llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                        llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        false
                    );
                    strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
                if (!strcatFunc) {
                    llvm::FunctionType* strcatType = llvm::FunctionType::get(
                        llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        false
                    );
                    strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
                }

                llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
                llvm::Value* rightLen = Builder.CreateCall(strlenFunc, {rightStr});
                llvm::Value* totalLen = Builder.CreateAdd(leftLen, rightLen);
                llvm::Value* totalLenPlusOne = Builder.CreateAdd(totalLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));

                llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {totalLenPlusOne});
                
                Builder.CreateCall(strcpyFunc, {resultPtr, Left});
                Builder.CreateCall(strcatFunc, {resultPtr, rightStr});
                
                return resultPtr;
            }
        }
        
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFAdd(Left, Right, "addtmp");
        } else {
            return Builder.CreateAdd(Left, Right, "addtmp");
        }
    } else if (BinOpNode->op == "-") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFSub(Left, Right, "subtmp");
        } else {
            return Builder.CreateSub(Left, Right, "subtmp");
        }
    } else if (BinOpNode->op == "*") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFMul(Left, Right, "multmp");
        } else {
            return Builder.CreateMul(Left, Right, "multmp");
        }
    } else if (BinOpNode->op == "/") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFDiv(Left, Right, "divtmp");
        } else {
            return Builder.CreateSDiv(Left, Right, "divtmp");
        }
    } else if (BinOpNode->op == ">=") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFCmpOGE(Left, Right, "fcmp_ge");
        } else {
            return Builder.CreateICmpSGE(Left, Right, "icmp_ge");
        }
    } else if (BinOpNode->op == "<=") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFCmpOLE(Left, Right, "fcmp_le");
        } else {
            return Builder.CreateICmpSLE(Left, Right, "icmp_le");
        }
    } else if (BinOpNode->op == ">") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFCmpOGT(Left, Right, "fcmp_gt");
        } else {
            return Builder.CreateICmpSGT(Left, Right, "icmp_gt");
        }
    } else if (BinOpNode->op == "<") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFCmpOLT(Left, Right, "fcmp_lt");
        } else {
            return Builder.CreateICmpSLT(Left, Right, "icmp_lt");
        }
    } else if (BinOpNode->op == "==") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFCmpOEQ(Left, Right, "fcmp_eq");
        } else {
            return Builder.CreateICmpEQ(Left, Right, "icmp_eq");
        }
    } else if (BinOpNode->op == "!=") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            if (Left->getType()->isIntegerTy()) {
                Left = Builder.CreateSIToFP(Left, Right->getType());
            } else if (Right->getType()->isIntegerTy()) {
                Right = Builder.CreateSIToFP(Right, Left->getType());
            }
            return Builder.CreateFCmpONE(Left, Right, "fcmp_ne");
        } else {
            return Builder.CreateICmpNE(Left, Right, "icmp_ne");
        }
    } else {
        exit(1);
    }
}