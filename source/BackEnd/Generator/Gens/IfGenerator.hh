#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateIf(IfNode* Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap);