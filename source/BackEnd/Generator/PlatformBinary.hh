#include "LLVMHeader.hh"

void CreatePlatformBinary(std::unique_ptr<llvm::Module> Module, std::string Triple, fs::path Output);