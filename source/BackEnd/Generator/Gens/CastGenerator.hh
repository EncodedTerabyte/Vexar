#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateCast(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods);