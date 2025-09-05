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
        if (Left->getType()->isPointerTy() && Right->getType()->isPointerTy()) {
            llvm::Value* leftLen = IR->var("left_len", IR->i32(), IR->constI32(0));
            llvm::Value* tempPtr = Left;
            
            llvm::BasicBlock* leftLenLoop = IR->createBlock("left_len_loop");
            llvm::BasicBlock* leftLenDone = IR->createBlock("left_len_done");
            llvm::BasicBlock* leftLenCheck = IR->createBlock("left_len_check");
            
            IR->branch(leftLenLoop);
            IR->setInsertPoint(leftLenLoop);
            
            llvm::Value* currentLeftLen = IR->load(leftLen);
            llvm::Value* leftCharPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), tempPtr, currentLeftLen);
            llvm::Value* leftChar = IR->load(leftCharPtr, IR->i8());
            llvm::Value* leftIsEnd = IR->eq(leftChar, IR->constI8(0));
            IR->condBranch(leftIsEnd, leftLenDone, leftLenCheck);
            
            IR->setInsertPoint(leftLenCheck);
            llvm::Value* nextLeftLen = IR->add(currentLeftLen, IR->constI32(1));
            IR->store(nextLeftLen, leftLen);
            IR->branch(leftLenLoop);
            
            IR->setInsertPoint(leftLenDone);
            llvm::Value* finalLeftLen = IR->load(leftLen);
            
            llvm::Value* rightLen = IR->var("right_len", IR->i32(), IR->constI32(0));
            tempPtr = Right;
            
            llvm::BasicBlock* rightLenLoop = IR->createBlock("right_len_loop");
            llvm::BasicBlock* rightLenDone = IR->createBlock("right_len_done");
            llvm::BasicBlock* rightLenCheck = IR->createBlock("right_len_check");
            
            IR->branch(rightLenLoop);
            IR->setInsertPoint(rightLenLoop);
            
            llvm::Value* currentRightLen = IR->load(rightLen);
            llvm::Value* rightCharPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), Right, currentRightLen);
            llvm::Value* rightChar = IR->load(rightCharPtr, IR->i8());
            llvm::Value* rightIsEnd = IR->eq(rightChar, IR->constI8(0));
            IR->condBranch(rightIsEnd, rightLenDone, rightLenCheck);
            
            IR->setInsertPoint(rightLenCheck);
            llvm::Value* nextRightLen = IR->add(currentRightLen, IR->constI32(1));
            IR->store(nextRightLen, rightLen);
            IR->branch(rightLenLoop);
            
            IR->setInsertPoint(rightLenDone);
            llvm::Value* finalRightLen = IR->load(rightLen);
            
            llvm::Value* totalLen = IR->add(finalLeftLen, finalRightLen);
            llvm::Value* newSize = IR->add(totalLen, IR->constI32(1));
            llvm::Value* newSizeI64 = IR->intCast(newSize, IR->i64());
            
            llvm::Value* result = IR->malloc(IR->i8(), newSizeI64);
            
            llvm::Value* copyIdx = IR->var("copy_idx", IR->i32(), IR->constI32(0));
            
            llvm::BasicBlock* copyLeftLoop = IR->createBlock("copy_left_loop");
            llvm::BasicBlock* copyLeftDone = IR->createBlock("copy_left_done");
            llvm::BasicBlock* copyLeftNext = IR->createBlock("copy_left_next");
            
            IR->branch(copyLeftLoop);
            IR->setInsertPoint(copyLeftLoop);
            
            llvm::Value* leftIdx = IR->load(copyIdx);
            llvm::Value* shouldCopyLeft = IR->lt(leftIdx, finalLeftLen);
            IR->condBranch(shouldCopyLeft, copyLeftNext, copyLeftDone);
            
            IR->setInsertPoint(copyLeftNext);
            llvm::Value* leftSrcPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), Left, leftIdx);
            llvm::Value* leftDstPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), result, leftIdx);
            llvm::Value* leftCopyChar = IR->load(leftSrcPtr, IR->i8());
            IR->store(leftCopyChar, leftDstPtr);
            llvm::Value* nextLeftIdx = IR->add(leftIdx, IR->constI32(1));
            IR->store(nextLeftIdx, copyIdx);
            IR->branch(copyLeftLoop);
            
            IR->setInsertPoint(copyLeftDone);
            IR->store(IR->constI32(0), copyIdx);
            
            llvm::BasicBlock* copyRightLoop = IR->createBlock("copy_right_loop");
            llvm::BasicBlock* copyRightDone = IR->createBlock("copy_right_done");
            llvm::BasicBlock* copyRightNext = IR->createBlock("copy_right_next");
            
            IR->branch(copyRightLoop);
            IR->setInsertPoint(copyRightLoop);
            
            llvm::Value* rightIdx = IR->load(copyIdx);
            llvm::Value* shouldCopyRight = IR->lt(rightIdx, finalRightLen);
            IR->condBranch(shouldCopyRight, copyRightNext, copyRightDone);
            
            IR->setInsertPoint(copyRightNext);
            llvm::Value* rightSrcPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), Right, rightIdx);
            llvm::Value* destIdx = IR->add(finalLeftLen, rightIdx);
            llvm::Value* rightDstPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), result, destIdx);
            llvm::Value* rightCopyChar = IR->load(rightSrcPtr, IR->i8());
            IR->store(rightCopyChar, rightDstPtr);
            llvm::Value* nextRightIdx = IR->add(rightIdx, IR->constI32(1));
            IR->store(nextRightIdx, copyIdx);
            IR->branch(copyRightLoop);
            
            IR->setInsertPoint(copyRightDone);
            llvm::Value* nullPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), result, totalLen);
            IR->store(IR->constI8(0), nullPtr);
            
            return result;
        }
        
        if (Left->getType()->isPointerTy() && Right->getType()->isIntegerTy(8)) {
            llvm::Value* strLen = IR->var("str_len", IR->i32(), IR->constI32(0));
            llvm::Value* tempPtr = Left;
            
            llvm::BasicBlock* lenLoop = IR->createBlock("len_loop");
            llvm::BasicBlock* lenDone = IR->createBlock("len_done");
            llvm::BasicBlock* lenCheck = IR->createBlock("len_check");
            
            IR->branch(lenLoop);
            IR->setInsertPoint(lenLoop);
            
            llvm::Value* currentLen = IR->load(strLen);
            llvm::Value* charPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), tempPtr, currentLen);
            llvm::Value* currentChar = IR->load(charPtr, IR->i8());
            llvm::Value* isEnd = IR->eq(currentChar, IR->constI8(0));
            IR->condBranch(isEnd, lenDone, lenCheck);
            
            IR->setInsertPoint(lenCheck);
            llvm::Value* nextLen = IR->add(currentLen, IR->constI32(1));
            IR->store(nextLen, strLen);
            IR->branch(lenLoop);
            
            IR->setInsertPoint(lenDone);
            llvm::Value* finalLen = IR->load(strLen);
            llvm::Value* newSize = IR->add(finalLen, IR->constI32(2));
            llvm::Value* newSizeI64 = IR->intCast(newSize, IR->i64());
            
            llvm::Value* result = IR->malloc(IR->i8(), newSizeI64);
            
            llvm::Value* copyIdx = IR->var("copy_idx", IR->i32(), IR->constI32(0));
            
            llvm::BasicBlock* copyLoop = IR->createBlock("copy_loop");
            llvm::BasicBlock* copyDone = IR->createBlock("copy_done");
            llvm::BasicBlock* copyNext = IR->createBlock("copy_next");
            
            IR->branch(copyLoop);
            IR->setInsertPoint(copyLoop);
            
            llvm::Value* idx = IR->load(copyIdx);
            llvm::Value* shouldCopy = IR->lt(idx, finalLen);
            IR->condBranch(shouldCopy, copyNext, copyDone);
            
            IR->setInsertPoint(copyNext);
            llvm::Value* srcPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), Left, idx);
            llvm::Value* dstPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), result, idx);
            llvm::Value* copyChar = IR->load(srcPtr, IR->i8());
            IR->store(copyChar, dstPtr);
            llvm::Value* nextIdx = IR->add(idx, IR->constI32(1));
            IR->store(nextIdx, copyIdx);
            IR->branch(copyLoop);
            
            IR->setInsertPoint(copyDone);
            llvm::Value* charPos = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), result, finalLen);
            IR->store(Right, charPos);
            llvm::Value* nullPos = IR->add(finalLen, IR->constI32(1));
            llvm::Value* nullPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), result, nullPos);
            IR->store(IR->constI8(0), nullPtr);
            
            return result;
        }
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
        if (Left->getType() != Right->getType()) {
            if (Left->getType()->isIntegerTy() && Right->getType()->isIntegerTy()) {
                unsigned leftBits = Left->getType()->getIntegerBitWidth();
                unsigned rightBits = Right->getType()->getIntegerBitWidth();
                if (leftBits < rightBits) {
                    Left = IR->intCast(Left, Right->getType());
                } else if (rightBits < leftBits) {
                    Right = IR->intCast(Right, Left->getType());
                }
            } else {
                promoteToCommonType(Left, Right);
            }
        } else {
            promoteToCommonType(Left, Right);
        }
        return IR->eq(Left, Right);
    } else if (BinOpNode->op == "!=") {
        if (Left->getType() != Right->getType()) {
            if (Left->getType()->isIntegerTy() && Right->getType()->isIntegerTy()) {
                unsigned leftBits = Left->getType()->getIntegerBitWidth();
                unsigned rightBits = Right->getType()->getIntegerBitWidth();
                if (leftBits < rightBits) {
                    Left = IR->intCast(Left, Right->getType());
                } else if (rightBits < leftBits) {
                    Right = IR->intCast(Right, Left->getType());
                }
            } else {
                promoteToCommonType(Left, Right);
            }
        } else {
            promoteToCommonType(Left, Right);
        }
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