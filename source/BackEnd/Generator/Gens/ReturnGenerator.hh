#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateReturn(const ReturnNode* Ret, AeroIR* IR, FunctionSymbols& Methods);