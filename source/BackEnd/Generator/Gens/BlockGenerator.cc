#include "CompoundAssignmentGenerator.hh"
#include "UnaryAssignmentGenerator.hh"
#include "ArrayAssignmentGenerator.hh"
#include "AssignmentGenerator.hh"
#include "ExpressionGenerator.hh"
#include "ConditionGenerator.hh"
#include "VariableGenerator.hh"
#include "ReturnGenerator.hh"
#include "WhileGenerator.hh"
#include "BlockGenerator.hh"
#include "BreakGenerator.hh"
#include "ForGenerator.hh"
#include "IfGenerator.hh"

#include <iostream>

void ProcessStatement(const std::unique_ptr<ASTNode>& Statement, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Statement) {
        Write("Block Generator", "Null statement", 2, true, true, "");
        return;
    }

    std::string StmtLocation = " at line " + std::to_string(Statement->token.line) + ", column " + std::to_string(Statement->token.column);

    if (Builder.GetInsertBlock()->getTerminator()) {
        return;
    }
    
    if (Statement->type == NodeType::If) {
        auto* If = static_cast<IfNode*>(Statement.get());
        if (!If) {
            Write("Block Generator", "Failed to cast to IfNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateIf(If, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::Variable) {
        auto* Var = static_cast<VariableNode*>(Statement.get());
        if (!Var) {
            Write("Block Generator", "Failed to cast to VariableNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateVariable(Var, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::ArrayAssignment) {
        auto* ArrayAssign = static_cast<ArrayAssignmentNode*>(Statement.get());
        if (!ArrayAssign) {
            Write("Block Generator", "Failed to cast to ArrayAssignmentNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateArrayAssignment(ArrayAssign, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::Assignment) {
        auto* Assign = static_cast<AssignmentOpNode*>(Statement.get());
        if (!Assign) {
            Write("Block Generator", "Failed to cast to AssignNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateAssignment(Assign, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::CompoundAssignment) {
        auto* CompAssign = static_cast<CompoundAssignmentOpNode*>(Statement.get());
        if (!CompAssign) {
            Write("Block Generator", "Failed to cast to CompoundAssignmentOpNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateCompoundAssignment(CompAssign, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::UnaryOp) {
        auto* UnaryOp = static_cast<UnaryOpNode*>(Statement.get());
        if (!UnaryOp) {
            Write("Block Generator", "Failed to cast to UnaryOpNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateUnaryAssignment(UnaryOp, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::Return) {
        auto* Ret = static_cast<ReturnNode*>(Statement.get());    
        if (!Ret) {
            Write("Block Generator", "Failed to cast to ReturnNode" + StmtLocation, 2, true, true, "");
            return;
        }
        GenerateReturn(Ret, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::FunctionCall) {
        auto* FuncCall = static_cast<FunctionCallNode*>(Statement.get());
        if (!FuncCall) {
            Write("Block Generator", "Failed to cast to FunctionCallNode" + StmtLocation, 2, true, true, "");
            return;
        }
        llvm::Value* CallResult = GenerateExpression(Statement, Builder, SymbolStack, Methods);
        if (!CallResult) {
            Write("Block Generator", "Invalid function call expression" + StmtLocation, 2, true, true, "");
            return;
        }
    } else if (Statement->type == NodeType::While) {
        auto* While = static_cast<WhileNode*>(Statement.get());
        if (!While) {
            Write("Block Generator", "Failed to cast to WhileNode" + StmtLocation, 2, true, true, "");
            return;
        }
        llvm::Value* WhileResult = GenerateWhile(While, Builder, SymbolStack, Methods);
        if (!WhileResult) {
            Write("Block Generator", "Invalid while statement" + StmtLocation, 2, true, true, "");
            return;
        }
    } else if (Statement->type == NodeType::Break) {
        auto* Break = static_cast<BreakNode*>(Statement.get());
        if (!Break) {
            Write("Block Generator", "Failed to cast to BreakNode" + StmtLocation, 2, true, true, "");
            return;
        }
        llvm::Value* BreakResult = GenerateBreak(Break, Builder, SymbolStack, Methods);
        if (!BreakResult) {
            Write("Block Generator", "Invalid break statement" + StmtLocation, 2, true, true, "");
            return;
        }
    } else if (Statement->type == NodeType::SemiColon) {
        
    } else if (Statement->type == NodeType::Block) {
        auto* NestedBlock = static_cast<BlockNode*>(Statement.get());
        if (!NestedBlock) {
            Write("Block Generator", "Failed to cast to BlockNode" + StmtLocation, 2, true, true, "");
            return;
        }
        
        auto TempBlock = std::make_unique<BlockNode>();
        TempBlock->token = NestedBlock->token;
        TempBlock->statements = std::move(const_cast<std::vector<std::unique_ptr<ASTNode>>&>(NestedBlock->statements));
        GenerateBlock(TempBlock, Builder, SymbolStack, Methods);
    } else if (Statement->type == NodeType::For) {
        auto* For = static_cast<ForNode*>(Statement.get());
        if (!For) {
            Write("Block Generator", "Failed to cast to ForNode" + StmtLocation, 2, true, true, "");
            return;
        }
        llvm::Value* ForResult = GenerateFor(For, Builder, SymbolStack, Methods);
        if (!ForResult) {
            Write("Block Generator", "Invalid for statement" + StmtLocation, 2, true, true, "");
            return;
        }
    } else {
        Write("Block Generator", "Unsupported statement type: " + std::to_string(static_cast<int>(Statement->type)) + StmtLocation, 2, true, true, "");
    }
}

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
        
        if (Builder.GetInsertBlock()->getTerminator()) {
            break;
        }
        
        ProcessStatement(Statement, Builder, SymbolStack, Methods);
    }
    
    PopScope(SymbolStack);
}