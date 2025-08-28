#include "UnaryOpGenerator.hh" 
#include "ExpressionGenerator.hh" 
 
llvm::Value* GenerateUnaryOp(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    auto* UnaryNode = static_cast<UnaryOpNode*>(Expr.get());
    if (!UnaryNode) {
        return nullptr;
    }
    
    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    if (UnaryNode->op == "++") {
        if (!UnaryNode->operand || UnaryNode->operand->type != NodeType::Identifier) {
            Write("Unary Op Generation", "Increment operator requires identifier operand" + Location, 2, true, true, "");
            return nullptr;
        }
        
        auto* IdentNode = static_cast<IdentifierNode*>(UnaryNode->operand.get());
        llvm::AllocaInst* VarAlloca = FindInScopes(SymbolStack, IdentNode->name);
        if (!VarAlloca) {
            Write("Unary Op Generation", "Undefined variable: " + IdentNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* CurrentValue = Builder.CreateLoad(VarAlloca->getAllocatedType(), VarAlloca);
        llvm::Value* One = nullptr;
        
        if (CurrentValue->getType()->isFloatingPointTy()) {
            One = llvm::ConstantFP::get(CurrentValue->getType(), 1.0);
            llvm::Value* NewValue = Builder.CreateFAdd(CurrentValue, One);
            Builder.CreateStore(NewValue, VarAlloca);
            return NewValue;
        } else if (CurrentValue->getType()->isIntegerTy()) {
            One = llvm::ConstantInt::get(CurrentValue->getType(), 1);
            llvm::Value* NewValue = Builder.CreateAdd(CurrentValue, One);
            Builder.CreateStore(NewValue, VarAlloca);
            return NewValue;
        } else {
            Write("Unary Op Generation", "Increment operator not supported for this type" + Location, 2, true, true, "");
            return nullptr;
        }
    }
    
    if (UnaryNode->op == "--") {
        if (!UnaryNode->operand || UnaryNode->operand->type != NodeType::Identifier) {
            Write("Unary Op Generation", "Decrement operator requires identifier operand" + Location, 2, true, true, "");
            return nullptr;
        }
        
        auto* IdentNode = static_cast<IdentifierNode*>(UnaryNode->operand.get());
        llvm::AllocaInst* VarAlloca = FindInScopes(SymbolStack, IdentNode->name);
        if (!VarAlloca) {
            Write("Unary Op Generation", "Undefined variable: " + IdentNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* CurrentValue = Builder.CreateLoad(VarAlloca->getAllocatedType(), VarAlloca);
        llvm::Value* One = nullptr;
        
        if (CurrentValue->getType()->isFloatingPointTy()) {
            One = llvm::ConstantFP::get(CurrentValue->getType(), 1.0);
            llvm::Value* NewValue = Builder.CreateFSub(CurrentValue, One);
            Builder.CreateStore(NewValue, VarAlloca);
            return NewValue;
        } else if (CurrentValue->getType()->isIntegerTy()) {
            One = llvm::ConstantInt::get(CurrentValue->getType(), 1);
            llvm::Value* NewValue = Builder.CreateSub(CurrentValue, One);
            Builder.CreateStore(NewValue, VarAlloca);
            return NewValue;
        } else {
            Write("Unary Op Generation", "Decrement operator not supported for this type" + Location, 2, true, true, "");
            return nullptr;
        }
    }
    
    if (UnaryNode->op == "-") {
        llvm::Value* Operand = GenerateExpression(UnaryNode->operand, Builder, SymbolStack, Methods);
        if (!Operand) {
            Write("Unary Op Generation", "Invalid operand for unary minus" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (Operand->getType()->isFloatingPointTy()) {
            return Builder.CreateFNeg(Operand);
        } else if (Operand->getType()->isIntegerTy()) {
            return Builder.CreateNeg(Operand);
        } else {
            Write("Unary Op Generation", "Unary minus not supported for this type" + Location, 2, true, true, "");
            return nullptr;
        }
    }
    
    if (UnaryNode->op == "!") {
        llvm::Value* Operand = GenerateExpression(UnaryNode->operand, Builder, SymbolStack, Methods);
        if (!Operand) {
            Write("Unary Op Generation", "Invalid operand for logical not" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (Operand->getType()->isIntegerTy(1)) {
            return Builder.CreateNot(Operand);
        } else {
            if (Operand->getType()->isFloatingPointTy()) {
                llvm::Value* Zero = llvm::ConstantFP::get(Operand->getType(), 0.0);
                llvm::Value* IsZero = Builder.CreateFCmpOEQ(Operand, Zero);
                return IsZero;
            } else if (Operand->getType()->isIntegerTy()) {
                llvm::Value* Zero = llvm::ConstantInt::get(Operand->getType(), 0);
                llvm::Value* IsZero = Builder.CreateICmpEQ(Operand, Zero);
                return IsZero;
            } else {
                Write("Unary Op Generation", "Logical not not supported for this type" + Location, 2, true, true, "");
                return nullptr;
            }
        }
    }
    
    Write("Unary Op Generation", "Unsupported unary operator: " + UnaryNode->op + Location, 2, true, true, "");
    return nullptr;
}