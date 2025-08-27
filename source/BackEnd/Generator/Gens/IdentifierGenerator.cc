#include "IdentifierGenerator.hh"

llvm::Value* GenerateIdentifier(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Expr) {
        Write("Identifier Generation", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    auto* Identifier = static_cast<IdentifierNode*>(Expr.get());
    if (!Identifier) {
        Write("Identifier Generation", "Failed to cast ASTNode to IdentifierNode at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column), 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    llvm::AllocaInst* AllocaInst = FindInScopes(SymbolStack, Identifier->name);
    
    if (AllocaInst) {
        llvm::Value* LoadInst = Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, Identifier->name.c_str());
        if (!LoadInst) {
            Write("Identifier Generation", "Failed to create load instruction for " + Identifier->name + Location, 2, true, true, "");
            return nullptr;
        }
        return LoadInst;
    } else {
        Write("Identifier Generation", "Identifier not found: " + Identifier->name + Location, 2, true, true, "");
        return nullptr;
    }
}