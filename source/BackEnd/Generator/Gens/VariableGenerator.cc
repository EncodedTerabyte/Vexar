#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"

void GenerateVariable(VariableNode* Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    std::string Name = Node->name;
    std::string Type = Node->varType.name;

    llvm::Value* Value = GenerateExpression(Node->value, Builder, AllocaMap);

    llvm::Type* IdentifiedType;
    if (Type == "auto") {
        IdentifiedType = Value->getType();
    } else {
        IdentifiedType = GetLLVMTypeFromString(Type, Builder.getContext());
    }

    if (GetStringFromLLVMType(IdentifiedType) == "int") {
        if (Value->getType()->isFloatTy()) {
            Value = Builder.CreateFPToSI(Value, IdentifiedType);
        } else if (Value->getType()->isDoubleTy()) {
            Value = Builder.CreateFPToSI(Value, IdentifiedType);
        }
    } else if (GetStringFromLLVMType(IdentifiedType) == "float") {
        if (Value->getType()->isIntegerTy()) {
            Value = Builder.CreateSIToFP(Value, IdentifiedType);
        } else if (Value->getType()->isDoubleTy()) {
            Value = Builder.CreateFPTrunc(Value, IdentifiedType);
        }
    } else if (GetStringFromLLVMType(IdentifiedType) == "double") {
        if (Value->getType()->isIntegerTy()) {
            Value = Builder.CreateSIToFP(Value, IdentifiedType);
        } else if (Value->getType()->isFloatTy()) {
            Value = Builder.CreateFPExt(Value, IdentifiedType);
        }
    }

    llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(IdentifiedType, nullptr, Name);
    Builder.CreateStore(Value, AllocaInst);
    AllocaMap[Name] = AllocaInst;
}