#pragma once

#include "../LLVMHeader.hh"

#define FunctionSymbols std::unordered_map<std::string, llvm::Function*> 

std::string GetStringFromLLVMType(llvm::Type* type);
llvm::Type* GetLLVMTypeFromString(const std::string& typeName, llvm::LLVMContext& context);
llvm::Type* GetLLVMTypeFromStringWithArrays(const std::string& typeStr, llvm::LLVMContext& ctx);
llvm::Type* GetAeroTypeFromString(const std::string& typeStr, AeroIR* IR);