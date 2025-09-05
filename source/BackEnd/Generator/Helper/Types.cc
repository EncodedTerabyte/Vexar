#include "Types.hh"

llvm::Type* GetAeroTypeFromString(const std::string& typeStr, AeroIR* IR) {
    if (typeStr == "void") return IR->void_t();
    if (typeStr == "int") return IR->int_t();
    if (typeStr == "float") return IR->float_t();
    if (typeStr == "double") return IR->double_t();
    if (typeStr == "bool") return IR->bool_t();
    if (typeStr == "char") return IR->char_t();
    if (typeStr == "string") return IR->string_t();
    
    if (typeStr.find("[]") != std::string::npos) {
        std::string baseType = typeStr.substr(0, typeStr.find("[]"));
        llvm::Type* elemType = GetAeroTypeFromString(baseType, IR);
        if (elemType) {
            return IR->ptr(elemType);
        }
    }
    
    if (typeStr.find("*") != std::string::npos) {
        std::string baseType = typeStr.substr(0, typeStr.find("*"));
        llvm::Type* elemType = GetAeroTypeFromString(baseType, IR);
        if (elemType) {
            return IR->ptr(elemType);
        }
    }
    
    llvm::Type* customType = IR->getCustomType(typeStr);
    if (customType) {
        return customType;
    }
    
    return nullptr;
}

llvm::Type* GetAeroTypeFromStringWithArrays(const std::string& typeStr, AeroIR* IR) {
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
    
    llvm::Type* baseType = GetAeroTypeFromString(baseTypeName, IR);
    if (!baseType) {
        return nullptr;
    }
    
    if (dimensions > 0) {
        return IR->ptr(baseType);
    }
    
    return baseType;
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

llvm::Type* GetLLVMTypeFromString(const std::string& type, llvm::LLVMContext& context) {
    if (type == "i8" || type == "char") {
        return llvm::Type::getInt8Ty(context);
    } else if (type == "i16" || type == "short") {
        return llvm::Type::getInt16Ty(context);
    } else if (type == "i32" || type == "int") {
        return llvm::Type::getInt32Ty(context);
    } else if (type == "i64" || type == "long") {
        return llvm::Type::getInt64Ty(context);
    } else if (type == "f32" || type == "float") {
        return llvm::Type::getFloatTy(context);
    } else if (type == "f64" || type == "double") {
        return llvm::Type::getDoubleTy(context);
    } else if (type == "bool" || type == "boolean") {
        return llvm::Type::getInt1Ty(context);
    } else if (type == "void") {
        return llvm::Type::getVoidTy(context);
    } else if (type == "string" || type == "str") {
        return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
    }
    
    return nullptr;
}