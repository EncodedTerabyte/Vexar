#include "ExpressionGenerator.hh"
#include <cmath>
#include <unordered_map>
#include <functional>

using BuiltinHandler = std::function<llvm::Value*(const std::vector<std::unique_ptr<ASTNode>>&, llvm::IRBuilder<>&, ScopeStack&, FunctionSymbols&)>;

static std::unordered_map<std::string, BuiltinHandler> builtins;

static void InitializeBuiltins() {
    if (!builtins.empty()) return;
    
    builtins["print"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) exit(1);
        
        llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
        if (!printfFunc) {
            llvm::FunctionType* printfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", 
                                              Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        std::vector<llvm::Value*> printfArgs;
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        
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
            exit(1);
        }
        
        Builder.CreateCall(printfFunc, printfArgs);
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };
    
    builtins["println"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) exit(1);
        
        llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
        if (!printfFunc) {
            llvm::FunctionType* printfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", 
                                              Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        std::vector<llvm::Value*> printfArgs;
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        
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
            exit(1);
        }
        
        Builder.CreateCall(printfFunc, printfArgs);
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };

    builtins["input"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        llvm::Function* scanfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("scanf");
        if (!scanfFunc) {
            llvm::FunctionType* scanfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            scanfFunc = llvm::Function::Create(scanfType, llvm::Function::ExternalLinkage, "scanf", 
                                            Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                            Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Value* bufferSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 256);
        llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {bufferSize}, "input_buffer");
        
        llvm::Value* formatStr = Builder.CreateGlobalString(" %255[^\n]", "input_fmt");
        llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
        
        Builder.CreateCall(scanfFunc, {formatPtr, bufferPtr});
        
        return bufferPtr;
    };
    
    builtins["type"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) exit(1);
        
        llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
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
        }
        
        return Builder.CreateGlobalString(typeStr, "type_result");
    };

    builtins["str"] = [](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
    if (args.empty()) exit(1);
    
    llvm::Value* ArgValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
    llvm::Type* ArgType = ArgValue->getType();
    
    llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
    if (!mallocFunc) {
        llvm::FunctionType* mallocType = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
            {llvm::Type::getInt64Ty(Builder.getContext())},
            false
        );
        mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                          Builder.GetInsertBlock()->getParent()->getParent());
    }

    llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
        if (!sprintfFunc) {
            llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", 
                                            Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
        
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
            exit(1);
        }
        
        return bufferPtr;
    };
}

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    InitializeBuiltins();
    
    if (Expr->type == NodeType::Number) {
        auto* NumNode = static_cast<NumberNode*>(Expr.get());
        
        if (NumNode->value == floor(NumNode->value)) {
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), (int)NumNode->value);
        } else {
            return llvm::ConstantFP::get(Builder.getContext(), llvm::APFloat(NumNode->value));
        }
    } else if (Expr->type == NodeType::String) {
       auto* StrNode = static_cast<StringNode*>(Expr.get());
       return Builder.CreateGlobalString(StrNode->value, "", 0, nullptr);
   } else if (Expr->type == NodeType::Character) {
       auto* CharNode = static_cast<CharacterNode*>(Expr.get());
       return llvm::ConstantInt::get(llvm::Type::getInt8Ty(Builder.getContext()), CharNode->value);
   } else if (Expr->type == NodeType::Paren) {
        auto* ParenNodePtr = static_cast<ParenNode*>(Expr.get());
        return GenerateExpression(ParenNodePtr->inner, Builder, SymbolStack, Methods);
   } else if (Expr->type == NodeType::BinaryOp) {
       auto* BinOpNode = static_cast<BinaryOpNode*>(Expr.get());
       
       llvm::Value* Left = GenerateExpression(BinOpNode->left, Builder, SymbolStack, Methods);
       llvm::Value* Right = GenerateExpression(BinOpNode->right, Builder, SymbolStack, Methods);

       if (BinOpNode->op == "+") {
           if (Left->getType()->isPointerTy() && Right->getType()->isPointerTy()) {
               llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
               if (!strlenFunc) {
                   llvm::FunctionType* strlenType = llvm::FunctionType::get(
                       llvm::Type::getInt64Ty(Builder.getContext()),
                       {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                       false
                   );
                   strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
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

               llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
               if (!strcpyFunc) {
                   llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                       llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                       {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                       false
                   );
                   strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
               }

               llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
               if (!strcatFunc) {
                   llvm::FunctionType* strcatType = llvm::FunctionType::get(
                       llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                       {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                       false
                   );
                   strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
               }

               llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
               llvm::Value* rightLen = Builder.CreateCall(strlenFunc, {Right});
               llvm::Value* totalLen = Builder.CreateAdd(leftLen, rightLen);
               llvm::Value* totalLenPlusOne = Builder.CreateAdd(totalLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));

               llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {totalLenPlusOne});
               
               Builder.CreateCall(strcpyFunc, {resultPtr, Left});
               Builder.CreateCall(strcatFunc, {resultPtr, Right});
               
               return resultPtr;
           }
           
           if (Left->getType()->isPointerTy() && !Right->getType()->isPointerTy()) {
               llvm::Value* rightStr = nullptr;
               if (Right->getType()->isIntegerTy()) {
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

                   rightStr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
                   llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
                   llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
                   Builder.CreateCall(sprintfFunc, {rightStr, formatPtr, Right});
               } else if (Right->getType()->isFloatingPointTy()) {
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

                   rightStr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
                   llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
                   llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
                   
                   if (Right->getType()->isFloatTy()) {
                       Right = Builder.CreateFPExt(Right, Builder.getDoubleTy());
                   }
                   
                   Builder.CreateCall(sprintfFunc, {rightStr, formatPtr, Right});
               }
               
               if (rightStr) {
                   llvm::Function* strlenFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strlen");
                   if (!strlenFunc) {
                       llvm::FunctionType* strlenType = llvm::FunctionType::get(
                           llvm::Type::getInt64Ty(Builder.getContext()),
                           {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                           false
                       );
                       strlenFunc = llvm::Function::Create(strlenType, llvm::Function::ExternalLinkage, "strlen", Builder.GetInsertBlock()->getParent()->getParent());
                   }

                   llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
                   llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
                   if (!strcpyFunc) {
                       llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                           llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                           {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                           false
                       );
                       strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
                   }

                   llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
                   if (!strcatFunc) {
                       llvm::FunctionType* strcatType = llvm::FunctionType::get(
                           llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                           {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                           false
                       );
                       strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
                   }

                   llvm::Value* leftLen = Builder.CreateCall(strlenFunc, {Left});
                   llvm::Value* rightLen = Builder.CreateCall(strlenFunc, {rightStr});
                   llvm::Value* totalLen = Builder.CreateAdd(leftLen, rightLen);
                   llvm::Value* totalLenPlusOne = Builder.CreateAdd(totalLen, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));

                   llvm::Value* resultPtr = Builder.CreateCall(mallocFunc, {totalLenPlusOne});
                   
                   Builder.CreateCall(strcpyFunc, {resultPtr, Left});
                   Builder.CreateCall(strcatFunc, {resultPtr, rightStr});
                   
                   return resultPtr;
               }
           }
           
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFAdd(Left, Right, "addtmp");
           } else {
               return Builder.CreateAdd(Left, Right, "addtmp");
           }
       } else if (BinOpNode->op == "-") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFSub(Left, Right, "subtmp");
           } else {
               return Builder.CreateSub(Left, Right, "subtmp");
           }
       } else if (BinOpNode->op == "*") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFMul(Left, Right, "multmp");
           } else {
               return Builder.CreateMul(Left, Right, "multmp");
           }
       } else if (BinOpNode->op == "/") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFDiv(Left, Right, "divtmp");
           } else {
               return Builder.CreateSDiv(Left, Right, "divtmp");
           }
       } else if (BinOpNode->op == ">=") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFCmpOGE(Left, Right, "fcmp_ge");
           } else {
               return Builder.CreateICmpSGE(Left, Right, "icmp_ge");
           }
       } else if (BinOpNode->op == "<=") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFCmpOLE(Left, Right, "fcmp_le");
           } else {
               return Builder.CreateICmpSLE(Left, Right, "icmp_le");
           }
       } else if (BinOpNode->op == ">") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFCmpOGT(Left, Right, "fcmp_gt");
           } else {
               return Builder.CreateICmpSGT(Left, Right, "icmp_gt");
           }
       } else if (BinOpNode->op == "<") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFCmpOLT(Left, Right, "fcmp_lt");
           } else {
               return Builder.CreateICmpSLT(Left, Right, "icmp_lt");
           }
       } else if (BinOpNode->op == "==") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFCmpOEQ(Left, Right, "fcmp_eq");
           } else {
               return Builder.CreateICmpEQ(Left, Right, "icmp_eq");
           }
       } else if (BinOpNode->op == "!=") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               if (Left->getType()->isIntegerTy()) {
                   Left = Builder.CreateSIToFP(Left, Right->getType());
               } else if (Right->getType()->isIntegerTy()) {
                   Right = Builder.CreateSIToFP(Right, Left->getType());
               }
               return Builder.CreateFCmpONE(Left, Right, "fcmp_ne");
           } else {
               return Builder.CreateICmpNE(Left, Right, "icmp_ne");
           }
       } else {
           exit(1);
       }
   } else if (Expr->type == NodeType::Identifier) {
       auto* Identifier = static_cast<IdentifierNode*>(Expr.get());
       
       llvm::AllocaInst* AllocaInst = FindInScopes(SymbolStack, Identifier->name);

       if (AllocaInst) {
           return Builder.CreateLoad(AllocaInst->getAllocatedType(), AllocaInst, Identifier->name.c_str());
       } else {
           return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), 0);
       }
   } else if (Expr->type == NodeType::FunctionCall) {
       auto* FuncCallNode = static_cast<FunctionCallNode*>(Expr.get());
       
       auto builtinIt = builtins.find(FuncCallNode->name);
       if (builtinIt != builtins.end()) {
           return builtinIt->second(FuncCallNode->arguments, Builder, SymbolStack, Methods);
       }
       
       auto it = Methods.find(FuncCallNode->name);
       if (it == Methods.end()) {
           exit(1);
       }
       
       llvm::Function* Function = it->second;
       llvm::FunctionType* FuncType = Function->getFunctionType();
       
       std::vector<llvm::Value*> Args;
       for (size_t i = 0; i < FuncCallNode->arguments.size(); ++i) {
           llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[i], Builder, SymbolStack, Methods);
           
           if (i < FuncType->getNumParams()) {
               llvm::Type* ExpectedType = FuncType->getParamType(i);
               if (ArgValue->getType() != ExpectedType) {
                   if (ExpectedType->isIntegerTy(32) && ArgValue->getType()->isFloatingPointTy()) {
                       if (llvm::ConstantFP* constFP = llvm::dyn_cast<llvm::ConstantFP>(ArgValue)) {
                           double val = constFP->getValueAPF().convertToDouble();
                           if (val == floor(val)) {
                               ArgValue = Builder.CreateFPToSI(ArgValue, ExpectedType);
                           } else {
                               ArgValue = Builder.CreateFPToSI(ArgValue, ExpectedType);
                           }
                       } else {
                           ArgValue = Builder.CreateFPToSI(ArgValue, ExpectedType);
                       }
                   } else if (ExpectedType->isFloatingPointTy() && ArgValue->getType()->isIntegerTy()) {
                       ArgValue = Builder.CreateSIToFP(ArgValue, ExpectedType);
                   } else if (ExpectedType->isFloatTy() && ArgValue->getType()->isDoubleTy()) {
                       ArgValue = Builder.CreateFPTrunc(ArgValue, ExpectedType);
                   } else if (ExpectedType->isDoubleTy() && ArgValue->getType()->isFloatTy()) {
                       ArgValue = Builder.CreateFPExt(ArgValue, ExpectedType);
                   }
               }
           }
           
           Args.push_back(ArgValue);
       }
       
       return Builder.CreateCall(Function, Args);
    } else if (Expr->type == NodeType::Cast) {
       auto* CastNodePtr = static_cast<CastNode*>(Expr.get());
       llvm::Value* ExprValue = GenerateExpression(CastNodePtr->expr, Builder, SymbolStack, Methods);
       
       llvm::Type* TargetType = nullptr;
       if (CastNodePtr->targetType == "int") {
           TargetType = llvm::Type::getInt32Ty(Builder.getContext());
       } else if (CastNodePtr->targetType == "float") {
           TargetType = llvm::Type::getFloatTy(Builder.getContext());
       } else if (CastNodePtr->targetType == "double") {
           TargetType = llvm::Type::getDoubleTy(Builder.getContext());
       } else if (CastNodePtr->targetType == "char") {
           TargetType = llvm::Type::getInt8Ty(Builder.getContext());
       } else if (CastNodePtr->targetType == "bool") {
           TargetType = llvm::Type::getInt1Ty(Builder.getContext());
       } else if (CastNodePtr->targetType == "string") {
           TargetType = llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0);
       } else {
           exit(1);
       }
       
       llvm::Type* SourceType = ExprValue->getType();
       
       if (SourceType == TargetType) {
           return ExprValue;
       }
       
       if (SourceType->isIntegerTy() && TargetType->isIntegerTy()) {
           if (SourceType->getIntegerBitWidth() < TargetType->getIntegerBitWidth()) {
               return Builder.CreateSExt(ExprValue, TargetType);
           } else if (SourceType->getIntegerBitWidth() > TargetType->getIntegerBitWidth()) {
               return Builder.CreateTrunc(ExprValue, TargetType);
           }
       } else if (SourceType->isIntegerTy() && TargetType->isFloatingPointTy()) {
           return Builder.CreateSIToFP(ExprValue, TargetType);
       } else if (SourceType->isFloatingPointTy() && TargetType->isIntegerTy()) {
           return Builder.CreateFPToSI(ExprValue, TargetType);
       } else if (SourceType->isFloatingPointTy() && TargetType->isFloatingPointTy()) {
           if (SourceType->isFloatTy() && TargetType->isDoubleTy()) {
               return Builder.CreateFPExt(ExprValue, TargetType);
           } else if (SourceType->isDoubleTy() && TargetType->isFloatTy()) {
               return Builder.CreateFPTrunc(ExprValue, TargetType);
           }
       } else if (SourceType->isIntegerTy(8) && TargetType->isPointerTy()) {
           llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
           if (!mallocFunc) {
               llvm::FunctionType* mallocType = llvm::FunctionType::get(
                   llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                   {llvm::Type::getInt64Ty(Builder.getContext())},
                   false
               );
               mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                                 Builder.GetInsertBlock()->getParent()->getParent());
           }
           llvm::Value* charPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 2)});
           Builder.CreateStore(ExprValue, charPtr);
           llvm::Value* nullPtr = Builder.CreateGEP(llvm::Type::getInt8Ty(Builder.getContext()), charPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));
           Builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), nullPtr);
           return charPtr;
       } else if (SourceType->isPointerTy() && TargetType->isIntegerTy()) {
           llvm::Value* firstChar = Builder.CreateLoad(llvm::Type::getInt8Ty(Builder.getContext()), ExprValue);
           if (TargetType->isIntegerTy(32)) {
               return Builder.CreateSExt(firstChar, TargetType);
           } else if (TargetType->isIntegerTy(8)) {
               return firstChar;
           } else {
               return Builder.CreateSExt(firstChar, TargetType);
           }
       } else if (SourceType->isPointerTy() && TargetType->isFloatingPointTy()) {
           llvm::Function* strtodFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strtod");
           if (!strtodFunc) {
               llvm::FunctionType* strtodType = llvm::FunctionType::get(
                   llvm::Type::getDoubleTy(Builder.getContext()),
                   {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                    llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0)},
                   false
               );
               strtodFunc = llvm::Function::Create(strtodType, llvm::Function::ExternalLinkage, "strtod", 
                                                 Builder.GetInsertBlock()->getParent()->getParent());
           }
           llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0));
           llvm::Value* doubleResult = Builder.CreateCall(strtodFunc, {ExprValue, nullPtr});
           
           if (TargetType->isFloatTy()) {
               return Builder.CreateFPTrunc(doubleResult, TargetType);
           } else {
               return doubleResult;
           }
       } else if (SourceType->isIntegerTy() && TargetType->isPointerTy()) {
           llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
           if (!mallocFunc) {
               llvm::FunctionType* mallocType = llvm::FunctionType::get(
                   llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                   {llvm::Type::getInt64Ty(Builder.getContext())},
                   false
               );
               mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                                 Builder.GetInsertBlock()->getParent()->getParent());
           }

           llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
           if (!sprintfFunc) {
               llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                   llvm::Type::getInt32Ty(Builder.getContext()),
                   {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                    llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                   true
               );
               sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", 
                                                  Builder.GetInsertBlock()->getParent()->getParent());
           }

           llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
           llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
           llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
           Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ExprValue});
           return bufferPtr;
       } else if (SourceType->isFloatingPointTy() && TargetType->isPointerTy()) {
           llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
           if (!mallocFunc) {
               llvm::FunctionType* mallocType = llvm::FunctionType::get(
                   llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                   {llvm::Type::getInt64Ty(Builder.getContext())},
                   false
               );
               mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                                 Builder.GetInsertBlock()->getParent()->getParent());
           }

           llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
           if (!sprintfFunc) {
               llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                   llvm::Type::getInt32Ty(Builder.getContext()),
                   {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                    llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                   true
               );
               sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", 
                                                  Builder.GetInsertBlock()->getParent()->getParent());
           }

           llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
           llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
           llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
           
           if (ExprValue->getType()->isFloatTy()) {
               ExprValue = Builder.CreateFPExt(ExprValue, Builder.getDoubleTy());
           }
           
           Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ExprValue});
           return bufferPtr;
       }
    }
       
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Builder.getContext()), 0);
}