#include "BlockGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ConditionGenerator.hh"
#include "VariableGenerator.hh"
#include "IfGenerator.hh"
#include <iostream>

void GenerateBlock(const std::unique_ptr<BlockNode>& Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    PushScope(SymbolStack);
    
    for (const auto& Statement : Node->statements) {
        if (Builder.GetInsertBlock()->getTerminator()) {
            break;
        }
        
        if (Statement->type == NodeType::If) {
            auto* If = static_cast<IfNode*>(Statement.get());
            GenerateIf(If, Builder, SymbolStack, Methods);
        } else if (Statement->type == NodeType::Variable) {
            auto* Var = static_cast<VariableNode*>(Statement.get());
            GenerateVariable(Var, Builder, SymbolStack, Methods);
        } else if (Statement->type == NodeType::Return) {
            auto* Ret = static_cast<ReturnNode*>(Statement.get());
            
            llvm::Value* Value = nullptr;
            if (Ret->value) {
                Value = GenerateExpression(Ret->value, Builder, SymbolStack, Methods);
            }
            
            llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
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
                    }
                    Builder.CreateRet(defaultValue);
                }
            }
        } else if (Statement->type == NodeType::FunctionCall) {
            auto* FuncCall = static_cast<FunctionCallNode*>(Statement.get());
            GenerateExpression(Statement, Builder, SymbolStack, Methods);
        }
    }
    
    PopScope(SymbolStack);
}