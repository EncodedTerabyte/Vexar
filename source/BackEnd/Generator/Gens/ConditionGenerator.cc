#include "ConditionGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* ResolveIdentifier(const std::string& name, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    auto it = AllocaMap.find(name);
    if (it != AllocaMap.end()) {
        llvm::AllocaInst* AllocaInst = it->second;
        return Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, name.c_str());
    }
    return nullptr;
}

llvm::Value* GenerateCondition(ConditionNode* Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    if (!Node->expression) return nullptr;
    return GenerateConditionExpression(Node->expression, Builder, AllocaMap);
}

llvm::Value* GenerateConditionExpression(const std::unique_ptr<ASTNode>& expr, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    if (!expr) return nullptr;
    
    if (expr->type == NodeType::Boolean) {
        auto* boolNode = static_cast<BooleanNode*>(expr.get());
        return llvm::ConstantInt::get(Builder.getInt1Ty(), boolNode->value ? 1 : 0);
    }

    if (expr->type == NodeType::Paren) {
        auto* parenNode = static_cast<ParenNode*>(expr.get());
        return GenerateConditionExpression(parenNode->inner, Builder, AllocaMap);
    }

    if (expr->type == NodeType::BinaryOp) {
        auto* binOpNode = static_cast<BinaryOpNode*>(expr.get());

        if (binOpNode->op == "&&") {
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, Builder, AllocaMap);
            if (!left) return nullptr;

            llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(Builder.getContext(), "land.rhs", currentFunc);
            llvm::BasicBlock* endBB = llvm::BasicBlock::Create(Builder.getContext(), "land.end", currentFunc);
            
            Builder.CreateCondBr(left, rhsBB, endBB);
            
            Builder.SetInsertPoint(rhsBB);
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, Builder, AllocaMap);
            if (!right) return nullptr;
            Builder.CreateBr(endBB);
            rhsBB = Builder.GetInsertBlock();
            
            Builder.SetInsertPoint(endBB);
            llvm::PHINode* phi = Builder.CreatePHI(Builder.getInt1Ty(), 2, "land");
            phi->addIncoming(llvm::ConstantInt::getFalse(Builder.getContext()), Builder.GetInsertBlock()->getSinglePredecessor());
            phi->addIncoming(right, rhsBB);
            
            return phi; // Return i1 directly, no extension
        }
        
        if (binOpNode->op == "||") {
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, Builder, AllocaMap);
            if (!left) return nullptr;

            llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(Builder.getContext(), "lor.rhs", currentFunc);
            llvm::BasicBlock* endBB = llvm::BasicBlock::Create(Builder.getContext(), "lor.end", currentFunc);
            
            Builder.CreateCondBr(left, endBB, rhsBB);
            
            Builder.SetInsertPoint(rhsBB);
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, Builder, AllocaMap);
            if (!right) return nullptr;
            Builder.CreateBr(endBB);
            rhsBB = Builder.GetInsertBlock();
            
            Builder.SetInsertPoint(endBB);
            llvm::PHINode* phi = Builder.CreatePHI(Builder.getInt1Ty(), 2, "lor");
            phi->addIncoming(llvm::ConstantInt::getTrue(Builder.getContext()), Builder.GetInsertBlock()->getSinglePredecessor());
            phi->addIncoming(right, rhsBB);
            
            return phi; // Return i1 directly, no extension
        }

        llvm::Value* left = GenerateExpression(binOpNode->left, Builder, AllocaMap);
        llvm::Value* right = GenerateExpression(binOpNode->right, Builder, AllocaMap);
        
        if (!left || !right) return nullptr;

        llvm::Type* leftType = left->getType();
        llvm::Type* rightType = right->getType();
        
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

    llvm::Value* exprValue = GenerateExpression(expr, Builder, AllocaMap);
    if (exprValue) {
        if (!exprValue->getType()->isIntegerTy(1)) {
            if (exprValue->getType()->isFloatingPointTy()) {
                llvm::Value* zero = llvm::ConstantFP::get(exprValue->getType(), 0.0);
                return Builder.CreateFCmpONE(exprValue, zero, "tobool");
            } else if (exprValue->getType()->isIntegerTy()) {
                llvm::Value* zero = llvm::ConstantInt::get(exprValue->getType(), 0);
                return Builder.CreateICmpNE(exprValue, zero, "tobool");
            }
        }
        return exprValue;
    }

    return nullptr;
}