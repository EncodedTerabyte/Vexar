#include "BlockGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ConditionGenerator.hh"
#include "VariableGenerator.hh"
#include "IfGenerator.hh"

void GenerateBlock(const std::unique_ptr<BlockNode>& Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    for (const auto& Statement : Node->statements) {
        if (Statement->type == NodeType::If) {
            auto* If = static_cast<IfNode*>(Statement.get());
            GenerateIf(If, Builder, AllocaMap);
        } else if (Statement->type == NodeType::Variable) {
            auto* Var = static_cast<VariableNode*>(Statement.get());
            GenerateVariable(Var, Builder, AllocaMap);
        } else if (Statement->type == NodeType::Return) {
            auto* Ret = static_cast<ReturnNode*>(Statement.get());
            llvm::Value* Value = GenerateExpression(Ret->value, Builder, AllocaMap);
            Builder.CreateRet(Value);
        }
    }
}
