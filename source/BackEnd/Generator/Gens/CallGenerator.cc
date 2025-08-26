#include "CallGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCall(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods, BuiltinSymbols& BuiltIns) {
    auto* FuncCallNode = static_cast<FunctionCallNode*>(Expr.get());
    
    auto BuiltinIt = BuiltIns.find(FuncCallNode->name);
    if (BuiltinIt != BuiltIns.end()) {
        return BuiltinIt->second(FuncCallNode->arguments, Builder, SymbolStack, Methods);
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