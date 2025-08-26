#include "NumberGenerator.hh"

llvm::Value* GenerateNumber(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    auto* NumNode = static_cast<NumberNode*>(Expr.get());
        
    if (NumNode->value == floor(NumNode->value)) {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), (int)NumNode->value);
    } else {
        return llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(NumNode->value));
    }
}