#include "ExpressionGenerator.hh"
#include "IdentifierGenerator.hh"
#include "BinaryOpGenerator.hh"
#include "CastGenerator.hh"
#include "CallGenerator.hh"
#include "NumberGenerator.hh"

#include <cmath>
#include <functional>
#include <unordered_map>

static void InitializeBuiltinSymbols() {
    if (!Builtins.empty()) return;
    
    Builtins["print"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for print function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
        if (!printfFunc) {
            llvm::FunctionType* printfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        std::vector<llvm::Value*> printfArgs;
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for print" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArgValue->getType()->isIntegerTy(32)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isFloatingPointTy()) {
            if (ArgValue->getType()->isFloatTy()) {
                ArgValue = Builder.CreateFPExt(ArgValue, Builder.getDoubleTy());
            }
            llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isPointerTy()) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%s", "str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isIntegerTy(8)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%c", "char_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isIntegerTy(1)) {
            llvm::Value* boolAsInt = Builder.CreateZExt(ArgValue, Builder.getInt32Ty());
            llvm::Value* formatStr = Builder.CreateGlobalString("%d", "bool_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(boolAsInt);
        } else {
            Write("Expression Generation", "Unsupported argument type for print" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* CallResult = Builder.CreateCall(printfFunc, printfArgs);
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };
    
    Builtins["println"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for println function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
        if (!printfFunc) {
            llvm::FunctionType* printfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        std::vector<llvm::Value*> printfArgs;
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for println" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArgValue->getType()->isIntegerTy(32)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%d\n", "int_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isFloatingPointTy()) {
            if (ArgValue->getType()->isFloatTy()) {
                ArgValue = Builder.CreateFPExt(ArgValue, Builder.getDoubleTy());
            }
            llvm::Value* formatStr = Builder.CreateGlobalString("%.6f\n", "float_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isPointerTy()) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%s\n", "str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isIntegerTy(8)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%c\n", "char_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(ArgValue);
        } else if (ArgValue->getType()->isIntegerTy(1)) {
            llvm::Value* boolAsInt = Builder.CreateZExt(ArgValue, Builder.getInt32Ty());
            llvm::Value* formatStr = Builder.CreateGlobalString("%d\n", "bool_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(boolAsInt);
        } else {
            Write("Expression Generation", "Unsupported argument type for println" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* CallResult = Builder.CreateCall(printfFunc, printfArgs);
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };

    Builtins["input"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        llvm::Function* scanfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("scanf");
        if (!scanfFunc) {
            llvm::FunctionType* scanfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            scanfFunc = llvm::Function::Create(scanfType, llvm::Function::ExternalLinkage, "scanf", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Value* bufferSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 256);
        llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {bufferSize}, "input_buffer");
        if (!bufferPtr) {
            Write("Expression Generation", "Failed to allocate buffer for input function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* formatStr = Builder.CreateGlobalString(" %255[^\n]", "input_fmt");
        llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
        
        llvm::Value* CallResult = Builder.CreateCall(scanfFunc, {formatPtr, bufferPtr});
        return bufferPtr;
    };
    
    Builtins["type"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for type function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for type" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ArgType = ArgValue->getType();
        
        std::string typeStr;
        if (ArgType->isIntegerTy(32)) {
            typeStr = "int";
        } else if (ArgType->isIntegerTy(8)) {
            typeStr = "char";
        } else if (ArgType->isIntegerTy(1)) {
            typeStr = "bool";
        } else if (ArgType->isFloatTy()) {
            typeStr = "float";
        } else if (ArgType->isDoubleTy()) {
            typeStr = "double";
        } else if (ArgType->isPointerTy()) {
            typeStr = "string";
        } else {
            typeStr = "unknown";
            Write("Expression Generation", "Unsupported argument type for type function" + Location, 1, true, true, "");
        }
        
        llvm::Value* Result = Builder.CreateGlobalString(typeStr, "type_result");
        return Result;
    };

    Builtins["str"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for str function", 2, true, true, "");
            return nullptr;
        }
    
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for str" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ArgType = ArgValue->getType();
    
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
        if (!sprintfFunc) {
            llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
        if (!bufferPtr) {
            Write("Expression Generation", "Failed to allocate buffer for str function" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArgType->isIntegerTy(32)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ArgValue});
        } else if (ArgType->isIntegerTy(8)) {
            llvm::Value* extendedValue = Builder.CreateSExt(ArgValue, Builder.getInt32Ty());
            llvm::Value* formatStr = Builder.CreateGlobalString("%d", "char_to_str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, extendedValue});
        } else if (ArgType->isIntegerTy(1)) {
            llvm::Value* extendedValue = Builder.CreateZExt(ArgValue, Builder.getInt32Ty());
            llvm::Value* formatStr = Builder.CreateGlobalString("%d", "bool_to_str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, extendedValue});
        } else if (ArgType->isFloatingPointTy()) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            
            if (ArgValue->getType()->isFloatTy()) {
                ArgValue = Builder.CreateFPExt(ArgValue, Builder.getDoubleTy());
            }
            
            Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ArgValue});
        } else if (ArgType->isPointerTy()) {
            return ArgValue;
        } else {
            Write("Expression Generation", "Unsupported argument type for str function" + Location, 2, true, true, "");
            return nullptr;
        }
        
        return bufferPtr;
    };
}

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    InitializeBuiltinSymbols();
    
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
    } else if (Expr->type == NodeType::Array) {
        auto* ArrayNodePtr = static_cast<ArrayNode*>(Expr.get());
        if (!ArrayNodePtr) {
            Write("Expression Generation", "Failed to cast to ArrayNode" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArrayNodePtr->elements.empty()) {
            Write("Expression Generation", "Empty array literal" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ElementType = nullptr;
        if (!ArrayNodePtr->expectedType.empty() && ArrayNodePtr->expectedType != "auto") {
            ElementType = GetLLVMTypeFromString(ArrayNodePtr->expectedType, Builder.getContext());
            if (!ElementType) {
                Write("Expression Generation", "Invalid expected type: " + ArrayNodePtr->expectedType + Location, 2, true, true, "");
                return nullptr;
            }
        } else {
            llvm::Value* firstValue = GenerateExpression(ArrayNodePtr->elements[0], Builder, SymbolStack, Methods);
            if (!firstValue) {
                Write("Expression Generation", "Cannot deduce array element type" + Location, 2, true, true, "");
                return nullptr;
            }
            ElementType = firstValue->getType();
            
            if (ElementType->isFloatingPointTy()) {
                if (llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(firstValue)) {
                    double val = constFP->getValueAPF().convertToDouble();
                    if (val == floor(val)) {
                        ElementType = Builder.getInt32Ty();
                    } else {
                        ElementType = Builder.getDoubleTy();
                    }
                } else {
                    ElementType = Builder.getDoubleTy();
                }
            } else if (ElementType->isIntegerTy()) {
                ElementType = Builder.getInt32Ty();
            } else if (ElementType->isPointerTy()) {
                ElementType = llvm::PointerType::get(Builder.getInt8Ty(), 0);
            }
        }
        
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        uint64_t arraySize = ArrayNodePtr->elements.size();
        uint64_t elementSize = Builder.GetInsertBlock()->getParent()->getParent()->getDataLayout().getTypeAllocSize(ElementType);
        llvm::Value* totalSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), arraySize * elementSize);
        
        llvm::Value* arrayPtr = Builder.CreateCall(mallocFunc, {totalSize});
        if (!arrayPtr) {
            Write("Expression Generation", "Failed to allocate memory for array" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* typedPtrType = llvm::PointerType::get(ElementType, 0);
        llvm::Value* typedArrayPtr = Builder.CreatePointerCast(arrayPtr, typedPtrType);
        
        for (size_t i = 0; i < ArrayNodePtr->elements.size(); ++i) {
            llvm::Value* elementValue = GenerateExpression(ArrayNodePtr->elements[i], Builder, SymbolStack, Methods);
            if (!elementValue) {
                Write("Expression Generation", "Failed to generate array element at index " + std::to_string(i) + Location, 2, true, true, "");
                continue;
            }
            
            if (elementValue->getType() != ElementType) {
                if (ElementType->isIntegerTy(32) && elementValue->getType()->isFloatingPointTy()) {
                    elementValue = Builder.CreateFPToSI(elementValue, ElementType);
                } else if (ElementType->isFloatTy()) {
                    if (elementValue->getType()->isIntegerTy()) {
                        elementValue = Builder.CreateSIToFP(elementValue, ElementType);
                    } else if (elementValue->getType()->isDoubleTy()) {
                        elementValue = Builder.CreateFPTrunc(elementValue, ElementType);
                    }
                } else if (ElementType->isDoubleTy()) {
                    if (elementValue->getType()->isIntegerTy()) {
                        elementValue = Builder.CreateSIToFP(elementValue, ElementType);
                    } else if (elementValue->getType()->isFloatTy()) {
                        elementValue = Builder.CreateFPExt(elementValue, ElementType);
                    }
                }
            }
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(ElementType, typedArrayPtr, 
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), i));
            Builder.CreateStore(elementValue, elementPtr);
        }
        
        return typedArrayPtr;
    } else if (Expr->type == NodeType::ArrayAccess) {
        auto* AccessNodePtr = static_cast<ArrayAccessNode*>(Expr.get());
        if (!AccessNodePtr) {
            Write("Expression Generation", "Failed to cast to ArrayAccessNode" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* arrayPtr = nullptr;
        for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
            auto found = it->find(AccessNodePtr->identifier);
            if (found != it->end()) {
                arrayPtr = found->second;
                break;
            }
        }
        
        if (!arrayPtr) {
            Write("Expression Generation", "Undefined array identifier: " + AccessNodePtr->identifier + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
        if (!allocaInst) {
            Write("Expression Generation", "Array identifier is not an allocated variable: " + AccessNodePtr->identifier + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* allocatedType = allocaInst->getAllocatedType();
        
        if (AccessNodePtr->expr->type == NodeType::Array) {
            auto* IndexArrayPtr = static_cast<ArrayNode*>(AccessNodePtr->expr.get());
            std::vector<llvm::Value*> indices;
            indices.push_back(llvm::ConstantInt::get(Builder.getInt32Ty(), 0));
            
            llvm::Type* currentType = allocatedType;
            for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                llvm::Value* indexValue = GenerateExpression(IndexArrayPtr->elements[i], Builder, SymbolStack, Methods);
                if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                    Write("Expression Generation", "Invalid array index" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                if (!currentType->isArrayTy()) {
                    Write("Expression Generation", "Too many dimensions in array access: " + AccessNodePtr->identifier + Location, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                if (!arrayType) {
                    Write("Expression Generation", "Expected array type" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                    uint64_t index = constIndex->getZExtValue();
                    if (index >= arraySize) {
                        Write("Expression Generation", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + Location, 2, true, true, "");
                        return nullptr;
                    }
                }
                
                indices.push_back(indexValue);
                currentType = arrayType->getElementType();
            }
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            return Builder.CreateLoad(currentType, elementPtr);
            
        } else {
            llvm::Value* indexValue = GenerateExpression(AccessNodePtr->expr, Builder, SymbolStack, Methods);
            if (!indexValue || !indexValue->getType()->isIntegerTy()) {
                Write("Expression Generation", "Invalid array index" + Location, 2, true, true, "");
                return nullptr;
            }
            
            if (!allocatedType->isArrayTy()) {
                Write("Expression Generation", "Variable is not an array: " + AccessNodePtr->identifier + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
            if (!arrayType) {
                Write("Expression Generation", "Expected array type" + Location, 2, true, true, "");
                return nullptr;
            }
            
            uint64_t arraySize = arrayType->getNumElements();
            if (llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                uint64_t index = constIndex->getZExtValue();
                if (index >= arraySize) {
                    Write("Expression Generation", "Array index " + std::to_string(index) + " out of bounds for array of size " + std::to_string(arraySize) + Location, 2, true, true, "");
                    return nullptr;
                }
            }
            
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(Builder.getInt32Ty(), 0),
                indexValue
            };
            
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(allocatedType, arrayPtr, indices);
            return Builder.CreateLoad(arrayType->getElementType(), elementPtr);
        }
    }
       
    Write("Expression Generation", "Unsupported expression type: " + std::to_string(Expr->type) + Location, 2, true, true, "");
    return nullptr;
}