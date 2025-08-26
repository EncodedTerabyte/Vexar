#include "CompoundAssignmentGenerator.hh"
#include "UnaryAssignmentGenerator.hh"
#include "ArrayAssignmentGenerator.hh"
#include "AssignmentGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ConditionGenerator.hh"
#include "VariableGenerator.hh"
#include "ReturnGenerator.hh"
#include "BlockGenerator.hh"
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
            
            llvm::Value* ArrayAssignResult = GenerateArrayAssignment(ArrayAssign, Builder, SymbolStack, Methods);
            if (!ArrayAssignResult && Builder.GetInsertBlock()->getTerminator()) {
                Write("Block Generator", "ArrayAssign statement generation failed" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else if (Statement->type == NodeType::Assignment) {
            auto* Assign = static_cast<AssignmentOpNode*>(Statement.get());
            if (!Assign) {
                Write("Block Generator", "Failed to cast to AssignNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* AssignResult = GenerateAssignment(Assign, Builder, SymbolStack, Methods);
            if (!AssignResult && Builder.GetInsertBlock()->getTerminator()) {
                Write("Block Generator", "Assign statement generation failed" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else if (Statement->type == NodeType::CompoundAssignment) {
            auto* CompAssign = static_cast<CompoundAssignmentOpNode*>(Statement.get());
            if (!CompAssign) {
                Write("Block Generator", "Failed to cast to CompoundAssignmentOpNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* CompAssignResult = GenerateCompoundAssignment(CompAssign, Builder, SymbolStack, Methods);
            if (!CompAssignResult && Builder.GetInsertBlock()->getTerminator()) {
                Write("Block Generator", "CompoundAssign statement generation failed" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else if (Statement->type == NodeType::UnaryOp) {
            auto* UnaryOp = static_cast<UnaryOpNode*>(Statement.get());
            if (!UnaryOp) {
                Write("Block Generator", "Failed to cast to UnaryOpNode" + StmtLocation, 2, true, true, "");
                continue;
            }
            
            llvm::Value* UnaryOpResult = GenerateUnaryAssignment(UnaryOp, Builder, SymbolStack, Methods);
            if (!UnaryOpResult && Builder.GetInsertBlock()->getTerminator()) {
                Write("Block Generator", "UnaryOp statement generation failed" + StmtLocation, 2, true, true, "");
                continue;
            }
        } else if (Statement->type == NodeType::Return) {
            auto* Ret = static_cast<ReturnNode*>(Statement.get());    
            if (!Ret) {
                Write("Block Generator", "Failed to cast to ReturnNode" + StmtLocation, 2, true, true, "");
                continue;
            }

            llvm::Value* RetResult = GenerateReturn(Ret, Builder, SymbolStack, Methods);
            if (!RetResult && Builder.GetInsertBlock()->getTerminator()) {
                Write("Block Generator", "Return statement generation failed" + StmtLocation, 2, true, true, "");
                continue;
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