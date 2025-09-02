#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ArrayHandler.hh"
#include <iostream>
#include <cmath>

void GenerateVariable(VariableNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Variable Generation", "Null VariableNode provided", 2, true, true, "");
        return;
    }

    ArrayHandler arrayHandler(Builder);
    
    std::string Name = Node->name;
    std::string Type = Node->varType.name;
    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Type* BaseType = nullptr;
    bool isArrayFromLiteral = false;
    bool useArrayHandler = false;
    
    if (Type == "auto" || Type.empty()) {
        if (!Node->value) {
            Write("Variable Generation", "Auto variable requires initialization: " + Name + Location, 2, true, true, "");
            return;
        }
        
        if (Node->value->type == NodeType::Array) {
            BaseType = Builder.getInt32Ty();
            isArrayFromLiteral = true;
            useArrayHandler = true;
        } else {
            llvm::Value* tempValue = GenerateExpression(Node->value, Builder, AllocaMap, Methods);
            if (!tempValue) {
                Write("Variable Generation", "Invalid expression for auto variable: " + Name + Location, 2, true, true, "");
                return;
            }
            
            if (tempValue->getType()->isPointerTy()) {
                BaseType = tempValue->getType();
            } else if (tempValue->getType()->isIntegerTy(8)) {
                BaseType = Builder.getInt8Ty();
            } else if (tempValue->getType()->isIntegerTy()) {
                BaseType = Builder.getInt32Ty();
            } else if (tempValue->getType()->isFloatingPointTy()) {
                llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(tempValue);
                if (constFP) {
                    double val = constFP->getValueAPF().convertToDouble();
                    if (val == floor(val)) {
                        BaseType = Builder.getInt32Ty();
                    } else {
                        BaseType = Builder.getDoubleTy();
                    }
                } else {
                    BaseType = Builder.getDoubleTy();
                }
            } else {
                Write("Variable Generation", "Unsupported type for auto variable: " + Name + Location, 2, true, true, "");
                return;
            }
        }
    } else {
        std::string baseTypeName = Type;
        if (Type.find("[]") != std::string::npos) {
            baseTypeName = Type.substr(0, Type.find("[]"));
            useArrayHandler = true;
            if (Node->value && Node->value->type == NodeType::Array) {
                isArrayFromLiteral = true;
            }
        }
        
        if (baseTypeName == "int") {
            BaseType = Builder.getInt32Ty();
        } else if (baseTypeName == "uint") {
            BaseType = Builder.getInt32Ty();
        } else if (baseTypeName == "float") {
            BaseType = Builder.getFloatTy();
        } else if (baseTypeName == "char") {
            BaseType = Builder.getInt8Ty();
        } else if (baseTypeName == "bool") {
            BaseType = Builder.getInt1Ty();
        } else if (baseTypeName == "string") {
            BaseType = Builder.getInt8Ty();
            useArrayHandler = true;
        } else {
            BaseType = GetLLVMTypeFromString(baseTypeName, Builder.getContext());
            if (!BaseType) {
                Write("Variable Generation", "Invalid type specified for variable: " + Name + Location, 2, true, true, "");
                return;
            }
        }
    }

    if (useArrayHandler && isArrayFromLiteral && Node->value) {
        ArrayNode* arrayLiteral = static_cast<ArrayNode*>(Node->value.get());
        if (!arrayLiteral) {
            Write("Variable Generation", "Failed to cast to ArrayNode: " + Name + Location, 2, true, true, "");
            return;
        }
        
        ArrayInfo arrayInfo = arrayHandler.CreateArrayFromLiteral(arrayLiteral, BaseType, Name, AllocaMap, Methods);
        
        if (!arrayInfo.dataPtr || !arrayInfo.size) {
            Write("Variable Generation", "Failed to create array from literal: " + Name + Location, 2, true, true, "");
            return;
        }
        
        llvm::Type* ptrType = llvm::PointerType::get(BaseType, 0);
        llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(ptrType, nullptr, Name + "_ptr");
        
        if (!AllocaInst) {
            Write("Variable Generation", "Failed to create alloca for array pointer: " + Name + Location, 2, true, true, "");
            return;
        }
        
        Builder.CreateStore(arrayInfo.dataPtr, AllocaInst);
        AllocaMap.back()[Name] = AllocaInst;
        return;
    }

    if (useArrayHandler && Node->arrayExpression) {
        llvm::Value* sizeValue = GenerateExpression(Node->arrayExpression, Builder, AllocaMap, Methods);
        if (!sizeValue || !sizeValue->getType()->isIntegerTy()) {
            Write("Variable Generation", "Invalid array size for variable: " + Name + Location, 2, true, true, "");
            return;
        }
        
        ArrayInfo arrayInfo = arrayHandler.CreateArrayWithSize(BaseType, sizeValue, Name);
        
        if (!arrayInfo.dataPtr || !arrayInfo.size) {
            Write("Variable Generation", "Failed to create sized array: " + Name + Location, 2, true, true, "");
            return;
        }
        
        llvm::Type* ptrType = llvm::PointerType::get(BaseType, 0);
        llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(ptrType, nullptr, Name + "_ptr");
        
        if (!AllocaInst) {
            Write("Variable Generation", "Failed to create alloca for sized array pointer: " + Name + Location, 2, true, true, "");
            return;
        }
        
        Builder.CreateStore(arrayInfo.dataPtr, AllocaInst);
        AllocaMap.back()[Name] = AllocaInst;
        return;
    }
    
    llvm::Type* FinalType = BaseType;
    
    if (Type.find('[') != std::string::npos && !isArrayFromLiteral && !useArrayHandler) {
        FinalType = GetLLVMTypeFromStringWithArrays(Type, Builder.getContext());
        if (!FinalType) {
            Write("Variable Generation", "Invalid array type for variable: " + Name + Location, 2, true, true, "");
            return;
        }
    }

    llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(FinalType, nullptr, Name);
    if (!AllocaInst) {
        Write("Variable Generation", "Failed to create alloca for variable: " + Name + Location, 2, true, true, "");
        return;
    }

    if (Node->value && !isArrayFromLiteral) {
        llvm::Value* Value = GenerateExpression(Node->value, Builder, AllocaMap, Methods);
        if (!Value) {
            Write("Variable Generation", "Invalid expression for variable: " + Name + Location, 2, true, true, "");
            return;
        }

        if (Value->getType() != FinalType) {
            if (FinalType->isIntegerTy(8) && Value->getType()->isIntegerTy()) {
                Value = Builder.CreateTrunc(Value, FinalType);
            } else if (FinalType->isIntegerTy(32) && Value->getType()->isIntegerTy(8)) {
                Value = Builder.CreateZExt(Value, FinalType);
            } else if (FinalType->isIntegerTy(32) && Value->getType()->isFloatingPointTy()) {
                Value = Builder.CreateFPToSI(Value, FinalType);
            } else if (FinalType->isIntegerTy(1) && Value->getType()->isIntegerTy()) {
                Value = Builder.CreateTrunc(Value, FinalType);
            } else if (FinalType->isIntegerTy() && Value->getType()->isIntegerTy(1)) {
                Value = Builder.CreateZExt(Value, FinalType);
            } else if (FinalType->isFloatTy()) {
                if (Value->getType()->isIntegerTy()) {
                    Value = Builder.CreateSIToFP(Value, FinalType);
                } else if (Value->getType()->isDoubleTy()) {
                    Value = Builder.CreateFPTrunc(Value, FinalType);
                }
            } else if (FinalType->isDoubleTy()) {
                if (Value->getType()->isIntegerTy()) {
                    Value = Builder.CreateSIToFP(Value, FinalType);
                } else if (Value->getType()->isFloatTy()) {
                    Value = Builder.CreateFPExt(Value, FinalType);
                }
            } else if (FinalType->isPointerTy() && Value->getType()->isPointerTy()) {
                if (FinalType != Value->getType()) {
                    Write("Variable Generation", "Pointer type mismatch for variable: " + Name + Location, 2, true, true, "");
                    return;
                }
            } else {
                Write("Variable Generation", "Type mismatch for variable: " + Name + Location, 2, true, true, "");
                return;
            }
        }

        Builder.CreateStore(Value, AllocaInst);
    }

    AllocaMap.back()[Name] = AllocaInst;
}