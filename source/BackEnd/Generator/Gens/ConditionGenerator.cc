#include "ConditionGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* ResolveIdentifier(const std::string& name, AeroIR* IR) {
    llvm::Value* varPtr = IR->getVar(name);
    if (varPtr) {
        return IR->load(varPtr);
    }
    Write("Condition Generation", "Identifier not found: " + name, 2, true, true, "");
    return nullptr;
}

llvm::Value* GenerateCondition(ConditionNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Condition Generation", "Null ConditionNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    if (!Node->expression) {
        Write("Condition Generation", "Null expression in ConditionNode" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* Result = GenerateConditionExpression(Node->expression, IR, Methods);
    if (!Result) {
        Write("Condition Generation", "Failed to generate condition expression" + Location, 2, true, true, "");
        return nullptr;
    }

    return Result;
}

llvm::Value* GenerateConditionExpression(const std::unique_ptr<ASTNode>& expr, AeroIR* IR, FunctionSymbols& Methods) {
    if (!expr) {
        Write("Condition Expression", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(expr->token.line) + ", column " + std::to_string(expr->token.column);

    if (expr->type == NodeType::Boolean) {
        auto* boolNode = static_cast<BooleanNode*>(expr.get());
        if (!boolNode) {
            Write("Condition Expression", "Failed to cast to BooleanNode" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->constBool(boolNode->value);
    }

    if (expr->type == NodeType::Paren) {
        auto* parenNode = static_cast<ParenNode*>(expr.get());
        if (!parenNode) {
            Write("Condition Expression", "Failed to cast to ParenNode" + Location, 2, true, true, "");
            return nullptr;
        }
        if (!parenNode->inner) {
            Write("Condition Expression", "Null inner expression in ParenNode" + Location, 2, true, true, "");
            return nullptr;
        }
        return GenerateConditionExpression(parenNode->inner, IR, Methods);
    }

    if (expr->type == NodeType::BinaryOp) {
        auto* binOpNode = static_cast<BinaryOpNode*>(expr.get());
        if (!binOpNode) {
            Write("Condition Expression", "Failed to cast to BinaryOpNode" + Location, 2, true, true, "");
            return nullptr;
        }

        if (binOpNode->op == "&&") {
            if (!binOpNode->left) {
                Write("Condition Expression", "Null left operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, IR, Methods);
            if (!left) {
                Write("Condition Expression", "Invalid left operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::Function* currentFunc = IR->getBuilder()->GetInsertBlock()->getParent();
            if (!currentFunc) {
                Write("Condition Expression", "Invalid current function for &&" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::BasicBlock* leftBB = IR->getBuilder()->GetInsertBlock();
            llvm::BasicBlock* rhsBB = IR->createBlock("land.rhs");
            llvm::BasicBlock* endBB = IR->createBlock("land.end");
            
            IR->condBranch(left, rhsBB, endBB);
            
            IR->setInsertPoint(rhsBB);
            if (!binOpNode->right) {
                Write("Condition Expression", "Null right operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, IR, Methods);
            if (!right) {
                Write("Condition Expression", "Invalid right operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }
            IR->branch(endBB);
            rhsBB = IR->getBuilder()->GetInsertBlock();
            
            IR->setInsertPoint(endBB);
            llvm::PHINode* phi = IR->getBuilder()->CreatePHI(IR->bool_t(), 2, "land");
            phi->addIncoming(IR->constBool(false), leftBB);
            phi->addIncoming(right, rhsBB);
            
            return phi;
        }
        
        if (binOpNode->op == "||") {
            if (!binOpNode->left) {
                Write("Condition Expression", "Null left operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, IR, Methods);
            if (!left) {
                Write("Condition Expression", "Invalid left operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::Function* currentFunc = IR->getBuilder()->GetInsertBlock()->getParent();
            if (!currentFunc) {
                Write("Condition Expression", "Invalid current function for ||" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::BasicBlock* leftBB = IR->getBuilder()->GetInsertBlock();
            llvm::BasicBlock* rhsBB = IR->createBlock("lor.rhs");
            llvm::BasicBlock* endBB = IR->createBlock("lor.end");
            
            IR->condBranch(left, endBB, rhsBB);
            
            IR->setInsertPoint(rhsBB);
            if (!binOpNode->right) {
                Write("Condition Expression", "Null right operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, IR, Methods);
            if (!right) {
                Write("Condition Expression", "Invalid right operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }
            IR->branch(endBB);
            rhsBB = IR->getBuilder()->GetInsertBlock();
            
            IR->setInsertPoint(endBB);
            llvm::PHINode* phi = IR->getBuilder()->CreatePHI(IR->bool_t(), 2, "lor");
            phi->addIncoming(IR->constBool(true), leftBB);
            phi->addIncoming(right, rhsBB);
            
            return phi;
        }

        if (!binOpNode->left || !binOpNode->right) {
            Write("Condition Expression", "Null operand for binary operator " + binOpNode->op + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Value* left = GenerateExpression(binOpNode->left, IR, Methods);
        if (!left) {
            Write("Condition Expression", "Invalid left expression for operator " + binOpNode->op + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Value* right = GenerateExpression(binOpNode->right, IR, Methods);
        if (!right) {
            Write("Condition Expression", "Invalid right expression for operator " + binOpNode->op + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Type* leftType = left->getType();
        llvm::Type* rightType = right->getType();

        if (binOpNode->op == "&" || binOpNode->op == "|" || binOpNode->op == "^") {
            if (!leftType->isIntegerTy() || !rightType->isIntegerTy()) {
                Write("Condition Expression", "Bitwise operators require integer operands" + Location, 2, true, true, "");
                return nullptr;
            }
            
            if (leftType != rightType) {
                if (leftType->getIntegerBitWidth() > rightType->getIntegerBitWidth()) {
                    right = IR->intCast(right, leftType);
                } else if (rightType->getIntegerBitWidth() > leftType->getIntegerBitWidth()) {
                    left = IR->intCast(left, rightType);
                }
            }
            
            if (binOpNode->op == "&") {
                return IR->bitAnd(left, right);
            } else if (binOpNode->op == "|") {
                return IR->bitOr(left, right);
            } else if (binOpNode->op == "^") {
                return IR->bitXor(left, right);
            }
        }

        if (leftType->isPointerTy() && rightType->isPointerTy()) {
            llvm::Function* strcmpFunc = IR->getModule()->getFunction("strcmp");
            if (!strcmpFunc) {
                strcmpFunc = IR->createBuiltinFunction("strcmp", IR->i32(), {IR->string_t(), IR->string_t()});
            }

            llvm::Value* cmpResult = IR->call(strcmpFunc, {left, right});
            if (!cmpResult) {
                Write("Condition Expression", "Failed to compare strings for operator " + binOpNode->op + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* zero = IR->constI32(0);

            if (binOpNode->op == "==") {
                return IR->eq(cmpResult, zero);
            } else if (binOpNode->op == "!=") {
                return IR->ne(cmpResult, zero);
            } else if (binOpNode->op == "<") {
                return IR->lt(cmpResult, zero);
            } else if (binOpNode->op == "<=") {
                return IR->le(cmpResult, zero);
            } else if (binOpNode->op == ">") {
                return IR->gt(cmpResult, zero);
            } else if (binOpNode->op == ">=") {
                return IR->ge(cmpResult, zero);
            }
        }
        
        if (leftType != rightType) {
            if (leftType->isFloatingPointTy() && rightType->isIntegerTy()) {
                right = IR->getBuilder()->CreateSIToFP(right, leftType, "conv");
            } else if (leftType->isIntegerTy() && rightType->isFloatingPointTy()) {
                left = IR->getBuilder()->CreateSIToFP(left, rightType, "conv");
            }
        }

        if (left->getType()->isFloatingPointTy()) {
            if (binOpNode->op == "==") {
                return IR->eq(left, right);
            } else if (binOpNode->op == "!=") {
                return IR->ne(left, right);
            } else if (binOpNode->op == "<") {
                return IR->lt(left, right);
            } else if (binOpNode->op == "<=") {
                return IR->le(left, right);
            } else if (binOpNode->op == ">") {
                return IR->gt(left, right);
            } else if (binOpNode->op == ">=") {
                return IR->ge(left, right);
            }
        } else {
            if (binOpNode->op == "==") {
                return IR->eq(left, right);
            } else if (binOpNode->op == "!=") {
                return IR->ne(left, right);
            } else if (binOpNode->op == "<") {
                return IR->lt(left, right);
            } else if (binOpNode->op == "<=") {
                return IR->le(left, right);
            } else if (binOpNode->op == ">") {
                return IR->gt(left, right);
            } else if (binOpNode->op == ">=") {
                return IR->ge(left, right);
            }
        }
    }

    llvm::Value* exprValue = GenerateExpression(expr, IR, Methods);
    if (exprValue) {
        if (!exprValue->getType()->isIntegerTy(1)) {
            if (exprValue->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(exprValue->getType(), 0.0);
                return IR->getBuilder()->CreateFCmpONE(exprValue, zero, "tobool");
            } else if (exprValue->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(exprValue->getType(), 0);
                return IR->getBuilder()->CreateICmpNE(exprValue, zero, "tobool");
            } else {
                Write("Condition Expression", "Unsupported type for boolean conversion" + Location, 2, true, true, "");
                return nullptr;
            }
        }
        return exprValue;
    }

    Write("Condition Expression", "Failed to generate expression" + Location, 2, true, true, "");
    return nullptr;
}