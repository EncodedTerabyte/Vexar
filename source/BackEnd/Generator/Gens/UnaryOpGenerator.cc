#include "UnaryOpGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateUnaryOp(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    auto* UnaryNode = static_cast<UnaryOpNode*>(Expr.get());
    if (!UnaryNode) {
        return nullptr;
    }
    
    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    llvm::Value* Operand = GenerateExpression(UnaryNode->operand, Builder, SymbolStack, Methods);
    if (!Operand) {
        Write("Expression Generation", "Invalid unary operand" + Location, 2, true, true, "");
        return nullptr;
    }
    
    if (UnaryNode->op == "-") {
        if (Operand->getType()->isFloatingPointTy()) {
            return Builder.CreateFNeg(Operand, "neg");
        } else if (Operand->getType()->isIntegerTy()) {
            return Builder.CreateNeg(Operand, "neg");
        } else {
            Write("Expression Generation", "Invalid type for unary minus" + Location, 2, true, true, "");
            return nullptr;
        }
    } else if (UnaryNode->op == "+") {
        return Operand;
    } else if (UnaryNode->op == "!") {
        if (Operand->getType()->isIntegerTy(1)) {
            return Builder.CreateNot(Operand, "not");
        } else {
            llvm::Value* Zero;
            if (Operand->getType()->isFloatingPointTy()) {
                Zero = llvm::ConstantFP::get(Operand->getType(), 0.0);
                return Builder.CreateFCmpOEQ(Operand, Zero, "not");
            } else if (Operand->getType()->isIntegerTy()) {
                Zero = llvm::ConstantInt::get(Operand->getType(), 0);
                return Builder.CreateICmpEQ(Operand, Zero, "not");
            } else {
                Write("Expression Generation", "Invalid type for logical not" + Location, 2, true, true, "");
                return nullptr;
            }
        }
    }
    
    Write("Expression Generation", "Unsupported unary operator: " + UnaryNode->op + Location, 2, true, true, "");
    return nullptr;
}