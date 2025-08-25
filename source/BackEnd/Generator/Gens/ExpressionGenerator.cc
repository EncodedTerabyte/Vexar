#include "ExpressionGenerator.hh"
#include <cmath>

llvm::Value* GenerateExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
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
       auto* ParennNode = static_cast<ParenNode*>(Expr.get());
       return GenerateExpression(ParennNode->inner, Builder, SymbolStack, Methods);
   } else if (Expr->type == NodeType::BinaryOp) {
       auto* BinOpNode = static_cast<BinaryOpNode*>(Expr.get());
       
       llvm::Value* Left = GenerateExpression(BinOpNode->left, Builder, SymbolStack, Methods);
       llvm::Value* Right = GenerateExpression(BinOpNode->right, Builder, SymbolStack, Methods);

       if (Left->getType()->isPointerTy() && Right->getType()->isPointerTy()) {
           llvm::Function* strcatFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcat");
           if (!strcatFunc) {
               llvm::FunctionType* strcatType = llvm::FunctionType::get(
                   llvm::PointerType::get(Builder.getContext(), 0),
                   {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                   false
               );
               strcatFunc = llvm::Function::Create(strcatType, llvm::Function::ExternalLinkage, "strcat", Builder.GetInsertBlock()->getParent()->getParent());
           }

           llvm::Function* strcpyFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strcpy");
           if (!strcpyFunc) {
               llvm::FunctionType* strcpyType = llvm::FunctionType::get(
                   llvm::PointerType::get(Builder.getContext(), 0),
                   {llvm::PointerType::get(Builder.getContext(), 0), llvm::PointerType::get(Builder.getContext(), 0)},
                   false
               );
               strcpyFunc = llvm::Function::Create(strcpyType, llvm::Function::ExternalLinkage, "strcpy", Builder.GetInsertBlock()->getParent()->getParent());
           }
           
           llvm::Value* buffer = Builder.CreateAlloca(llvm::Type::getInt8Ty(Builder.getContext()), 
                                                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1000),
                                                    "str_buffer");

           llvm::Value* bufferPtr = Builder.CreatePointerCast(buffer, llvm::PointerType::get(Builder.getContext(), 0));
           
           Builder.CreateCall(strcpyFunc, {bufferPtr, Left});
           return Builder.CreateCall(strcatFunc, {bufferPtr, Right});
       }

       if (BinOpNode->op == "+") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               return Builder.CreateFAdd(Left, Right, "addtmp");
           } else {
               return Builder.CreateAdd(Left, Right, "addtmp");
           }
       } else if (BinOpNode->op == "-") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               return Builder.CreateFSub(Left, Right, "subtmp");
           } else {
               return Builder.CreateSub(Left, Right, "subtmp");
           }
       } else if (BinOpNode->op == "*") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               return Builder.CreateFMul(Left, Right, "multmp");
           } else {
               return Builder.CreateMul(Left, Right, "multmp");
           }
       } else if (BinOpNode->op == "/") {
           if (Left->getType()->isFloatingPointTy() || Right->getType()->isFloatingPointTy()) {
               return Builder.CreateFDiv(Left, Right, "divtmp");
           } else {
               return Builder.CreateSDiv(Left, Right, "divtmp");
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
           
       }
   } else if (Expr->type == NodeType::FunctionCall) {
       auto* FuncCallNode = static_cast<FunctionCallNode*>(Expr.get());
       
       if (FuncCallNode->name == "print") {
           if (FuncCallNode->arguments.empty()) {
               exit(1);
           }
           
           llvm::Function* printfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("printf");
           if (!printfFunc) {
               llvm::FunctionType* printfType = llvm::FunctionType::get(
                   llvm::Type::getInt32Ty(Builder.getContext()),
                   {llvm::PointerType::get(Builder.getContext(), 0)},
                   true
               );
               printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", 
                                                 Builder.GetInsertBlock()->getParent()->getParent());
           }
           
           std::vector<llvm::Value*> printfArgs;
           
           llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[0], Builder, SymbolStack, Methods);
           
           if (ArgValue->getType()->isIntegerTy(32)) {
               llvm::Value* formatStr = Builder.CreateGlobalString("%d\n", "int_fmt");
               llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
               printfArgs.push_back(formatPtr);
               printfArgs.push_back(ArgValue);
           } else if (ArgValue->getType()->isFloatingPointTy()) {
               if (ArgValue->getType()->isFloatTy()) {
                   ArgValue = Builder.CreateFPExt(ArgValue, Builder.getDoubleTy());
               }
               llvm::Value* formatStr = Builder.CreateGlobalString("%.6f\n", "float_fmt");
               llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
               printfArgs.push_back(formatPtr);
               printfArgs.push_back(ArgValue);
           } else if (ArgValue->getType()->isPointerTy()) {
               llvm::Value* formatStr = Builder.CreateGlobalString("%s\n", "str_fmt");
               llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
               printfArgs.push_back(formatPtr);
               printfArgs.push_back(ArgValue);
           } else if (ArgValue->getType()->isIntegerTy(8)) {
               llvm::Value* formatStr = Builder.CreateGlobalString("%c\n", "char_fmt");
               llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
               printfArgs.push_back(formatPtr);
               printfArgs.push_back(ArgValue);
           } else if (ArgValue->getType()->isIntegerTy(1)) {
               llvm::Value* boolAsInt = Builder.CreateZExt(ArgValue, Builder.getInt32Ty());
               llvm::Value* formatStr = Builder.CreateGlobalString("%d\n", "bool_fmt");
               llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(Builder.getContext(), 0));
               printfArgs.push_back(formatPtr);
               printfArgs.push_back(boolAsInt);
           } else {
               exit(1);
           }
           
           llvm::Value* result = Builder.CreateCall(printfFunc, printfArgs);
           return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
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
   }

   return nullptr;
}