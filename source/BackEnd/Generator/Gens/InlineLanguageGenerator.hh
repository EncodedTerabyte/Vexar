#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateInlineLanguage(InlineCodeNode* ICN, AeroIR* IR, FunctionSymbols& Methods);