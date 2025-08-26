#include "Malloc.hh"

llvm::Function* Malloc(llvm::Module* module) {
    llvm::Function* mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) {
        llvm::FunctionType* mallocType = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(module->getContext()), 0),
            {llvm::Type::getInt64Ty(module->getContext())},
            false
        );
        mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", module);
    }
    return mallocFunc;
}