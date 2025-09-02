#include "ExpressionGenerator.hh"
#include "IdentifierGenerator.hh"
#include "BinaryOpGenerator.hh"
#include "CastGenerator.hh"
#include "CallGenerator.hh"
#include "NumberGenerator.hh"
#include "UnaryOpGenerator.hh"
#include "CompoundAssignmentGenerator.hh"
#include "ArrayExpressionGenerator.hh"

#include <cmath>
#include <functional>
#include <unordered_map>

#include "DefaultSymbols.hh"

static BuiltinSymbols Builtins;

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods) {
    InitializeBuiltinSymbols(Builtins);
    
    if (!Expr) {
        Write("Expression Generation", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    if (Expr->type == NodeType::Number) {
        llvm::Value* Result = GenerateNumber(Expr, IR, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid number expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::String) {
       auto* StrNode = static_cast<StringNode*>(Expr.get());
       if (!StrNode) {
           Write("Expression Generation", "Failed to cast to StringNode" + Location, 2, true, true, "");
           return nullptr;
       }
       return IR->constString(StrNode->value);
    } else if (Expr->type == NodeType::Character) {
       auto* CharNode = static_cast<CharacterNode*>(Expr.get());
       if (!CharNode) {
           Write("Expression Generation", "Failed to cast to CharacterNode" + Location, 2, true, true, "");
           return nullptr;
       }
       return IR->constChar(CharNode->value);
    } else if (Expr->type == NodeType::Paren) {
        auto* ParenNodePtr = static_cast<ParenNode*>(Expr.get());
        if (!ParenNodePtr) {
            Write("Expression Generation", "Failed to cast to ParenNode" + Location, 2, true, true, "");
            return nullptr;
        }
        llvm::Value* Result = GenerateExpression(ParenNodePtr->inner, IR, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid parenthesized expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::BinaryOp) {
       llvm::Value* Result = GenerateBinaryOp(Expr, IR, Methods);
       if (!Result) {
           Write("Expression Generation", "Invalid binary operation" + Location, 2, true, true, "");
           return nullptr;
       }
       return Result;
    } else if (Expr->type == NodeType::UnaryOp) {
       llvm::Value* Result = GenerateUnaryOp(Expr, IR, Methods);
       if (!Result) {
           Write("Expression Generation", "Invalid unary operation" + Location, 2, true, true, "");
           return nullptr;
       }
       return Result;
    } else if (Expr->type == NodeType::CompoundAssignment) {
        auto* CompoundAssign = static_cast<CompoundAssignmentOpNode*>(Expr.get());
        if (!CompoundAssign) {
            Write("Expression Generation", "Failed to cast to CompoundAssignmentOpNode" + Location, 2, true, true, "");
            return nullptr;
        }
        llvm::Value* Result = GenerateCompoundAssignment(CompoundAssign, IR, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid compound assignment" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::Identifier) {
        llvm::Value* Result = GenerateIdentifier(Expr, IR, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid identifier expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::FunctionCall) {
        llvm::Value* Result = GenerateCall(Expr, IR, Methods, Builtins);
        if (!Result) {
            Write("Expression Generation", "Invalid function call" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::Cast) {
        llvm::Value* Result = GenerateCast(Expr, IR, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid cast expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::Array || Expr->type == NodeType::ArrayAccess) {
        return GenerateArrayExpression(Expr, IR, Methods);
    } else if (Expr->type == NodeType::Boolean) {
        auto* BoolNode = static_cast<BooleanNode*>(Expr.get());
        if (!BoolNode) {
            Write("Expression Generation", "Failed to cast to BooleanNode" + Location, 2, true, true, "");
            return nullptr;
        }
        return IR->constBool(BoolNode->value);
    }
       
    Write("Expression Generation", "Unsupported expression type: " + std::to_string(static_cast<int>(Expr->type)) + Location, 2, true, true, "");
    return nullptr;
}