#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateWhile(WhileNode* Node, AeroIR* IR, FunctionSymbols& Methods);