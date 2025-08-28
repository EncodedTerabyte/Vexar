#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"
#include <iostream>
#include <cmath>

std::unique_ptr<ASTNode> WrapASTNode(ASTNode* node) {
    return std::unique_ptr<ASTNode>(node);
}

llvm::Type* InferArrayTypeFromLiteral(ArrayNode* arrayLiteral, llvm::Type* baseType, llvm::IRBuilder<>& Builder) {
    if (!arrayLiteral || arrayLiteral->elements.empty()) {
        return nullptr;
    }
    
    uint64_t size = arrayLiteral->elements.size();
    
    if (arrayLiteral->elements[0]->type == NodeType::Array) {
        ArrayNode* subArray = static_cast<ArrayNode*>(arrayLiteral->elements[0].get());
        llvm::Type* subType = InferArrayTypeFromLiteral(subArray, baseType, Builder);
        if (!subType) return nullptr;
        return llvm::ArrayType::get(subType, size);
    } else {
        return llvm::ArrayType::get(baseType, size);
    }
}

llvm::Type* GetArrayTypeFromDimensions(ASTNode* dimensionNode, llvm::Type* baseType, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!dimensionNode) return baseType;
    
    if (dimensionNode->type == NodeType::Array) {
        ArrayNode* arrayDimNode = static_cast<ArrayNode*>(dimensionNode);
        llvm::Type* currentType = baseType;
        
        for (int i = arrayDimNode->elements.size() - 1; i >= 0; i--) {
            llvm::Value* dimValue = GenerateExpression(arrayDimNode->elements[i], Builder, AllocaMap, Methods);
            if (!dimValue || !dimValue->getType()->isIntegerTy()) {
                return nullptr;
            }
            
            if (llvm::ConstantInt* constDim = llvm::dyn_cast<llvm::ConstantInt>(dimValue)) {
                uint64_t size = constDim->getZExtValue();
                currentType = llvm::ArrayType::get(currentType, size);
            } else {
                return nullptr;
            }
        }
        return currentType;
    } else {
        auto tempNode = std::unique_ptr<ASTNode>();
        tempNode.reset(dimensionNode);
        llvm::Value* dimValue = GenerateExpression(tempNode, Builder, AllocaMap, Methods);
        tempNode.release();
        
        if (!dimValue || !dimValue->getType()->isIntegerTy()) {
            return nullptr;
        }

        if (llvm::ConstantInt* constDim = llvm::dyn_cast<llvm::ConstantInt>(dimValue)) {
            uint64_t size = constDim->getZExtValue();
            return llvm::ArrayType::get(baseType, size);
        } else {
            return nullptr;
        }
    }
}

void InitializeArrayFromLiteral(llvm::AllocaInst* arrayAlloca, ArrayNode* arrayLiteral, llvm::Type* elementType, 
                               llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods,
                               std::vector<llvm::Value*>& indices) {
    
    if (!arrayLiteral) {
        Write("Variable Generation", "Null array literal in initialization", 2, true, true, "");
        return;
    }
    
    for (size_t i = 0; i < arrayLiteral->elements.size(); i++) {
        if (!arrayLiteral->elements[i]) {
            Write("Variable Generation", "Null element at index " + std::to_string(i), 2, true, true, "");
            continue;
        }
        
        ASTNode* element = arrayLiteral->elements[i].get();
        if (!element) {
            Write("Variable Generation", "Null element pointer at index " + std::to_string(i), 2, true, true, "");
            continue;
        }
        
        if (element->type == NodeType::Array) {
            ArrayNode* subArray = static_cast<ArrayNode*>(element);
            std::vector<llvm::Value*> newIndices = indices;
            newIndices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), i));
            InitializeArrayFromLiteral(arrayAlloca, subArray, elementType, Builder, AllocaMap, Methods, newIndices);
        } else {
            llvm::Value* elementValue = nullptr;
            if (element->type == NodeType::Number) {
                auto* numNode = static_cast<NumberNode*>(element);
                elementValue = llvm::ConstantInt::get(elementType, static_cast<int>(numNode->value));
            } else {
                elementValue = GenerateExpression(arrayLiteral->elements[i], Builder, AllocaMap, Methods);
                if (!elementValue) continue;
            }
            
            if (elementValue->getType() != elementType) {
                if (elementType->isIntegerTy(32) && elementValue->getType()->isFloatingPointTy()) {
                    elementValue = Builder.CreateFPToSI(elementValue, elementType);
                } else if (elementType->isFloatTy()) {
                    if (elementValue->getType()->isIntegerTy()) {
                        elementValue = Builder.CreateSIToFP(elementValue, elementType);
                    } else if (elementValue->getType()->isDoubleTy()) {
                        elementValue = Builder.CreateFPTrunc(elementValue, elementType);
                    }
                } else if (elementType->isDoubleTy()) {
                    if (elementValue->getType()->isIntegerTy()) {
                        elementValue = Builder.CreateSIToFP(elementValue, elementType);
                    } else if (elementValue->getType()->isFloatTy()) {
                        elementValue = Builder.CreateFPExt(elementValue, elementType);
                    }
                }
            }
            
            std::vector<llvm::Value*> gepIndices;
            gepIndices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
            for (llvm::Value* idx : indices) {
                gepIndices.push_back(idx);
            }
            gepIndices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), i));
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(arrayAlloca->getAllocatedType(), arrayAlloca, gepIndices);
            Builder.CreateStore(elementValue, elementPtr);
        }
    }
}

void GenerateVariable(VariableNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Variable Generation", "Null VariableNode provided", 2, true, true, "");
        return;
    }

    std::string Name = Node->name;
    std::string Type = Node->varType.name;
    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Type* BaseType = nullptr;
    bool isArrayFromLiteral = false;
    bool isPointerArray = false;
    
    if (Type == "auto" || Type.empty()) {
        if (!Node->value) {
            Write("Variable Generation", "Auto variable requires initialization: " + Name + Location, 2, true, true, "");
            return;
        }
        
        if (Node->value->type == NodeType::Array) {
            BaseType = Builder.getInt32Ty();
            isArrayFromLiteral = true;
        } else {
            llvm::Value* tempValue = GenerateExpression(Node->value, Builder, AllocaMap, Methods);
            if (!tempValue) {
                Write("Variable Generation", "Invalid expression for auto variable: " + Name + Location, 2, true, true, "");
                return;
            }
            
            if (tempValue->getType()->isPointerTy()) {
                BaseType = tempValue->getType();
                isPointerArray = true;
            } else if (tempValue->getType()->isIntegerTy(8)) {
                BaseType = Builder.getInt8Ty();
            } else if (tempValue->getType()->isIntegerTy()) {
                BaseType = Builder.getInt32Ty();
            } else if (tempValue->getType()->isFloatingPointTy()) {
                if (llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(tempValue)) {
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
        size_t pos = baseTypeName.find('[');
        if (pos != std::string::npos) {
            baseTypeName = baseTypeName.substr(0, pos);
            
            size_t currentPos = pos;
            int dimensions = 0;
            while (currentPos < Type.length()) {
                size_t openBracket = Type.find('[', currentPos);
                size_t closeBracket = Type.find(']', currentPos);
                if (openBracket != std::string::npos && closeBracket != std::string::npos) {
                    dimensions++;
                    currentPos = closeBracket + 1;
                } else {
                    break;
                }
            }
            
            if (dimensions > 0 && !Node->value) {
                isPointerArray = true;
            }
        }
        
        if (baseTypeName == "string") {
            BaseType = llvm::PointerType::get(Builder.getContext(), 0);
        } else if (baseTypeName == "bool") {
            BaseType = Builder.getInt1Ty();
        } else {
            BaseType = GetLLVMTypeFromString(baseTypeName, Builder.getContext());
            if (!BaseType) {
                Write("Variable Generation", "Invalid type specified for variable: " + Name + Location, 2, true, true, "");
                return;
            }
        }
        
        if (Node->value && Node->value->type == NodeType::Array && !Node->arrayExpression) {
            isArrayFromLiteral = true;
        }
    }

    llvm::Type* FinalType = BaseType;
    
    if (Type.find('[') != std::string::npos && !isArrayFromLiteral) {
        FinalType = GetLLVMTypeFromStringWithArrays(Type, Builder.getContext());
        if (!FinalType) {
            Write("Variable Generation", "Invalid array type for variable: " + Name + Location, 2, true, true, "");
            return;
        }
        isPointerArray = true;
    } else if (Node->arrayExpression) {
        FinalType = GetArrayTypeFromDimensions(Node->arrayExpression.get(), BaseType, Builder, AllocaMap, Methods);
        if (!FinalType) {
            Write("Variable Generation", "Invalid array dimensions for variable: " + Name + Location, 2, true, true, "");
            return;
        }
    } else if (isArrayFromLiteral) {
        ArrayNode* arrayLiteral = static_cast<ArrayNode*>(Node->value.get());
        FinalType = InferArrayTypeFromLiteral(arrayLiteral, BaseType, Builder);
        if (!FinalType) {
            Write("Variable Generation", "Failed to infer array type from literal: " + Name + Location, 2, true, true, "");
            return;
        }
    }

    llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(FinalType, nullptr, Name);
    if (!AllocaInst) {
        Write("Variable Generation", "Failed to create alloca for variable: " + Name + Location, 2, true, true, "");
        return;
    }

    if (Node->value) {
        if (isArrayFromLiteral && !isPointerArray) {
            if (Node->value->type == NodeType::Array) {
                ArrayNode* arrayLiteral = static_cast<ArrayNode*>(Node->value.get());
                std::vector<llvm::Value*> indices;
                InitializeArrayFromLiteral(AllocaInst, arrayLiteral, BaseType, Builder, AllocaMap, Methods, indices);
            } else {
                Write("Variable Generation", "Array variable requires array literal initialization: " + Name + Location, 2, true, true, "");
                return;
            }
        } else {
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
                    if (FinalType == Value->getType()) {
                    } else {
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
    }

    AllocaMap.back()[Name] = AllocaInst;
}