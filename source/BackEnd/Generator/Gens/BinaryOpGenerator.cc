#include "BinaryOpGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateBinaryOp(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods) {
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
       
    llvm::Value* Left = GenerateExpression(BinOpNode->left, IR, Methods);
    if (!Left) {
        Write("Binary Expression", "Invalid left expression for operator " + BinOpNode->op + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* Right = GenerateExpression(BinOpNode->right, IR, Methods);
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
            targetType = IR->f64();
        } else if (L->getType()->isFloatTy() || R->getType()->isFloatTy()) {
            targetType = IR->f32();
        } else if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy()) {
            unsigned leftBits = L->getType()->getIntegerBitWidth();
            unsigned rightBits = R->getType()->getIntegerBitWidth();
            unsigned targetBits = std::max(leftBits, rightBits);
            
            if (targetBits < 32) {
                targetBits = 32;
            }
            
            targetType = llvm::Type::getIntNTy(*IR->getContext(), targetBits);
        } else {
            return nullptr;
        }
        
        if (L->getType()->isIntegerTy() && targetType->isFloatingPointTy()) {
            L = IR->getBuilder()->CreateSIToFP(L, targetType);
        } else if (L->getType()->isIntegerTy() && targetType->isIntegerTy()) {
            if (L->getType()->getIntegerBitWidth() < targetType->getIntegerBitWidth()) {
                L = IR->intCast(L, targetType);
            }
        } else if (L->getType() != targetType) {
            if (L->getType()->isFloatTy() && targetType->isDoubleTy()) {
                L = IR->floatCast(L, targetType);
            } else if (L->getType()->isDoubleTy() && targetType->isFloatTy()) {
                L = IR->floatCast(L, targetType);
            }
        }
        
        if (R->getType()->isIntegerTy() && targetType->isFloatingPointTy()) {
            R = IR->getBuilder()->CreateSIToFP(R, targetType);
        } else if (R->getType()->isIntegerTy() && targetType->isIntegerTy()) {
            if (R->getType()->getIntegerBitWidth() < targetType->getIntegerBitWidth()) {
                R = IR->intCast(R, targetType);
            }
        } else if (R->getType() != targetType) {
            if (R->getType()->isFloatTy() && targetType->isDoubleTy()) {
                R = IR->floatCast(R, targetType);
            } else if (R->getType()->isDoubleTy() && targetType->isFloatTy()) {
                R = IR->floatCast(R, targetType);
            }
        }
        
        return targetType;
    };

    if (BinOpNode->op == "+") {
        if (Left->getType()->isIntegerTy(8) && Right->getType()->isIntegerTy(8)) {
            llvm::Value* size = IR->constI64(3);
            llvm::Value* buffer = IR->malloc(IR->i8(), size);
            
            llvm::Value* ptr0 = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), buffer, IR->constI32(0));
            llvm::Value* ptr1 = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), buffer, IR->constI32(1));
            llvm::Value* ptr2 = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), buffer, IR->constI32(2));
            
            IR->store(Left, ptr0);
            IR->store(Right, ptr1);
            IR->store(IR->constI8(0), ptr2);
            
            return buffer;
        }
        
        if (Left->getType()->isPointerTy() && Right->getType()->isPointerTy()) {
            llvm::Function* strlenFunc = IR->getRegisteredBuiltin("strlen");
            if (!strlenFunc) {
                IR->registerBuiltin("strlen", IR->i64(), {IR->i8ptr()});
                strlenFunc = IR->getRegisteredBuiltin("strlen");
            }

            llvm::Function* strcpyFunc = IR->getRegisteredBuiltin("strcpy");
            if (!strcpyFunc) {
                IR->registerBuiltin("strcpy", IR->i8ptr(), {IR->i8ptr(), IR->i8ptr()});
                strcpyFunc = IR->getRegisteredBuiltin("strcpy");
            }

            llvm::Function* strcatFunc = IR->getRegisteredBuiltin("strcat");
            if (!strcatFunc) {
                IR->registerBuiltin("strcat", IR->i8ptr(), {IR->i8ptr(), IR->i8ptr()});
                strcatFunc = IR->getRegisteredBuiltin("strcat");
            }

            llvm::Value* leftLen = IR->call("strlen", {Left});
            llvm::Value* rightLen = IR->call("strlen", {Right});
            llvm::Value* totalLen = IR->add(leftLen, rightLen);
            llvm::Value* totalLenPlusOne = IR->add(totalLen, IR->constI64(1));

            llvm::Value* resultPtr = IR->malloc(IR->i8(), totalLenPlusOne);
            if (!resultPtr) {
                Write("Binary Expression", "Failed to allocate memory for string concatenation" + Location, 2, true, true, "");
                return nullptr;
            }
            
            IR->call("strcpy", {resultPtr, Left});
            IR->call("strcat", {resultPtr, Right});
            
            return resultPtr;
        }
        
        if (Left->getType()->isPointerTy() && !Right->getType()->isPointerTy()) {
            llvm::Value* rightStr = nullptr;
            if (Right->getType()->isIntegerTy()) {
                llvm::Function* sprintfFunc = IR->getRegisteredBuiltin("sprintf");
                if (!sprintfFunc) {
                    IR->registerBuiltin("sprintf", IR->i32(), {IR->i8ptr(), IR->i8ptr()});
                    sprintfFunc = IR->getRegisteredBuiltin("sprintf");
                }

                rightStr = IR->malloc(IR->i8(), IR->constI64(32));
                if (!rightStr) {
                    Write("Binary Expression", "Failed to allocate memory for integer to string conversion" + Location, 2, true, true, "");
                    return nullptr;
                }
                llvm::Value* formatStr = IR->constString("%d");
                IR->call("sprintf", {rightStr, formatStr, Right});
            } else if (Right->getType()->isFloatingPointTy()) {
                llvm::Function* sprintfFunc = IR->getRegisteredBuiltin("sprintf");
                if (!sprintfFunc) {
                    IR->registerBuiltin("sprintf", IR->i32(), {IR->i8ptr(), IR->i8ptr()});
                    sprintfFunc = IR->getRegisteredBuiltin("sprintf");
                }

                rightStr = IR->malloc(IR->i8(), IR->constI64(32));
                if (!rightStr) {
                    Write("Binary Expression", "Failed to allocate memory for floating-point to string conversion" + Location, 2, true, true, "");
                    return nullptr;
                }
                llvm::Value* formatStr = IR->constString("%.6f");
                
                if (Right->getType()->isFloatTy()) {
                    Right = IR->floatCast(Right, IR->f64());
                }
                
                IR->call("sprintf", {rightStr, formatStr, Right});
            }
            
            if (rightStr) {
                llvm::Value* leftLen = IR->call("strlen", {Left});
                llvm::Value* rightLen = IR->call("strlen", {rightStr});
                llvm::Value* totalLen = IR->add(leftLen, rightLen);
                llvm::Value* totalLenPlusOne = IR->add(totalLen, IR->constI64(1));

                llvm::Value* resultPtr = IR->malloc(IR->i8(), totalLenPlusOne);
                if (!resultPtr) {
                    Write("Binary Expression", "Failed to allocate memory for string concatenation" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                IR->call("strcpy", {resultPtr, Left});
                IR->call("strcat", {resultPtr, rightStr});
                
                return resultPtr;
            }
        }

        if (Left->getType()->isPointerTy() && Right->getType()->isIntegerTy(8)) {
            llvm::Value* leftLen = IR->call("strlen", {Left});
            llvm::Value* newLen = IR->add(leftLen, IR->constI64(2));
            
            llvm::Value* resultPtr = IR->malloc(IR->i8(), newLen);
            if (!resultPtr) {
                Write("Binary Expression", "Failed to allocate memory for string + char concatenation" + Location, 2, true, true, "");
                return nullptr;
            }
            
            IR->call("strcpy", {resultPtr, Left});
            
            llvm::Value* charPos = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), resultPtr, leftLen);
            IR->store(Right, charPos);
            
            llvm::Value* nullPos = IR->add(leftLen, IR->constI64(1));
            llvm::Value* nullPosPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), resultPtr, nullPos);
            IR->store(IR->constI8(0), nullPosPtr);
            
            return resultPtr;
        }
        
        llvm::Type* commonType = promoteToCommonType(Left, Right);
        return IR->add(Left, Right);
    } else if (BinOpNode->op == "-") {
        promoteToCommonType(Left, Right);
        return IR->sub(Left, Right);
    } else if (BinOpNode->op == "*") {
        promoteToCommonType(Left, Right);
        return IR->mul(Left, Right);
    } else if (BinOpNode->op == "/") {
        promoteToCommonType(Left, Right);
        return IR->div(Left, Right);
    } else if (BinOpNode->op == "%") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Modulo operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->mod(Left, Right);
    } else if (BinOpNode->op == "<<") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Shift left operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        if (Left->getType()->isPointerTy() || Right->getType()->isPointerTy()) {
            Write("Binary Expression", "Shift left operator not supported on pointer types" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->shl(Left, Right);
    } else if (BinOpNode->op == ">>") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Shift right operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        if (Left->getType()->isPointerTy() || Right->getType()->isPointerTy()) {
            Write("Binary Expression", "Shift right operator not supported on pointer types" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->ashr(Left, Right);
    } else if (BinOpNode->op == "&") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Bitwise AND operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->bitAnd(Left, Right);
    } else if (BinOpNode->op == "|") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Bitwise OR operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->bitOr(Left, Right);
    } else if (BinOpNode->op == "^") {
        if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
            Write("Binary Expression", "Bitwise XOR operator not supported on floating-point numbers" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->bitXor(Left, Right);
    } else if (BinOpNode->op == ">=") {
        promoteToCommonType(Left, Right);
        return IR->ge(Left, Right);
    } else if (BinOpNode->op == "<=") {
        promoteToCommonType(Left, Right);
        return IR->le(Left, Right);
    } else if (BinOpNode->op == ">") {
        promoteToCommonType(Left, Right);
        return IR->gt(Left, Right);
    } else if (BinOpNode->op == "<") {
        promoteToCommonType(Left, Right);
        return IR->lt(Left, Right);
    } else if (BinOpNode->op == "==") {
        promoteToCommonType(Left, Right);
        return IR->eq(Left, Right);
    } else if (BinOpNode->op == "!=") {
        promoteToCommonType(Left, Right);
        return IR->ne(Left, Right);
    } else if (BinOpNode->op == "&&") {
        llvm::Value* leftBool = Left;
        llvm::Value* rightBool = Right;
        
        if (!Left->getType()->isIntegerTy(1)) {
            if (Left->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Left->getType(), 0.0);
                leftBool = IR->getBuilder()->CreateFCmpONE(Left, zero);
            } else if (Left->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Left->getType(), 0);
                leftBool = IR->getBuilder()->CreateICmpNE(Left, zero);
            }
        }
        
        if (!Right->getType()->isIntegerTy(1)) {
            if (Right->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Right->getType(), 0.0);
                rightBool = IR->getBuilder()->CreateFCmpONE(Right, zero);
            } else if (Right->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Right->getType(), 0);
                rightBool = IR->getBuilder()->CreateICmpNE(Right, zero);
            }
        }
        
        return IR->and_(leftBool, rightBool);
    } else if (BinOpNode->op == "||") {
        llvm::Value* leftBool = Left;
        llvm::Value* rightBool = Right;
        
        if (!Left->getType()->isIntegerTy(1)) {
            if (Left->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Left->getType(), 0.0);
                leftBool = IR->getBuilder()->CreateFCmpONE(Left, zero);
            } else if (Left->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Left->getType(), 0);
                leftBool = IR->getBuilder()->CreateICmpNE(Left, zero);
            }
        }
        
        if (!Right->getType()->isIntegerTy(1)) {
            if (Right->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(Right->getType(), 0.0);
                rightBool = IR->getBuilder()->CreateFCmpONE(Right, zero);
            } else if (Right->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(Right->getType(), 0);
                rightBool = IR->getBuilder()->CreateICmpNE(Right, zero);
            }
        }
        
        return IR->or_(leftBool, rightBool);
    } else {
        Write("Binary Expression", "Unsupported binary operator: " + BinOpNode->op + Location, 2, true, true, "");
        return nullptr;
    }
}