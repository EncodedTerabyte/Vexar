#include "NumberGenerator.hh"

llvm::Value* GenerateNumber(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Expr) {
        Write("NumberGenerator", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);

    auto* NumNode = static_cast<NumberNode*>(Expr.get());
    if (!NumNode) {
        Write("NumberGenerator", "Failed to cast ASTNode to NumberNode" + Location, 2, true, true, "");
        return nullptr;
    }

    try {
        double value = NumNode->value;
        
        if (value == floor(value) && value >= INT32_MIN && value <= INT32_MAX) {
            int32_t intValue = static_cast<int32_t>(value);
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), intValue, true);
        } else {
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(Builder.getContext()), static_cast<float>(value));
        }
    } catch (const std::exception& e) {
        Write("NumberGenerator", "Exception during number generation: " + std::string(e.what()) + Location, 2, true, true, "");
        return nullptr;
    }
}