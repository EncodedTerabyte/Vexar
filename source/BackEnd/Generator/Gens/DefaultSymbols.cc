#include "ExpressionGenerator.hh"
#include "DefaultSymbols.hh"
#include "GarbageCollector.hh"

void InitializeBuiltinSymbols(BuiltinSymbols& Builtins) {
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
            llvm::Value* zero = llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
            llvm::Value* isZero = Builder.CreateICmpEQ(boolAsInt, zero);
            
            llvm::Value* trueStr = Builder.CreateGlobalString("true", "true_str");
            llvm::Value* falseStr = Builder.CreateGlobalString("false", "false_str");
            llvm::Value* selectedStr = Builder.CreateSelect(isZero, falseStr, trueStr);
            
            llvm::Value* formatStr = Builder.CreateGlobalString("%s", "bool_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(selectedStr);
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
            llvm::Value* zero = llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
            llvm::Value* isZero = Builder.CreateICmpEQ(boolAsInt, zero);
            
            llvm::Value* trueStr = Builder.CreateGlobalString("true\n", "true_str_ln");
            llvm::Value* falseStr = Builder.CreateGlobalString("false\n", "false_str_ln");
            llvm::Value* selectedStr = Builder.CreateSelect(isZero, falseStr, trueStr);
            
            llvm::Value* formatStr = Builder.CreateGlobalString("%s", "bool_fmt_ln");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            printfArgs.push_back(formatPtr);
            printfArgs.push_back(selectedStr);
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
        
        llvm::Value* bufferSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 256);
        llvm::Value* bufferPtr = gc.GenerateTrackedMalloc(Builder, bufferSize);
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

        llvm::Value* bufferPtr = gc.GenerateTrackedMalloc(Builder, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32));
        if (!bufferPtr) {
            Write("Expression Generation", "Failed to allocate buffer for str function" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArgType->isIntegerTy(32)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ArgValue});
        } else if (ArgType->isIntegerTy(8)) {
            llvm::Value* formatStr = Builder.CreateGlobalString("%c", "char_to_str_fmt");
            llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
            Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ArgValue});
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

    Builtins["int"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for int function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for int" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ArgType = ArgValue->getType();
        
        if (ArgType->isIntegerTy(32)) {
            return ArgValue;
        } else if (ArgType->isFloatingPointTy()) {
            return Builder.CreateFPToSI(ArgValue, Builder.getInt32Ty());
        } else if (ArgType->isIntegerTy(8)) {
            return Builder.CreateSExt(ArgValue, Builder.getInt32Ty());
        } else if (ArgType->isIntegerTy(1)) {
            return Builder.CreateZExt(ArgValue, Builder.getInt32Ty());
        } else if (ArgType->isPointerTy()) {
            llvm::Function* atoiFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("atoi");
            if (!atoiFunc) {
                llvm::FunctionType* atoiType = llvm::FunctionType::get(
                    llvm::Type::getInt32Ty(Builder.getContext()),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    false
                );
                atoiFunc = llvm::Function::Create(atoiType, llvm::Function::ExternalLinkage, "atoi", Builder.GetInsertBlock()->getParent()->getParent());
            }
            return Builder.CreateCall(atoiFunc, {ArgValue});
        } else {
            Write("Expression Generation", "Unsupported argument type for int function" + Location, 2, true, true, "");
            return nullptr;
        }
    };

    Builtins["float"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for float function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for float" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ArgType = ArgValue->getType();
        
        if (ArgType->isFloatTy()) {
            return ArgValue;
        } else if (ArgType->isDoubleTy()) {
            return Builder.CreateFPTrunc(ArgValue, Builder.getFloatTy());
        } else if (ArgType->isIntegerTy()) {
            return Builder.CreateSIToFP(ArgValue, Builder.getFloatTy());
        } else if (ArgType->isPointerTy()) {
            llvm::Function* atofFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("atof");
            if (!atofFunc) {
                llvm::FunctionType* atofType = llvm::FunctionType::get(
                    llvm::Type::getDoubleTy(Builder.getContext()),
                    {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                    false
                );
                atofFunc = llvm::Function::Create(atofType, llvm::Function::ExternalLinkage, "atof", Builder.GetInsertBlock()->getParent()->getParent());
            }
            llvm::Value* doubleResult = Builder.CreateCall(atofFunc, {ArgValue});
            return Builder.CreateFPTrunc(doubleResult, Builder.getFloatTy());
        } else {
            Write("Expression Generation", "Unsupported argument type for float function" + Location, 2, true, true, "");
            return nullptr;
        }
    };

    Builtins["len"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for len function", 2, true, true, "");
            return nullptr;
        }
        
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (args[0]->type == NodeType::ArrayAccess) {
            auto* AccessNodePtr = static_cast<ArrayAccessNode*>(args[0].get());
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
            
            if (allocatedType->isPointerTy()) {
                llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
                return gc.GenerateGetSize(Builder, loadedPtr);
            }
            
            if (AccessNodePtr->expr->type == NodeType::Array) {
                auto* IndexArrayPtr = static_cast<ArrayNode*>(AccessNodePtr->expr.get());
                
                llvm::Type* currentType = allocatedType;
                for (size_t i = 0; i < IndexArrayPtr->elements.size(); ++i) {
                    if (!currentType->isArrayTy()) {
                        Write("Expression Generation", "Too many dimensions in array access for len: " + AccessNodePtr->identifier + Location, 2, true, true, "");
                        return nullptr;
                    }
                    
                    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                    if (!arrayType) {
                        Write("Expression Generation", "Expected array type in len" + Location, 2, true, true, "");
                        return nullptr;
                    }
                    
                    currentType = arrayType->getElementType();
                }
                
                if (currentType->isArrayTy()) {
                    llvm::ArrayType* finalArrayType = llvm::dyn_cast<llvm::ArrayType>(currentType);
                    if (!finalArrayType) {
                        Write("Expression Generation", "Expected final array type in len" + Location, 2, true, true, "");
                        return nullptr;
                    }
                    
                    uint64_t arraySize = finalArrayType->getNumElements();
                    return llvm::ConstantInt::get(Builder.getInt32Ty(), arraySize);
                } else {
                    Write("Expression Generation", "Result of array access is not an array for len function" + Location, 2, true, true, "");
                    return nullptr;
                }
            } else {
                if (allocatedType->isPointerTy()) {
                    llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
                    return gc.GenerateGetSize(Builder, loadedPtr);
                }
                
                if (!allocatedType->isArrayTy()) {
                    Write("Expression Generation", "Variable is not an array for len: " + AccessNodePtr->identifier + Location, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                if (!arrayType) {
                    Write("Expression Generation", "Expected array type for len" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                llvm::Type* elementType = arrayType->getElementType();
                if (elementType->isArrayTy()) {
                    llvm::ArrayType* subArrayType = llvm::dyn_cast<llvm::ArrayType>(elementType);
                    if (!subArrayType) {
                        Write("Expression Generation", "Expected sub-array type for len" + Location, 2, true, true, "");
                        return nullptr;
                    }
                    
                    uint64_t arraySize = subArrayType->getNumElements();
                    return llvm::ConstantInt::get(Builder.getInt32Ty(), arraySize);
                } else {
                    Write("Expression Generation", "Array element is not an array for len function" + Location, 2, true, true, "");
                    return nullptr;
                }
            }
        }
        
        if (args[0]->type == NodeType::Identifier) {
            auto* IdentifierNodePtr = static_cast<IdentifierNode*>(args[0].get());
            if (!IdentifierNodePtr) {
                Write("Expression Generation", "Failed to cast to IdentifierNode" + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::Value* varPtr = nullptr;
            for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
                auto found = it->find(IdentifierNodePtr->name);
                if (found != it->end()) {
                    varPtr = found->second;
                    break;
                }
            }
            
            if (!varPtr) {
                Write("Expression Generation", "Undefined identifier: " + IdentifierNodePtr->name + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(varPtr);
            if (!allocaInst) {
                Write("Expression Generation", "Identifier is not an allocated variable: " + IdentifierNodePtr->name + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::Type* allocatedType = allocaInst->getAllocatedType();
            
            if (allocatedType->isPointerTy()) {
                llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
                if (loadedPtr->getType() == llvm::PointerType::get(Builder.getInt8Ty(), 0)) {
                    llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
                    if (!strlenFunc) {
                        llvm::FunctionType* strlenType = llvm::FunctionType::get(
                            llvm::Type::getInt64Ty(Builder.getContext()),
                            {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                            false
                        );
                        strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
                    }
                    llvm::Value* length = Builder.CreateCall(strlenFunc, {loadedPtr});
                    return Builder.CreateTrunc(length, Builder.getInt32Ty());
                } else {
                    llvm::Value* sizeBytes = gc.GenerateGetSize(Builder, loadedPtr);
                    llvm::Value* elementSize = llvm::ConstantInt::get(Builder.getInt32Ty(), 4);
                    return Builder.CreateUDiv(sizeBytes, elementSize);
                }
            }
            
            if (allocatedType->isArrayTy()) {
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                if (!arrayType) {
                    Write("Expression Generation", "Expected array type" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                return llvm::ConstantInt::get(Builder.getInt32Ty(), arraySize);
            }
            
            Write("Expression Generation", "Unsupported type for len function: " + IdentifierNodePtr->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for len" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArgValue->getType()->isPointerTy()) {
            if (ArgValue->getType() == llvm::PointerType::get(Builder.getInt8Ty(), 0)) {
                llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
                if (!strlenFunc) {
                    llvm::FunctionType* strlenType = llvm::FunctionType::get(
                        llvm::Type::getInt64Ty(Builder.getContext()),
                        {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                        false
                    );
                    strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
                }
                llvm::Value* length = Builder.CreateCall(strlenFunc, {ArgValue});
                return Builder.CreateTrunc(length, Builder.getInt32Ty());
            } else {
                llvm::Value* sizeBytes = gc.GenerateGetSize(Builder, ArgValue);
                llvm::Value* elementSize = llvm::ConstantInt::get(Builder.getInt32Ty(), 4);
                return Builder.CreateUDiv(sizeBytes, elementSize);
            }
        } else {
            Write("Expression Generation", "Unsupported argument type for len function" + Location, 2, true, true, "");
            return nullptr;
        }
    };

    Builtins["malloc"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for malloc function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* elementsValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        if (!elementsValue || !elementsValue->getType()->isIntegerTy()) {
            Write("Expression Generation", "Invalid elements argument for malloc", 2, true, true, "");
            return nullptr;
        }
        
        if (elementsValue->getType() != Builder.getInt64Ty()) {
            elementsValue = Builder.CreateZExt(elementsValue, Builder.getInt64Ty());
        }
        
        llvm::Value* sizePerElement = llvm::ConstantInt::get(Builder.getInt64Ty(), 4);
        llvm::Value* totalSize = Builder.CreateMul(elementsValue, sizePerElement);
        
        return gc.GenerateTrackedMalloc(Builder, totalSize);
    };

    Builtins["arraysize"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for arraysize function", 2, true, true, "");
            return nullptr;
        }
        
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (args[0]->type == NodeType::Identifier) {
            auto* IdentifierNodePtr = static_cast<IdentifierNode*>(args[0].get());
            if (!IdentifierNodePtr) {
                Write("Expression Generation", "Failed to cast to IdentifierNode" + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::Value* varPtr = nullptr;
            for (auto it = SymbolStack.rbegin(); it != SymbolStack.rend(); ++it) {
                auto found = it->find(IdentifierNodePtr->name);
                if (found != it->end()) {
                    varPtr = found->second;
                    break;
                }
            }
            
            if (!varPtr) {
                Write("Expression Generation", "Undefined identifier: " + IdentifierNodePtr->name + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(varPtr);
            if (!allocaInst) {
                Write("Expression Generation", "Identifier is not an allocated variable: " + IdentifierNodePtr->name + Location, 2, true, true, "");
                return nullptr;
            }
            
            llvm::Type* allocatedType = allocaInst->getAllocatedType();
            
            if (allocatedType->isPointerTy()) {
                llvm::Value* loadedPtr = Builder.CreateLoad(allocatedType, allocaInst);
                llvm::Value* sizeBytes = gc.GenerateGetSize(Builder, loadedPtr);
                llvm::Value* elementSize = llvm::ConstantInt::get(Builder.getInt32Ty(), 4);
                return Builder.CreateUDiv(sizeBytes, elementSize);
            }
            
            if (allocatedType->isArrayTy()) {
                llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                if (!arrayType) {
                    Write("Expression Generation", "Expected array type" + Location, 2, true, true, "");
                    return nullptr;
                }
                
                uint64_t arraySize = arrayType->getNumElements();
                return llvm::ConstantInt::get(Builder.getInt32Ty(), arraySize);
            }
            
            Write("Expression Generation", "Unsupported type for arraysize function: " + IdentifierNodePtr->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for arraysize" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (ArgValue->getType()->isPointerTy()) {
            llvm::Value* sizeBytes = gc.GenerateGetSize(Builder, ArgValue);
            llvm::Value* elementSize = llvm::ConstantInt::get(Builder.getInt32Ty(), 4);
            return Builder.CreateUDiv(sizeBytes, elementSize);
        } else {
            Write("Expression Generation", "Unsupported argument type for arraysize function" + Location, 2, true, true, "");
            return nullptr;
        }
    };

    Builtins["char"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for char function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for char" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ArgType = ArgValue->getType();
        
        if (ArgType->isIntegerTy(8)) {
            return ArgValue;
        } else if (ArgType->isIntegerTy()) {
            return Builder.CreateTrunc(ArgValue, Builder.getInt8Ty());
        } else {
            Write("Expression Generation", "Unsupported argument type for char function" + Location, 2, true, true, "");
            return nullptr;
        }
    };

    Builtins["exit"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        llvm::Function* exitFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("exit");
        if (!exitFunc) {
            llvm::FunctionType* exitType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()),
                {llvm::Type::getInt32Ty(Builder.getContext())},
                false
            );
            exitFunc = llvm::Function::Create(exitType, llvm::Function::ExternalLinkage, "exit", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Value* exitCode = llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
        if (!args.empty()) {
            llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
            if (ArgValue && ArgValue->getType()->isIntegerTy(32)) {
                exitCode = ArgValue;
            }
        }
        
        Builder.CreateCall(exitFunc, {exitCode});
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };

    Builtins["bool"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for bool function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        std::string Location = " at line " + std::to_string(args[0]->token.line) + ", column " + std::to_string(args[0]->token.column);
        
        if (!ArgValue) {
            Write("Expression Generation", "Invalid argument expression for bool" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* ArgType = ArgValue->getType();
        
        if (ArgType->isIntegerTy(1)) {
            return ArgValue;
        } else if (ArgType->isIntegerTy()) {
            return Builder.CreateICmpNE(ArgValue, llvm::ConstantInt::get(ArgType, 0));
        } else if (ArgType->isFloatingPointTy()) {
            return Builder.CreateFCmpONE(ArgValue, llvm::ConstantFP::get(ArgType, 0.0));
        } else if (ArgType->isPointerTy()) {
            return Builder.CreateICmpNE(ArgValue, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ArgType)));
        } else {
            Write("Expression Generation", "Unsupported argument type for bool function" + Location, 2, true, true, "");
            return nullptr;
        }
    };

    Builtins["free"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for free function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ptrValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        if (!ptrValue || !ptrValue->getType()->isPointerTy()) {
            Write("Expression Generation", "Invalid pointer argument for free function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Function* freeFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("free");
        if (!freeFunc) {
            llvm::FunctionType* freeType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                false
            );
            freeFunc = llvm::Function::Create(freeType, llvm::Function::ExternalLinkage, "free", Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        Builder.CreateCall(freeFunc, {ptrValue});
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };

    Builtins["gc_collect"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        gc.GenerateGCCall(Builder);
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };
    
    Builtins["free_deep"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for free_deep function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ptrValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        if (!ptrValue || !ptrValue->getType()->isPointerTy()) {
            Write("Expression Generation", "Invalid pointer argument for free_deep function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Function* freePtrArrayFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("free_ptr_array");
        llvm::Function* freeFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("free");
        
        if (!freePtrArrayFunc) {
            llvm::FunctionType* freePtrArrayType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()),
                {llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0),
                 llvm::Type::getInt32Ty(Builder.getContext())},
                false
            );
            freePtrArrayFunc = llvm::Function::Create(freePtrArrayType, llvm::Function::ExternalLinkage, "free_ptr_array", 
                Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        if (!freeFunc) {
            llvm::FunctionType* freeType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                false
            );
            freeFunc = llvm::Function::Create(freeType, llvm::Function::ExternalLinkage, "free", 
                Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Value* ptrArrayPtr = Builder.CreatePointerCast(ptrValue, 
            llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0));
        
        llvm::Value* arraySize = llvm::ConstantInt::get(Builder.getInt32Ty(), 2);
        Builder.CreateCall(freePtrArrayFunc, {ptrArrayPtr, arraySize});
        Builder.CreateCall(freeFunc, {ptrValue});
        
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };
    
    Builtins["malloc_tracked"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for malloc_tracked function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* sizeValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        if (!sizeValue || !sizeValue->getType()->isIntegerTy()) {
            Write("Expression Generation", "Invalid size argument for malloc_tracked", 2, true, true, "");
            return nullptr;
        }
        
        if (sizeValue->getType() != Builder.getInt64Ty()) {
            sizeValue = Builder.CreateZExt(sizeValue, Builder.getInt64Ty());
        }
        
        return gc.GenerateTrackedMalloc(Builder, sizeValue);
    };

    InitializeGCBuiltins(Builtins, gc);
}