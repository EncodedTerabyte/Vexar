#include "VariableGenerator.hh"
#include "ExpressionGenerator.hh"
#include <iostream>
#include <cmath>

void GenerateVariable(VariableNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Variable Generation", "Null VariableNode provided", 2, true, true, "");
        return;
    }
    
    std::string Name = Node->name;
    std::string Type = Node->varType.name;
    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Type* BaseType = nullptr;
    bool isArray = false;
    
    if (Type == "auto" || Type.empty()) {
        if (!Node->value) {
            Write("Variable Generation", "Auto variable requires initialization: " + Name + Location, 2, true, true, "");
            return;
        }
        
        if (Node->value->type == NodeType::Array) {
            BaseType = IR->int_t();
            isArray = true;
        } else {
            llvm::Value* tempValue = GenerateExpression(Node->value, IR, Methods);
            if (!tempValue) {
                Write("Variable Generation", "Invalid expression for auto variable: " + Name + Location, 2, true, true, "");
                return;
            }
            
            BaseType = tempValue->getType();
        }
    } else {
        std::string baseTypeName = Type;
        if (Type.find("[]") != std::string::npos) {
            baseTypeName = Type.substr(0, Type.find("[]"));
            isArray = true;
        }
        
        BaseType = GetAeroTypeFromString(baseTypeName, IR);
        if (!BaseType) {
            Write("Variable Generation", "Invalid type specified for variable: " + Name + Location, 2, true, true, "");
            return;
        }
    }

    llvm::Value* AllocaInst = nullptr;

    if (isArray) {
        if (Node->value && Node->value->type == NodeType::Array) {
            ArrayNode* arrayLiteral = static_cast<ArrayNode*>(Node->value.get());
            if (!arrayLiteral) {
                Write("Variable Generation", "Failed to cast to ArrayNode: " + Name + Location, 2, true, true, "");
                return;
            }
            
            int arraySize = arrayLiteral->elements.size();
            
            if (Type.find("[][]") != std::string::npos) {
                llvm::Type* innerArrayType = IR->array(BaseType, 3);
                AllocaInst = IR->var(Name, IR->array(innerArrayType, arraySize));
                
                for (size_t i = 0; i < arrayLiteral->elements.size(); ++i) {
                    if (arrayLiteral->elements[i]->type == NodeType::Array) {
                        ArrayNode* innerArray = static_cast<ArrayNode*>(arrayLiteral->elements[i].get());
                        if (innerArray) {
                            for (size_t j = 0; j < innerArray->elements.size(); ++j) {
                                llvm::Value* elementValue = GenerateExpression(innerArray->elements[j], IR, Methods);
                                if (elementValue) {
                                    llvm::Value* elementPtr = IR->getBuilder()->CreateInBoundsGEP(
                                        IR->array(innerArrayType, arraySize), 
                                        AllocaInst, 
                                        {IR->constI32(0), IR->constI32(i), IR->constI32(j)}
                                    );
                                    IR->store(elementValue, elementPtr);
                                }
                            }
                        }
                    }
                }
            } else {
                AllocaInst = IR->stackArray(Name, BaseType, arraySize);
                
                for (size_t i = 0; i < arrayLiteral->elements.size(); ++i) {
                    llvm::Value* elementValue = GenerateExpression(arrayLiteral->elements[i], IR, Methods);
                    if (!elementValue) {
                        Write("Variable Generation", "Invalid array element at index " + std::to_string(i) + ": " + Name + Location, 2, true, true, "");
                        continue;
                    }
                    
                    llvm::Value* elementPtr = IR->arrayAccess(AllocaInst, IR->constI32(i));
                    if (elementPtr) {
                        IR->store(elementValue, elementPtr);
                    }
                }
            }
        } else if (Node->arrayExpression) {
            llvm::Value* sizeValue = GenerateExpression(Node->arrayExpression, IR, Methods);
            if (!sizeValue || !sizeValue->getType()->isIntegerTy()) {
                Write("Variable Generation", "Invalid array size for variable: " + Name + Location, 2, true, true, "");
                return;
            }
            
            AllocaInst = IR->heapArray(Name, BaseType, sizeValue);
        } else {
            AllocaInst = IR->var(Name, IR->ptr(BaseType));
            
            if (Node->value) {
                llvm::Value* Value = GenerateExpression(Node->value, IR, Methods);
                if (!Value) {
                    Write("Variable Generation", "Invalid expression for variable: " + Name + Location, 2, true, true, "");
                    return;
                }
                IR->store(Value, AllocaInst);
            }
        }
    } else {
        AllocaInst = IR->var(Name, BaseType);
        
        if (Node->value) {
            llvm::Value* Value = GenerateExpression(Node->value, IR, Methods);
            if (!Value) {
                Write("Variable Generation", "Invalid expression for variable: " + Name + Location, 2, true, true, "");
                return;
            }

            if (Value->getType() != BaseType) {
                if (BaseType->isIntegerTy() && Value->getType()->isIntegerTy()) {
                    Value = IR->intCast(Value, BaseType);
                } else if (BaseType->isFloatingPointTy() && Value->getType()->isIntegerTy()) {
                    Value = IR->cast(Value, BaseType);
                } else if (BaseType->isIntegerTy() && Value->getType()->isFloatingPointTy()) {
                    Value = IR->cast(Value, BaseType);
                } else if (BaseType->isFloatingPointTy() && Value->getType()->isFloatingPointTy()) {
                    Value = IR->floatCast(Value, BaseType);
                } else if (BaseType->isPointerTy() && Value->getType()->isPointerTy()) {
                    if (BaseType != Value->getType()) {
                        Value = IR->cast(Value, BaseType);
                    }
                } else {
                    Write("Variable Generation", "Type mismatch for variable: " + Name + Location, 2, true, true, "");
                    return;
                }
            }

            IR->store(Value, AllocaInst);
        }
    }

    if (!AllocaInst) {
        Write("Variable Generation", "Failed to create variable: " + Name + Location, 2, true, true, "");
        return;
    }
}