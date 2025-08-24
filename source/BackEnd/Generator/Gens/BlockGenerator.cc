#include "BlockGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ConditionGenerator.hh"

void GenerateBlock(const std::unique_ptr<BlockNode>& Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    for (const auto& Statement : Node->statements) {
        if (Statement->type == NodeType::If) {
            auto* If = static_cast<IfNode*>(Statement.get());
            
            for (auto& Branch : If->branches) {
                int Type = static_cast<int>(Branch.type);

                if (Type == 0) { // main branch
                    ConditionNode* Condition = Branch.condition.get();
                    std::cout << "before eval" << std::endl;
                    llvm::Value* Value = GenerateCondition(Condition, Builder, AllocaMap);
                    Value->print(llvm::outs());
                } 
            }
        } else if (Statement->type == NodeType::Variable) {
            auto* Var = static_cast<VariableNode*>(Statement.get());
            std::string Name = Var->name;
            std::string Type = Var->varType.name;

            llvm::Value* Value = GenerateExpression(Var->value, Builder, AllocaMap);

            llvm::Type* IdentifiedType;
            if (Type == "auto") {
                IdentifiedType = Value->getType();
            } else {
                IdentifiedType = GetLLVMTypeFromString(Type, Builder.getContext());
            }

            if (GetStringFromLLVMType(IdentifiedType) == "int") {
                if (Value->getType()->isFloatTy()) {
                    Value = Builder.CreateFPToSI(Value, IdentifiedType);
                } else if (Value->getType()->isDoubleTy()) {
                    Value = Builder.CreateFPToSI(Value, IdentifiedType);
                }
            } else if (GetStringFromLLVMType(IdentifiedType) == "float") {
                if (Value->getType()->isIntegerTy()) {
                    Value = Builder.CreateSIToFP(Value, IdentifiedType);
                } else if (Value->getType()->isDoubleTy()) {
                    Value = Builder.CreateFPTrunc(Value, IdentifiedType);
                }
            } else if (GetStringFromLLVMType(IdentifiedType) == "double") {
                if (Value->getType()->isIntegerTy()) {
                    Value = Builder.CreateSIToFP(Value, IdentifiedType);
                } else if (Value->getType()->isFloatTy()) {
                    Value = Builder.CreateFPExt(Value, IdentifiedType);
                }
            }

            llvm::AllocaInst* AllocaInst = Builder.CreateAlloca(IdentifiedType, nullptr, Name);
            Builder.CreateStore(Value, AllocaInst);
            AllocaMap[Name] = AllocaInst;
        } else if (Statement->type == NodeType::Return) {
            auto* Ret = static_cast<ReturnNode*>(Statement.get());
            llvm::Value* Value = GenerateExpression(Ret->value, Builder, AllocaMap);
            Builder.CreateRet(Value);
        }
    }
}
