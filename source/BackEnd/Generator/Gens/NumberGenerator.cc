#include "NumberGenerator.hh"

llvm::Value* GenerateNumber(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Expr) {
        Write("NumberGenerator", "Null ASTNode pointer at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column), 2, true, true, "");
        return nullptr;
    }

    auto* NumNode = static_cast<NumberNode*>(Expr.get());
    if (!NumNode) {
        Write("NumberGenerator", "Failed to cast ASTNode to NumberNode at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column), 2, true, true, "");
        return nullptr;
    }

    try {
        if (NumNode->value == floor(NumNode->value)) {
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), (int)NumNode->value);
        } else {
            return llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(NumNode->value));
        }
    } catch (const std::exception& e) {
        Write("NumberGenerator", "Exception during number generation: " + std::string(e.what()) + " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column), 2, true, true, "");
        return nullptr;
    }
}