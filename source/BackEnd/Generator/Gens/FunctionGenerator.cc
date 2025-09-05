#include "FunctionGenerator.hh"
#include "BlockGenerator.hh"
#include "../Helper/Types.hh"

llvm::Function* GenerateFunction(FunctionNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Function Generation", "Null FunctionNode provided", 2, true, true, "");
        return nullptr;
    }

    std::string Name = Node->name;
    std::string ReturnTypeStr = Node->returnType;
    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Type* ReturnType = nullptr;
    if (ReturnTypeStr.find("[]") != std::string::npos) {
        std::string baseType = ReturnTypeStr.substr(0, ReturnTypeStr.find("[]"));
        llvm::Type* elementType = GetAeroTypeFromString(baseType, IR);
        if (!elementType) {
            Write("Function Generation", "Unknown return array element type: " + baseType + Location, 2, true, true, "");
            return nullptr;
        }
        ReturnType = IR->ptr(elementType);
    } else {
        ReturnType = GetAeroTypeFromString(ReturnTypeStr, IR);
    }
    
    if (!ReturnType) {
        Write("Function Generation", "Unknown return type: " + ReturnTypeStr + Location, 2, true, true, "");
        return nullptr;
    }

    std::vector<llvm::Type*> ArgTypes;
    for (const auto& Arg : Node->params) {
        std::string paramType = std::get<1>(Arg);
        
        llvm::Type* ArgType = nullptr;
        if (paramType.find("[]") != std::string::npos) {
            std::string baseType = paramType.substr(0, paramType.find("[]"));
            llvm::Type* elementType = GetAeroTypeFromString(baseType, IR);
            if (!elementType) {
                Write("Function Generation", "Unknown array element type: " + baseType + " for function: " + Name + Location, 2, true, true, "");
                return nullptr;
            }
            ArgType = IR->ptr(elementType);
        } else {
            ArgType = GetAeroTypeFromString(paramType, IR);
        }
        
        if (!ArgType) {
            Write("Function Generation", "Unknown argument type: " + paramType + " for function: " + Name + Location, 2, true, true, "");
            return nullptr;
        }
        ArgTypes.push_back(ArgType);
    }

    llvm::Function* Function = IR->createFunction(Name, ReturnType, ArgTypes);
    if (!Function) {
        Write("Function Generation", "Failed to create function: " + Name + Location, 2, true, true, "");
        return nullptr;
    }

    Methods[Name] = Function;

    Function->removeFnAttr(llvm::Attribute::AlwaysInline);
    Function->removeFnAttr(llvm::Attribute::InlineHint);
    Function->removeFnAttr(llvm::Attribute::NoInline);

    if (Node->alwaysInline) {
        Function->addFnAttr(llvm::Attribute::AlwaysInline);
    } else if (Node->isInlined) {
        Function->addFnAttr(llvm::Attribute::InlineHint);
    } else {
        Function->addFnAttr(llvm::Attribute::NoInline);
    }

    int paramIndex = 0;
    for (const auto& Arg : Node->params) {
        std::string paramName = std::get<0>(Arg);
        std::string paramType = std::get<1>(Arg);
        
        llvm::Value* param = IR->param(paramIndex);
        llvm::Value* alloca = IR->var(paramName, param->getType(), param);
        paramIndex++;
    }

    GenerateBlock(Node->body, IR, Methods);

    llvm::BasicBlock* currentBlock = IR->getBuilder()->GetInsertBlock();
    if (!currentBlock->getTerminator()) {
        if (ReturnType->isVoidTy()) {
            IR->ret(nullptr);
        } else if (ReturnType->isPointerTy()) {
            llvm::Value* DefaultValue = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ReturnType));
            IR->ret(DefaultValue);
        } else {
            llvm::Value* DefaultValue;
            if (ReturnType->isIntegerTy(1)) {
                DefaultValue = IR->constBool(false);
            } else if (ReturnType->isIntegerTy(32)) {
                DefaultValue = IR->constI32(0);
            } else if (ReturnType->isIntegerTy(64)) {
                DefaultValue = IR->constI64(0);
            } else if (ReturnType->isIntegerTy()) {
                DefaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isFloatTy()) {
                DefaultValue = IR->constF32(0.0f);
            } else if (ReturnType->isDoubleTy()) {
                DefaultValue = IR->constF64(0.0);
            } else if (ReturnType->isFloatingPointTy()) {
                DefaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
            } else {
                DefaultValue = llvm::UndefValue::get(ReturnType);
                Write("Function Generation", "Unsupported return type for default value in function: " + Name + Location, 1, true, true, "");
            }
            IR->ret(DefaultValue);
        }
    }

    IR->endFunction();

    return Function;
}