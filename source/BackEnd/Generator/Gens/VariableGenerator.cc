
#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"
#include <iostream>
#include <cmath>

llvm::Type* DeduceArrayElementType(ArrayNode* arrayLiteral, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!arrayLiteral || arrayLiteral->elements.empty()) {
        return nullptr;
    }
    
    ASTNode* firstElement = arrayLiteral->elements[0].get();
    if (firstElement->type == NodeType::Array) {
        return DeduceArrayElementType(static_cast<ArrayNode*>(firstElement), Builder, AllocaMap, Methods);
    }
    
    llvm::Value* firstValue = GenerateExpression(arrayLiteral->elements[0], Builder, AllocaMap, Methods);
    if (!firstValue) return nullptr;
    
    if (firstValue->getType()->isIntegerTy()) {
        return Builder.getInt32Ty();
    } else if (firstValue->getType()->isFloatingPointTy()) {
        if (llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(firstValue)) {
            double val = constFP->getValueAPF().convertToDouble();
            if (val == floor(val)) {
                return Builder.getInt32Ty();
            }
        }
        return Builder.getDoubleTy();
    } else if (firstValue->getType()->isPointerTy()) {
        return Builder.getPtrTy();
    }
    
    return nullptr;
}

std::unique_ptr<ASTNode> WrapASTNode(ASTNode* node) {
    return std::unique_ptr<ASTNode>(node);
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
                return llvm::PointerType::get(baseType, 0);
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
            return llvm::PointerType::get(baseType, 0);
        }
    }
}

void InitializeArrayFromLiteral(llvm::AllocaInst* arrayAlloca, ArrayNode* arrayLiteral, llvm::Type* elementType, 
                               llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods,
                               std::vector<llvm::Value*>& indices) {
    
    for (size_t i = 0; i < arrayLiteral->elements.size(); i++) {
        ASTNode* element = arrayLiteral->elements[i].get();
        
        if (element->type == NodeType::Array) {
            ArrayNode* subArray = static_cast<ArrayNode*>(element);
            std::vector<llvm::Value*> newIndices = indices;
            newIndices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), i));
            InitializeArrayFromLiteral(arrayAlloca, subArray, elementType, Builder, AllocaMap, Methods, newIndices);
        } else {
            llvm::Value* elementValue = GenerateExpression(arrayLiteral->elements[i], Builder, AllocaMap, Methods);
            if (!elementValue) continue;
            
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
    
    if (Type == "auto") {
        if (!Node->value) {
            Write("Variable Generation", "Auto variable requires initialization: " + Name + Location, 2, true, true, "");
            return;
        }
        
        if (Node->arrayExpression) {
            if (Node->value->type == NodeType::Array) {
                ArrayNode* arrayLiteral = static_cast<ArrayNode*>(Node->value.get());
                BaseType = DeduceArrayElementType(arrayLiteral, Builder, AllocaMap, Methods);
                if (!BaseType) {
                    Write("Variable Generation", "Cannot deduce element type for auto array: " + Name + Location, 2, true, true, "");
                    return;
                }
            } else {
                Write("Variable Generation", "Auto array variable requires array literal: " + Name + Location, 2, true, true, "");
                return;
            }
        } else {
            llvm::Value* tempValue = GenerateExpression(Node->value, Builder, AllocaMap, Methods);
            if (!tempValue) {
                Write("Variable Generation", "Invalid expression for auto variable: " + Name + Location, 2, true, true, "");
                return;
            }
            
            if (tempValue->getType()->isPointerTy()) {
                BaseType = Builder.getPtrTy();
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
    } else if (Type == "string") {
        BaseType = Builder.getPtrTy();
    } else {
        BaseType = GetLLVMTypeFromString(Type, Builder.getContext());
        if (!BaseType) {
            Write("Variable Generation", "Invalid type specified for variable: " + Name + Location, 2, true, true, "");
            return;
        }
    }

    llvm::Type* FinalType = BaseType;
    
    if (Node->arrayExpression) {
        FinalType = GetArrayTypeFromDimensions(Node->arrayExpression.get(), BaseType, Builder, AllocaMap, Methods);
        if (!FinalType) {
            Write("Variable Generation", "Invalid array dimensions for variable: " + Name + Location, 2, true, true, "");
            return;
        }
    }

    llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(FinalType, nullptr, Name);
    if (!AllocaInst) {
        Write("Variable Generation", "Failed to create alloca for variable: " + Name + Location, 2, true, true, "");
        return;
    }

    if (Node->value) {
        if (Node->arrayExpression) {
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

            if (Value->getType() != BaseType) {
                if (BaseType->isIntegerTy(32) && Value->getType()->isFloatingPointTy()) {
                    Value = Builder.CreateFPToSI(Value, BaseType);
                } else if (BaseType->isFloatTy()) {
                    if (Value->getType()->isIntegerTy()) {
                        Value = Builder.CreateSIToFP(Value, BaseType);
                    } else if (Value->getType()->isDoubleTy()) {
                        Value = Builder.CreateFPTrunc(Value, BaseType);
                    }
                } else if (BaseType->isDoubleTy()) {
                    if (Value->getType()->isIntegerTy()) {
                        Value = Builder.CreateSIToFP(Value, BaseType);
                    } else if (Value->getType()->isFloatTy()) {
                        Value = Builder.CreateFPExt(Value, BaseType);
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