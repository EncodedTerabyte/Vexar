#include "BlockGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ConditionGenerator.hh"
#include "VariableGenerator.hh"
#include "IfGenerator.hh"
#include <iostream>

void GenerateBlock(const std::unique_ptr<BlockNode>& Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Block Generator", "Null BlockNode pointer", 2, true, true, "");
        return;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    PushScope(SymbolStack);
    
    for (const auto& Statement : Node->statements) {
        if (!Statement) {
            Write("Block Generator", "Null statement in block" + Location, 2, true, true, "");
            continue;
        }

        std::string StmtLocation = " at line " + std::to_string(Statement->token.line) + ", column " + std::to_string(Statement->token.column);

        if (Builder.GetInsertBlock()->getTerminator()) {
            Write("Block Generator", "Terminator already present, skipping statement" + StmtLocation, 1, true, true, "");
            break;
        }
        
        if (Statement->type == NodeType::If) {
            auto* If = static_cast<IfNode*>(Statement.get());
            if (!If) {
                Write("Block Generator", "Failed to cast to IfNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            llvm::Value* IfResult = GenerateIf(If, Builder, SymbolStack, Methods);
            if (!IfResult && Builder.GetInsertBlock()->getTerminator()) {
                Write("Block Generator", "If statement generation failed" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else if (Statement->type == NodeType::Variable) {
            auto* Var = static_cast<VariableNode*>(Statement.get());
            if (!Var) {
                Write("Block Generator", "Failed to cast to VariableNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            GenerateVariable(Var, Builder, SymbolStack, Methods);
        } else if (Statement->type == NodeType::ArrayAssignment) {
            auto* ArrayAssign = static_cast<ArrayAssignmentNode*>(Statement.get());
            if (!ArrayAssign) {
                Write("Block Generator", "Failed to cast to ArrayAssignmentNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* rvalue = GenerateExpression(ArrayAssign->value, Builder, SymbolStack, Methods);
            if (!rvalue) {
                Write("Block Generator", "Invalid assignment value expression for array assignment" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* arrayPtr = nullptr;
            for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
                auto found = it->find(ArrayAssign->identifier);
                if (found != it->end()) {
                    arrayPtr = found->second;
                    break;
                }
            }
            
            if (!arrayPtr) {
                Write("Block Generator", "Undefined array identifier: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
            if (!allocaInst) {
                Write("Block Generator", "Array identifier is not an allocated variable: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Type* allocatedType = allocaInst->getAllocatedType();
            
            if (ArrayAssign->indexExpr->type == NodeType::Array) {
                // Multi-dimensional array assignment
                auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAssign->indexExpr.get());
                std::vector<llvm::Value*> indices;
                indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
                
                llvm::Type* currentType = allocatedType;
                for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                    llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
                    if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                        Write("Block Generator", "Invalid array index in array assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                    
                    if (!currentType->isArrayTy()) {
                        Write("Block Generator", "Too many dimensions in array assignment: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
                        continue;
                    }
                    
                    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                    if (!arrayType) {
                        Write("Block Generator", "Expected array type in array assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                    
                    uint64_t arraySize = arrayType->getNumElements();
                    if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                        uint64_t index = constIndex->getZExtValue();
                        if (index >= arraySize) {
                            Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in array assignment" + StmtLocation, 2, true, true, "");
                            continue;
                        }
                    }
                    
                    indices.push_back(indexValue);
                    currentType = arrayType->getElementType();
                }
                
                if (rvalue->getType() != currentType) {
                    if (currentType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                        rvalue = Builder.CreateFPToSI(rvalue, currentType);
                    } else if (currentType->isFloatTy()) {
                        if (rvalue->getType()->isIntegerTy()) {
                            rvalue = Builder.CreateSIToFP(rvalue, currentType);
                        } else if (rvalue->getType()->isDoubleTy()) {
                            rvalue = Builder.CreateFPTrunc(rvalue, currentType);
                        }
                    } else if (currentType->isDoubleTy()) {
                        if (rvalue->getType()->isIntegerTy()) {
                            rvalue = Builder.CreateSIToFP(rvalue, currentType);
                        } else if (rvalue->getType()->isFloatTy()) {
                            rvalue = Builder.CreateFPExt(rvalue, currentType);
                        }
                    } else {
                        Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                }
                
                llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
                Builder.CreateStore(rvalue, elementPtr);
                
            } else {
                // Single-dimensional array assignment
                llvm::Value* indexValue = GenerateExpression(ArrayAssign->indexExpr, Builder, SymbolStack, Methods);
                if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                    Write("Block Generator", "Invalid array index in array assignment" + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                if (!allocatedType->isArrayTy()) {
                    Write("Block Generator", "Variable is not an array in array assignment: " + ArrayAssign->identifier + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                if (!arrayType) {
                    Write("Block Generator", "Expected array type in array assignment" + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                    uint64_t index = constIndex->getZExtValue();
                    if (index >= arraySize) {
                        Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in array assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                }
                
                llvm::Type* elementType = arrayType->getElementType();
                if (rvalue->getType() != elementType) {
                    if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                        rvalue = Builder.CreateFPToSI(rvalue, elementType);
                    } else if (elementType->isFloatTy()) {
                        if (rvalue->getType()->isIntegerTy()) {
                            rvalue = Builder.CreateSIToFP(rvalue, elementType);
                        } else if (rvalue->getType()->isDoubleTy()) {
                            rvalue = Builder.CreateFPTrunc(rvalue, elementType);
                        }
                    } else if (elementType->isDoubleTy()) {
                        if (rvalue->getType()->isIntegerTy()) {
                            rvalue = Builder.CreateSIToFP(rvalue, elementType);
                        } else if (rvalue->getType()->isFloatTy()) {
                            rvalue = Builder.CreateFPExt(rvalue, elementType);
                        }
                    } else {
                        Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                }
                
                std::vector<llvm::Value*> indices = {
                    llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                    indexValue
                };
                
                llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
                Builder.CreateStore(rvalue, elementPtr);
            }
        } else if (Statement->type == NodeType::Assignment) {
            auto* Assign = static_cast<AssignmentOpNode*>(Statement.get());
            if (!Assign) {
                Write("Block Generator", "Failed to cast to AssignmentOpNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* rvalue = GenerateExpression(Assign->right, Builder, SymbolStack, Methods);
            if (!rvalue) {
                Write("Block Generator", "Invalid assignment value expression" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            if (Assign->left->type == NodeType::ArrayAccess) {
                auto* ArrayAccess = static_cast<ArrayAccessNode*>(Assign->left.get());
                
                llvm::Value* arrayPtr = nullptr;
                for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
                    auto found = it->find(ArrayAccess->identifier);
                    if (found != it->end()) {
                        arrayPtr = found->second;
                        break;
                    }
                }
                
                if (!arrayPtr) {
                    Write("Block Generator", "Undefined array identifier: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
                if (!allocaInst) {
                    Write("Block Generator", "Array identifier is not an allocated variable: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                llvm::Type* allocatedType = allocaInst->getAllocatedType();
                
                if (ArrayAccess->expr->type == NodeType::Array) {
                    auto* IndexArrayPtr = static_cast<ArrayNode*>(ArrayAccess->expr.get());
                    std::vector<llvm::Value*> indices;
                    indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
                    
                    llvm::Type* currentType = allocatedType;
                    for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                        llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
                        if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                            Write("Block Generator", "Invalid array index in assignment" + StmtLocation, 2, true, true, "");
                            continue;
                        }
                        
                        if (!currentType->isArrayTy()) {
                            Write("Block Generator", "Too many dimensions in array assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                            continue;
                        }
                        
                        llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                        if (!arrayType) {
                            Write("Block Generator", "Expected array type in assignment" + StmtLocation, 2, true, true, "");
                            continue;
                        }
                        
                        uint64_t arraySize = arrayType->getNumElements();
                        if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                            uint64_t index = constIndex->getZExtValue();
                            if (index >= arraySize) {
                                Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in assignment" + StmtLocation, 2, true, true, "");
                                continue;
                            }
                        }
                        
                        indices.push_back(indexValue);
                        currentType = arrayType->getElementType();
                    }
                    
                    if (rvalue->getType() != currentType) {
                        if (currentType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                            rvalue = Builder.CreateFPToSI(rvalue, currentType);
                        } else if (currentType->isFloatTy()) {
                            if (rvalue->getType()->isIntegerTy()) {
                                rvalue = Builder.CreateSIToFP(rvalue, currentType);
                            } else if (rvalue->getType()->isDoubleTy()) {
                                rvalue = Builder.CreateFPTrunc(rvalue, currentType);
                            }
                        } else if (currentType->isDoubleTy()) {
                            if (rvalue->getType()->isIntegerTy()) {
                                rvalue = Builder.CreateSIToFP(rvalue, currentType);
                            } else if (rvalue->getType()->isFloatTy()) {
                                rvalue = Builder.CreateFPExt(rvalue, currentType);
                            }
                        } else {
                            Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                            continue;
                        }
                    }
                    
                    llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
                    Builder.CreateStore(rvalue, elementPtr);
                    
                } else {
                    llvm::Value* indexValue = GenerateExpression(ArrayAccess->expr, Builder, SymbolStack, Methods);
                    if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                        Write("Block Generator", "Invalid array index in assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                    
                    if (!allocatedType->isArrayTy()) {
                        Write("Block Generator", "Variable is not an array in assignment: " + ArrayAccess->identifier + StmtLocation, 2, true, true, "");
                        continue;
                    }
                    
                    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                    if (!arrayType) {
                        Write("Block Generator", "Expected array type in assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                    
                    uint64_t arraySize = arrayType->getNumElements();
                    if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                        uint64_t index = constIndex->getZExtValue();
                        if (index >= arraySize) {
                            Write("Block Generator", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + " in assignment" + StmtLocation, 2, true, true, "");
                            continue;
                        }
                    }
                    
                    llvm::Type* elementType = arrayType->getElementType();
                    if (rvalue->getType() != elementType) {
                        if (elementType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                            rvalue = Builder.CreateFPToSI(rvalue, elementType);
                        } else if (elementType->isFloatTy()) {
                            if (rvalue->getType()->isIntegerTy()) {
                                rvalue = Builder.CreateSIToFP(rvalue, elementType);
                            } else if (rvalue->getType()->isDoubleTy()) {
                                rvalue = Builder.CreateFPTrunc(rvalue, elementType);
                            }
                        } else if (elementType->isDoubleTy()) {
                            if (rvalue->getType()->isIntegerTy()) {
                                rvalue = Builder.CreateSIToFP(rvalue, elementType);
                            } else if (rvalue->getType()->isFloatTy()) {
                                rvalue = Builder.CreateFPExt(rvalue, elementType);
                            }
                        } else {
                            Write("Block Generator", "Type mismatch in array assignment" + StmtLocation, 2, true, true, "");
                            continue;
                        }
                    }
                    
                    std::vector<llvm::Value*> indices = {
                        llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                        indexValue
                    };
                    
                    llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
                    Builder.CreateStore(rvalue, elementPtr);
                }
            } else if (Assign->left->type == NodeType::Identifier) {
                auto* Ident = static_cast<IdentifierNode*>(Assign->left.get());
                
                llvm::Value* varPtr = nullptr;
                for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
                    auto found = it->find(Ident->name);
                    if (found != it->end()) {
                        varPtr = found->second;
                        break;
                    }
                }
                
                if (!varPtr) {
                    Write("Block Generator", "Undefined variable: " + Ident->name + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(varPtr);
                if (!allocaInst) {
                    Write("Block Generator", "Variable is not modifiable: " + Ident->name + StmtLocation, 2, true, true, "");
                    continue;
                }
                
                llvm::Type* varType = allocaInst->getAllocatedType();
                if (rvalue->getType() != varType) {
                    if (varType->isIntegerTy(32) && rvalue->getType()->isFloatingPointTy()) {
                        rvalue = Builder.CreateFPToSI(rvalue, varType);
                    } else if (varType->isFloatTy()) {
                        if (rvalue->getType()->isIntegerTy()) {
                            rvalue = Builder.CreateSIToFP(rvalue, varType);
                        } else if (rvalue->getType()->isDoubleTy()) {
                            rvalue = Builder.CreateFPTrunc(rvalue, varType);
                        }
                    } else if (varType->isDoubleTy()) {
                        if (rvalue->getType()->isIntegerTy()) {
                            rvalue = Builder.CreateSIToFP(rvalue, varType);
                        } else if (rvalue->getType()->isFloatTy()) {
                            rvalue = Builder.CreateFPExt(rvalue, varType);
                        }
                    } else {
                        Write("Block Generator", "Type mismatch in variable assignment" + StmtLocation, 2, true, true, "");
                        continue;
                    }
                }
                
                Builder.CreateStore(rvalue, allocaInst);
            } else {
                Write("Block Generator", "Invalid assignment target" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else if (Statement->type == NodeType::Return) {
            auto* Ret = static_cast<ReturnNode*>(Statement.get());
            if (!Ret) {
                Write("Block Generator", "Failed to cast to ReturnNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* Value = nullptr;
            if (Ret->value) {
                Value = GenerateExpression(Ret->value, Builder, SymbolStack, Methods);
                if (!Value) {
                    Write("Block Generator", "Invalid return expression" + StmtLocation, 2, true, true, "");
                    continue;
                }
            }
            
            llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
            if (!CurrentFunc) {
                Write("Block Generator", "Invalid current function for return statement" + StmtLocation, 2, true, true, "");
                continue;
            }

            llvm::Type* ReturnType = CurrentFunc->getReturnType();

            if (Value && Value->getType() != ReturnType) {
                if (ReturnType->isIntegerTy() && Value->getType()->isFloatingPointTy()) {
                    Value = Builder.CreateFPToSI(Value, ReturnType, "fptosi");
                } else if (ReturnType->isFloatingPointTy() && Value->getType()->isIntegerTy()) {
                    Value = Builder.CreateSIToFP(Value, ReturnType, "sitofp");
                } else if (ReturnType->isFloatingPointTy() && Value->getType()->isFloatingPointTy()) {
                    if (ReturnType->getTypeID() == llvm::Type::FloatTyID && 
                        Value->getType()->getTypeID() == llvm::Type::DoubleTyID) {
                        Value = Builder.CreateFPTrunc(Value, ReturnType, "fptrunc");
                    } else if (ReturnType->getTypeID() == llvm::Type::DoubleTyID && 
                            Value->getType()->getTypeID() == llvm::Type::FloatTyID) {
                        Value = Builder.CreateFPExt(Value, ReturnType, "fpext");
                    }
                } else {
                    Write("Block Generator", "Type mismatch in return statement" + StmtLocation, 2, true, true, "");
                    continue;
                }
            }
            
            if (Value) {
                Builder.CreateRet(Value);
            } else {
                if (ReturnType->isVoidTy()) {
                    Builder.CreateRetVoid();
                } else {
                    llvm::Value* defaultValue;
                    if (ReturnType->isFloatingPointTy()) {
                        defaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
                    } else if (ReturnType->isIntegerTy()) {
                        defaultValue = llvm::ConstantInt::get(ReturnType, 0);
                    } else if (ReturnType->isPointerTy()) {
                        defaultValue = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ReturnType));
                    } else {
                        defaultValue = llvm::UndefValue::get(ReturnType);
                        Write("Block Generator", "Unsupported return type for default value" + StmtLocation, 1, true, true, "");
                    }
                    Builder.CreateRet(defaultValue);
                }
            }
        } else if (Statement->type == NodeType::FunctionCall) {
            auto* FuncCall = static_cast<FunctionCallNode*>(Statement.get());
            if (!FuncCall) {
                Write("Block Generator", "Failed to cast to FunctionCallNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            llvm::Value* CallResult = GenerateExpression(Statement, Builder, SymbolStack, Methods);
            if (!CallResult) {
                Write("Block Generator", "Invalid function call expression" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else {
            Write("Block Generator", "Unsupported statement type: " + std::to_string(Statement->type) + StmtLocation, 2, true, true, "");
        }
    }
    
    PopScope(SymbolStack);
}