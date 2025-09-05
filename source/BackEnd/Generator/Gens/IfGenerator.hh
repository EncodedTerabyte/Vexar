#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateIf(IfNode* Node, AeroIR* IR, FunctionSymbols& Methods);