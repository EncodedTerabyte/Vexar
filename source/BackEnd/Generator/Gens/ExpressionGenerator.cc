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

llvm::Value* GenerateUnaryOp(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    auto* UnaryNode = static_cast<UnaryOpNode*>(Expr.get());
    if (!UnaryNode) {
        return nullptr;
    }
    
    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    llvm::Value* Operand = GenerateExpression(UnaryNode->operand, Builder, SymbolStack, Methods);
    if (!Operand) {
        Write("Expression Generation", "Invalid unary operand" + Location, 2, true, true, "");
        return nullptr;
    }
    
    if (UnaryNode->op == "-") {
        if (Operand->getType()->isFloatingPointTy()) {
            return Builder.CreateFNeg(Operand, "neg");
        } else if (Operand->getType()->isIntegerTy()) {
            return Builder.CreateNeg(Operand, "neg");
        } else {
            Write("Expression Generation", "Invalid type for unary minus" + Location, 2, true, true, "");
            return nullptr;
        }
    } else if (UnaryNode->op == "+") {
        return Operand;
    } else if (UnaryNode->op == "!") {
        if (Operand->getType()->isIntegerTy(1)) {
            return Builder.CreateNot(Operand, "not");
        } else {
            llvm::Value* Zero;
            if (Operand->getType()->isFloatingPointTy()) {
                Zero = llvm::ConstantFP::get(Operand->getType(), 0.0);
                return Builder.CreateFCmpOEQ(Operand, Zero, "not");
            } else if (Operand->getType()->isIntegerTy()) {
                Zero = llvm::ConstantInt::get(Operand->getType(), 0);
                return Builder.CreateICmpEQ(Operand, Zero, "not");
            } else {
                Write("Expression Generation", "Invalid type for logical not" + Location, 2, true, true, "");
                return nullptr;
            }
        }
    }
    
    Write("Expression Generation", "Unsupported unary operator: " + UnaryNode->op + Location, 2, true, true, "");
    return nullptr;
}

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
    } else if (Expr->type == NodeType::UnaryOp) {
       llvm::Value* Result = GenerateUnaryOp(Expr, Builder, SymbolStack, Methods);
       if (!Result) {
           Write("Expression Generation", "Invalid unary operation" + Location, 2, true, true, "");
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
       
    Write("Expression Generation", "Unsupported expression type: " + std::to_string(static_cast<int>(Expr->type)) + Location, 2, true, true, "");
    return nullptr;
}