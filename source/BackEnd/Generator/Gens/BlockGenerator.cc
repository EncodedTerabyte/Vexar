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