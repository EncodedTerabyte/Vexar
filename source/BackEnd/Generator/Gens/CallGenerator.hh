#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

#include "DefaultSymbols.hh"

llvm::Value* GenerateCall(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods, BuiltinSymbols& BuiltIns);