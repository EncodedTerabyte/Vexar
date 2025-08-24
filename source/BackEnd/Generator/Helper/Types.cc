#include "Types.hh"

llvm::Type* GetLLVMTypeFromString(const std::string& typeName, llvm::LLVMContext& context) {
    if (typeName == "int") {
        return llvm::Type::getInt32Ty(context);
    }
    if (typeName == "float") {
        return llvm::Type::getFloatTy(context);
    }
    if (typeName == "double") {
        return llvm::Type::getDoubleTy(context);
    }
    if (typeName == "bool") {
        return llvm::Type::getInt1Ty(context);
    }
    if (typeName == "void") {
        return llvm::Type::getVoidTy(context);
    }
    if (typeName == "string") {
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    }
    return nullptr;
}

std::string GetStringFromLLVMType(llvm::Type* type) {
    if (type->isIntegerTy(32)) {
        return "int";
    }
    if (type->isFloatTy()) {
        return "float";
    }
    if (type->isDoubleTy()) {
        return "double";
    }
    if (type->isIntegerTy(1)) {
        return "bool";
    }
    if (type->isVoidTy()) {
        return "void";
    }

    return "unknown";
}
