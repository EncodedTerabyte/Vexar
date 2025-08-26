#include "ExpressionGenerator.hh"
#include "IdentifierGenerator.hh"
#include "BinaryOpGenerator.hh"
#include "CastGenerator.hh"
#include "CallGenerator.hh"
#include "NumberGenerator.hh"

#include "ArrayExpressionGenerator.hh"

#include <cmath>
#include <functional>
#include <unordered_map>

#include "DefaultSymbols.hh"

static BuiltinSymbols Builtins;

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    InitializeBuiltinSymbols(Builtins);
    
    if (!Expr) {
        Write("Expression Generation", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    if (Expr->type == NodeType::Number) {
        llvm::Value* Result = GenerateNumber(Expr, Builder, SymbolStack, Methods);
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
       llvm::Value* Result = Builder.CreateGlobalString(StrNode->value, "", 0, nullptr);
       return Result;
    } else if (Expr->type == NodeType::Character) {
       auto* CharNode = static_cast<CharacterNode*>(Expr.get());
       if (!CharNode) {
           Write("Expression Generation", "Failed to cast to CharacterNode" + Location, 2, true, true, "");
           return nullptr;
       }
       llvm::Value* Result = llvm::ConstantInt::get(llvm::Type::getInt8Ty(Builder.getContext()), CharNode->value);
       return Result;
    } else if (Expr->type == NodeType::Paren) {
        auto* ParenNodePtr = static_cast<ParenNode*>(Expr.get());
        if (!ParenNodePtr) {
            Write("Expression Generation", "Failed to cast to ParenNode" + Location, 2, true, true, "");
            return nullptr;
        }
        llvm::Value* Result = GenerateExpression(ParenNodePtr->inner, Builder, SymbolStack, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid parenthesized expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::BinaryOp) {
       llvm::Value* Result = GenerateBinaryOp(Expr, Builder, SymbolStack, Methods);
       if (!Result) {
           Write("Expression Generation", "Invalid binary operation" + Location, 2, true, true, "");
           return nullptr;
       }
       return Result;
    } else if (Expr->type == NodeType::Identifier) {
        llvm::Value* Result = GenerateIdentifier(Expr, Builder, SymbolStack, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid identifier expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::FunctionCall) {
        llvm::Value* Result = GenerateCall(Expr, Builder, SymbolStack, Methods, Builtins);
        if (!Result) {
            Write("Expression Generation", "Invalid function call" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::Cast) {
        llvm::Value* Result = GenerateCast(Expr, Builder, SymbolStack, Methods);
        if (!Result) {
            Write("Expression Generation", "Invalid cast expression" + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    } else if (Expr->type == NodeType::Array || Expr->type == NodeType::ArrayAccess) {
        return GenerateArrayExpression(Expr, Builder, SymbolStack, Methods);
    }
       
    Write("Expression Generation", "Unsupported expression type: " + std::to_string(Expr->type) + Location, 2, true, true, "");
    return nullptr;
}