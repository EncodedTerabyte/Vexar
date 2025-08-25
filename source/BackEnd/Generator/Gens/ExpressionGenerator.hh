#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods);