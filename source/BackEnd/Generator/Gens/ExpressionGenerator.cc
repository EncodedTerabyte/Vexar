#include "ExpressionGenerator.hh"

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    if (Expr->type == NodeType::Number) {
        auto* NumNode = static_cast<NumberNode*>(Expr.get());
        return llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(NumNode->value));
    } else if (Expr->type == NodeType::Paren) {
        auto* ParennNode = static_cast<ParenNode*>(Expr.get());
        return GenerateExpression(ParennNode->inner, Builder, AllocaMap);
    } else if (Expr->type == NodeType::BinaryOp) {
        auto* BinOpNode = static_cast<BinaryOpNode*>(Expr.get());
        llvm::Value* Left = GenerateExpression(BinOpNode->left, Builder, AllocaMap);
        llvm::Value* Right = GenerateExpression(BinOpNode->right, Builder, AllocaMap);

        if (BinOpNode->op == "+") {
            return Builder.CreateFAdd(Left, Right, "addtmp");
        } else if (BinOpNode->op == "-") {
            return Builder.CreateFSub(Left, Right, "subtmp");
        } else if (BinOpNode->op == "*") {
            return Builder.CreateFMul(Left, Right, "multmp");
        } else if (BinOpNode->op == "/") {
            return Builder.CreateFDiv(Left, Right, "divtmp");
        } else if (BinOpNode->op == "+") {
            return Builder.CreateAdd(Left, Right, "addtmp");
        } else if (BinOpNode->op == "-") {
            return Builder.CreateSub(Left, Right, "subtmp");
        } else if (BinOpNode->op == "*") {
            return Builder.CreateMul(Left, Right, "multmp");
        } else if (BinOpNode->op == "/") {
            return Builder.CreateSDiv(Left, Right, "divtmp");
        } else {
            std::cerr << "Unsupported binary operator: " << BinOpNode->op << std::endl;
            exit(1);
        }
    } else if (Expr->type == NodeType::Identifier) {
        auto* Identifier = static_cast<IdentifierNode*>(Expr.get());
        auto it = AllocaMap.find(Identifier->name);

        if (it != AllocaMap.end()) {
            llvm::AllocaInst* AllocaInst = it->second;
            return Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, Identifier->name.c_str());
        }
    }

    return nullptr;
}
