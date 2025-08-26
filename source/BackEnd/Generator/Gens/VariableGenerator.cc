#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"
#include <iostream>
#include <cmath>

void GenerateVariable(VariableNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Variable Generation", "Null VariableNode provided", 2, true, true, "");
        return;
    }

    std::string Name = Node->name;
    std::string Type = Node->varType.name;
    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Value* Value = GenerateExpression(Node->value, Builder, AllocaMap, Methods);
    if (!Value) {
        Write("Variable Generation", "Invalid expression for variable: " + Name + Location, 2, true, true, "");
        return;
    }

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
            Write("Variable Generation", "Unsupported type for auto variable: " + Name + Location, 2, true, true, "");
            return;
        }
    } else if (Type == "string") {
        IdentifiedType = llvm::PointerType::get(Builder.getContext(), 0);
    } else {
        IdentifiedType = GetLLVMTypeFromString(Type, Builder.getContext());
        if (!IdentifiedType) {
            Write("Variable Generation", "Invalid type specified for variable: " + Name + Location, 2, true, true, "");
            return;
        }
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
        } else {
            Write("Variable Generation", "Type mismatch for variable: " + Name + Location, 2, true, true, "");
            return;
        }
    }

    llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(IdentifiedType, nullptr, Name);
    if (!AllocaInst) {
        Write("Variable Generation", "Failed to create alloca for variable: " + Name + Location, 2, true, true, "");
        return;
    }

    Builder.CreateStore(Value, AllocaInst);
    AllocaMap.back()[Name] = AllocaInst;
}