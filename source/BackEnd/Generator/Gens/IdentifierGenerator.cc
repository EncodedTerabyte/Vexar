#include "IdentifierGenerator.hh"

llvm::Value* GenerateIdentifier(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    auto* Identifier = static_cast<IdentifierNode*>(Expr.get());
    
    llvm::AllocaInst* AllocaInst = FindInScopes(SymbolStack, Identifier->name);
    
    if (AllocaInst) {
        return Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, Identifier->name.c_str());
    } else {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), 0);
    }
}