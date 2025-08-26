#include "ConditionGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* ResolveIdentifier(const std::string& name, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack) {
    llvm::AllocaInst* AllocaInst = FindInScopes(SymbolStack, name);
    if (AllocaInst) {
        return Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, name.c_str());
    }
    return nullptr;
}

llvm::Value* GenerateCondition(ConditionNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Node->expression) return nullptr;
    return GenerateConditionExpression(Node->expression, Builder, SymbolStack, Methods);
}

llvm::Value* GenerateConditionExpression(const std::unique_ptr<ASTNode>& expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!expr) return nullptr;
    
    if (expr->type == NodeType::Boolean) {
        auto* boolNode = static_cast<BooleanNode*>(expr.get());
        return llvm::ConstantInt::get(Builder.getInt1Ty(), boolNode->value ? 1 : 0);
    }

    if (expr->type == NodeType::Paren) {
        auto* parenNode = static_cast<ParenNode*>(expr.get());
        return GenerateConditionExpression(parenNode->inner, Builder, SymbolStack, Methods);
    }

    if (expr->type == NodeType::BinaryOp) {
        auto* binOpNode = static_cast<BinaryOpNode*>(expr.get());

        if (binOpNode->op == "&&") {
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, Builder, SymbolStack, Methods);
            if (!left) return nullptr;

            llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* leftBB = Builder.GetInsertBlock();
            llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(Builder.getContext(), "land.rhs", currentFunc);
            llvm::BasicBlock* endBB = llvm::BasicBlock::Create(Builder.getContext(), "land.end", currentFunc);
            
            Builder.CreateCondBr(left, rhsBB, endBB);
            
            Builder.SetInsertPoint(rhsBB);
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, Builder, SymbolStack, Methods);
            if (!right) return nullptr;
            Builder.CreateBr(endBB);
            rhsBB = Builder.GetInsertBlock();
            
            Builder.SetInsertPoint(endBB);
            llvm::PHINode* phi = Builder.CreatePHI(Builder.getInt1Ty(), 2, "land");
            phi->addIncoming(llvm::ConstantInt::getFalse(Builder.getContext()), leftBB);
            phi->addIncoming(right, rhsBB);
            
            return phi;
        }
        
        if (binOpNode->op == "||") {
            llvm::Value* left = GenerateConditionExpression(binOpNode->left, Builder, SymbolStack, Methods);
            if (!left) return nullptr;

            llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* leftBB = Builder.GetInsertBlock();
            llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(Builder.getContext(), "lor.rhs", currentFunc);
            llvm::BasicBlock* endBB = llvm::BasicBlock::Create(Builder.getContext(), "lor.end", currentFunc);
            
            Builder.CreateCondBr(left, endBB, rhsBB);
            
            Builder.SetInsertPoint(rhsBB);
            llvm::Value* right = GenerateConditionExpression(binOpNode->right, Builder, SymbolStack, Methods);
            if (!right) return nullptr;
            Builder.CreateBr(endBB);
            rhsBB = Builder.GetInsertBlock();
            
            Builder.SetInsertPoint(endBB);
            llvm::PHINode* phi = Builder.CreatePHI(Builder.getInt1Ty(), 2, "lor");
            phi->addIncoming(llvm::ConstantInt::getTrue(Builder.getContext()), leftBB);
            phi->addIncoming(right, rhsBB);
            
            return phi;
        }

        llvm::Value* left = GenerateExpression(binOpNode->left, Builder, SymbolStack, Methods);
        llvm::Value* right = GenerateExpression(binOpNode->right, Builder, SymbolStack, Methods);
        
        if (!left || !right) return nullptr;

        llvm::Type* leftType = left->getType();
        llvm::Type* rightType = right->getType();

        if (leftType->isPointerTy() && rightType->isPointerTy()) {
            llvm::Function* strcmpFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcmp");
            if (!strcmpFunc) {
                llvm::FunctionType* strcmpType = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(Builder.getContext()),
                    {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                    false
                );
                strcmpFunc = llvm::Function::Create(strcmpType, llvm::Function::ExternalLinkage, "strcmp", Builder.GetInsertBlock()->getParent()->getParent());
            }

            llvm::Value* cmpResult = Builder.CreateCall(strcmpFunc, {left, right});
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
            }
        }
        return exprValue;
    }

    return nullptr;
}