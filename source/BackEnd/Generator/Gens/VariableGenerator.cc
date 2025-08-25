#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"
#include <iostream>
#include <cmath>

void GenerateVariable(VariableNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    std::string Name = Node->name;
    std::string Type = Node->varType.name;

    llvm::Value* Value = GenerateExpression(Node->value, Builder, AllocaMap, Methods);

    llvm::Type* IdentifiedType;
    if (Type == "auto") {
        if (Value->getType()->isPointerTy()) {
            IdentifiedType = llvm::PointerType::get(Builder.getContext(), 0);
        } else if (Value->getType()->isIntegerTy()) {
            IdentifiedType = Builder.getInt32Ty();
        } else if (Value->getType()->isFloatingPointTy()) {
            if (llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(Value)) {
                double val = constFP->getValueAPF().convertToDouble();
                if (val == floor(val)) {
                    IdentifiedType = Builder.getInt32Ty();
                } else {
                    IdentifiedType = Builder.getDoubleTy();
                }
            } else {
                IdentifiedType = Builder.getDoubleTy();
            }
        } else {
            IdentifiedType = Value->getType();
        }
    } else if (Type == "string") {
        IdentifiedType = llvm::PointerType::get(Builder.getContext(), 0);
    } else {
        IdentifiedType = GetLLVMTypeFromString(Type, Builder.getContext());
    }

    if (IdentifiedType && Value->getType() != IdentifiedType) {
        if (IdentifiedType->isIntegerTy(32) && Value->getType()->isFloatingPointTy()) {
            Value = Builder.CreateFPToSI(Value, IdentifiedType);
        } else if (IdentifiedType->isFloatTy()) {
            if (Value->getType()->isIntegerTy()) {
                Value = Builder.CreateSIToFP(Value, IdentifiedType);
            } else if (Value->getType()->isDoubleTy()) {
                Value = Builder.CreateFPTrunc(Value, IdentifiedType);
            }
        } else if (IdentifiedType->isDoubleTy()) {
            if (Value->getType()->isIntegerTy()) {
                Value = Builder.CreateSIToFP(Value, IdentifiedType);
            } else if (Value->getType()->isFloatTy()) {
                Value = Builder.CreateFPExt(Value, IdentifiedType);
            }
        }
    }

    llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(IdentifiedType, nullptr, Name);
    Builder.CreateStore(Value, AllocaInst);
    AllocaMap.back()[Name] = AllocaInst;
}