#pragma once

#include "../LLVMHeader.hh"

std::string GetStringFromLLVMType(llvm::Type* type);
llvm::Type* GetLLVMTypeFromString(const std::string& typeName, llvm::LLVMContext& context);