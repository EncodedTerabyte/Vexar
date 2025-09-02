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
        return llvm::PointerType::get(context, 0);
    }
    if (typeName == "char") {
        return llvm::Type::getInt8Ty(context);
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
    if (type->isPointerTy()) {
        return "string";
    }
    if (type->isIntegerTy(8)) {
        return "char";
    }
    return "unknown";
}

llvm::Type* GetLLVMTypeFromStringWithArrays(const std::string& typeStr, llvm::LLVMContext& ctx) {
    std::string baseTypeName = typeStr;
    int dimensions = 0;
    
    size_t pos = baseTypeName.find('[');
    if (pos != std::string::npos) {
        baseTypeName = baseTypeName.substr(0, pos);
        size_t currentPos = pos;
        while (currentPos < typeStr.length()) {
            size_t openBracket = typeStr.find('[', currentPos);
            size_t closeBracket = typeStr.find(']', currentPos);
            if (openBracket != std::string::npos && closeBracket != std::string::npos) {
                dimensions++;
                currentPos = closeBracket + 1;
            } else {
                break;
            }
        }
    }
    
    llvm::Type* baseType = GetLLVMTypeFromString(baseTypeName, ctx);
    if (!baseType) {
        return nullptr;
    }
    
    if (dimensions > 0) {
        return llvm::PointerType::get(baseType, 0);
    }
    
    return baseType;
}