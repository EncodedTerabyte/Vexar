#include "ConditionGenerator.hh"
#include "ExpressionGenerator.hh"
#include "LoggerFile.hh"

llvm::Value* ResolveIdentifier(const std::string& name, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack) {
    llvm::AllocaInst* AllocaInst = FindInScopes(SymbolStack, name);
    if (AllocaInst) {
        llvm::Value* Result = Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, name.c_str());
        if (!Result) {
            Write("Condition Generation", "Failed to load identifier: " + name, 2, true, true, "");
            return nullptr;
        }
        return Result;
    }
    Write("Condition Generation", "Identifier not found: " + name, 2, true, true, "");
    return nullptr;
}

llvm::Value* GenerateCondition(ConditionNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Condition Generation", "Null ConditionNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    if (!Node->expression) {
        Write("Condition Generation", "Null expression in ConditionNode" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* Result = GenerateConditionExpression(Node->expression, Builder, SymbolStack, Methods);
    if (!Result) {
        Write("Condition Generation", "Failed to generate condition expression" + Location, 2, true, true, "");
        return nullptr;
    }

    return Result;
}

llvm::Value* GenerateConditionExpression(const std::unique_ptr<ASTNode>& expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
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
        return llvm::ConstantInt::get(Builder.getInt1Ty(), boolNode->value ? 1 : 0);
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
        return GenerateConditionExpression(parenNode->inner, Builder, SymbolStack, Methods);
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
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, Builder, SymbolStack, Methods);
            if (!left) {
                Write("Condition Expression", "Invalid left operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
            if (!currentFunc) {
                Write("Condition Expression", "Invalid current function for &&" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::BasicBlock* leftBB = Builder.GetInsertBlock();
            llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(Builder.getContext(), "land.rhs", currentFunc);
            llvm::BasicBlock* endBB = llvm::BasicBlock::Create(Builder.getContext(), "land.end", currentFunc);
            
            Builder.CreateCondBr(left, rhsBB, endBB);
            
            Builder.SetInsertPoint(rhsBB);
            if (!binOpNode->right) {
                Write("Condition Expression", "Null right operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, Builder, SymbolStack, Methods);
            if (!right) {
                Write("Condition Expression", "Invalid right operand for &&" + Location, 2, true, true, "");
                return nullptr;
            }
            Builder.CreateBr(endBB);
            rhsBB = Builder.GetInsertBlock();
            
            Builder.SetInsertPoint(endBB);
            llvm::PHINode* phi = Builder.CreatePHI(Builder.getInt1Ty(), 2, "land");
            phi->addIncoming(llvm::ConstantInt::getFalse(Builder.getContext()), leftBB);
            phi->addIncoming(right, rhsBB);
            
            return phi;
        }
        
        if (binOpNode->op == "||") {
            if (!binOpNode->left) {
                Write("Condition Expression", "Null left operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, Builder, SymbolStack, Methods);
            if (!left) {
                Write("Condition Expression", "Invalid left operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
            if (!currentFunc) {
                Write("Condition Expression", "Invalid current function for ||" + Location, 2, true, true, "");
                return nullptr;
            }

            llvm::BasicBlock* leftBB = Builder.GetInsertBlock();
            llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(Builder.getContext(), "lor.rhs", currentFunc);
            llvm::BasicBlock* endBB = llvm::BasicBlock::Create(Builder.getContext(), "lor.end", currentFunc);
            
            Builder.CreateCondBr(left, endBB, rhsBB);
            
            Builder.SetInsertPoint(rhsBB);
            if (!binOpNode->right) {
                Write("Condition Expression", "Null right operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, Builder, SymbolStack, Methods);
            if (!right) {
                Write("Condition Expression", "Invalid right operand for ||" + Location, 2, true, true, "");
                return nullptr;
            }
            Builder.CreateBr(endBB);
            rhsBB = Builder.GetInsertBlock();
            
            Builder.SetInsertPoint(endBB);
            llvm::PHINode* phi = Builder.CreatePHI(Builder.getInt1Ty(), 2, "lor");
            phi->addIncoming(llvm::ConstantInt::getTrue(Builder.getContext()), leftBB);
            phi->addIncoming(right, rhsBB);
            
            return phi;
        }

        if (!binOpNode->left || !binOpNode->right) {
            Write("Condition Expression", "Null operand for binary operator " + binOpNode->op + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Value* left = GenerateExpression(binOpNode->left, Builder, SymbolStack, Methods);
        if (!left) {
            Write("Condition Expression", "Invalid left expression for operator " + binOpNode->op + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Value* right = GenerateExpression(binOpNode->right, Builder, SymbolStack, Methods);
        if (!right) {
            Write("Condition Expression", "Invalid right expression for operator " + binOpNode->op + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Type* leftType = left->getType();
        llvm::Type* rightType = right->getType();

        if (leftType->isPointerTy() && rightType->isPointerTy()) {
            llvm::Function* strcmpFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcmp");
            if (!strcmpFunc) {
                llvm::FunctionType* strcmpType = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(Builder.getContext()),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    false
                );
                strcmpFunc = llvm::Function::Create(strcmpType, llvm::Function::ExternalLinkage, "strcmp", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Value* cmpResult = Builder.CreateCall(strcmpFunc, {left, right});
            if (!cmpResult) {
                Write("Condition Expression", "Failed to compare strings for operator " + binOpNode->op + Location, 2, true, true, "");
                return nullptr;
            }
            llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), 0);

            if (binOpNode->op == "==") {
                return Builder.CreateICmpEQ(cmpResult, zero, "streq");
            } else if (binOpNode->op == "!=") {
                return Builder.CreateICmpNE(cmpResult, zero, "strne");
            } else if (binOpNode->op == "<") {
                return Builder.CreateICmpSLT(cmpResult, zero, "strlt");
            } else if (binOpNode->op == "<=") {
                return Builder.CreateICmpSLE(cmpResult, zero, "strle");
            } else if (binOpNode->op == ">") {
                return Builder.CreateICmpSGT(cmpResult, zero, "strgt");
            } else if (binOpNode->op == ">=") {
                return Builder.CreateICmpSGE(cmpResult, zero, "strge");
            }
        }
        
        if (leftType != rightType) {
            if (leftType->isFloatingPointTy() && rightType->isIntegerTy()) {
                right = Builder.CreateSIToFP(right, leftType, "conv");
            } else if (leftType->isIntegerTy() && rightType->isFloatingPointTy()) {
                left = Builder.CreateSIToFP(left, rightType, "conv");
            }
        }

        if (left->getType()->isFloatingPointTy()) {
            if (binOpNode->op == "==") {
                return Builder.CreateFCmpOEQ(left, right, "fcmp_eq");
            } else if (binOpNode->op == "!=") {
                return Builder.CreateFCmpONE(left, right, "fcmp_ne");
            } else if (binOpNode->op == "<") {
                return Builder.CreateFCmpOLT(left, right, "fcmp_lt");
            } else if (binOpNode->op == "<=") {
                return Builder.CreateFCmpOLE(left, right, "fcmp_le");
            } else if (binOpNode->op == ">") {
                return Builder.CreateFCmpOGT(left, right, "fcmp_gt");
            } else if (binOpNode->op == ">=") {
                return Builder.CreateFCmpOGE(left, right, "fcmp_ge");
            }
        } else {
            if (binOpNode->op == "==") {
                return Builder.CreateICmpEQ(left, right, "icmp_eq");
            } else if (binOpNode->op == "!=") {
                return Builder.CreateICmpNE(left, right, "icmp_ne");
            } else if (binOpNode->op == "<") {
                return Builder.CreateICmpSLT(left, right, "icmp_lt");
            } else if (binOpNode->op == "<=") {
                return Builder.CreateICmpSLE(left, right, "icmp_le");
            } else if (binOpNode->op == ">") {
                return Builder.CreateICmpSGT(left, right, "icmp_gt");
            } else if (binOpNode->op == ">=") {
                return Builder.CreateICmpSGE(left, right, "icmp_ge");
            }
        }
    }

    llvm::Value* exprValue = GenerateExpression(expr, Builder, SymbolStack, Methods);
    if (exprValue) {
        if (!exprValue->getType()->isIntegerTy(1)) {
            if (exprValue->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(exprValue->getType(), 0.0);
                return Builder.CreateFCmpONE(exprValue, zero, "tobool");
            } else if (exprValue->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(exprValue->getType(), 0);
                return Builder.CreateICmpNE(exprValue, zero, "tobool");
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