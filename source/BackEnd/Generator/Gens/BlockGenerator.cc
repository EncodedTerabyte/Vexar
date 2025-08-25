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
            
            std::cout << "Processing return statement..." << std::endl;
            
            llvm::Value* Value = GenerateExpression(Ret->value, Builder, SymbolStack, Methods);
            
            std::cout << "Return value type: ";
            if (Value) {
                Value->getType()->print(llvm::errs());
                std::cout << std::endl;
                if (llvm::ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(Value)) {
                    std::cout << "Constant int value: " << CI->getSExtValue() << std::endl;
                } else if (llvm::ConstantFP* CF = llvm::dyn_cast<llvm::ConstantFP>(Value)) {
                    std::cout << "Constant float value: " << CF->getValueAPF().convertToDouble() << std::endl;
                }
            } else {
                std::cout << "nullptr" << std::endl;
            }

            llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
            llvm::Type* ReturnType = CurrentFunc->getReturnType();

            if (Value->getType() != ReturnType) {
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
            
            Builder.CreateRet(Value);
        } else if (Statement->type == NodeType::FunctionCall) {
            auto* FuncCall = static_cast<FunctionCallNode*>(Statement.get());
            GenerateExpression(Statement, Builder, SymbolStack, Methods);
        }
        
    }
    
    PopScope(SymbolStack);
}