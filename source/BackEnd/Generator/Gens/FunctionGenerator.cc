#include "FunctionGenerator.hh"
#include "BlockGenerator.hh"

llvm::Function* GenerateFunction(FunctionNode* Node, llvm::Module* Module, AllocaSymbols& AllocaMap) {
    llvm::LLVMContext& ctx = Module->getContext();

    std::string Name = Node->name;
    std::string ReturnTypeStr = Node->returnType;
    llvm::Type* ReturnType = GetLLVMTypeFromString(ReturnTypeStr, ctx);

    if (!ReturnType) {
        std::cerr << "Unknown return type: " << ReturnTypeStr << std::endl;
        exit(0);
    }

    std::vector<llvm::Type*> ArgTypes;
    for (const auto& Arg : Node->params) {
        llvm::Type* ArgType = GetLLVMTypeFromString(Arg.second, ctx);
        if (!ArgType) {
            std::cerr << "Unknown argument type: " << Arg.second << std::endl;
            exit(0);
        }
        ArgTypes.push_back(ArgType);
    }

    llvm::FunctionType* FuncType = llvm::FunctionType::get(ReturnType, ArgTypes, false);
    llvm::Function* Function = llvm::Function::Create(FuncType, llvm::Function::ExternalLinkage, Name, Module);

    if (Node->isInlined) {
        Function->addFnAttr(llvm::Attribute::AlwaysInline);
    } else {
        Function->addFnAttr(llvm::Attribute::NoInline);
    }

    auto ArgIter = Function->arg_begin();
    for (const auto& Arg : Node->params) {
        ArgIter->setName(Arg.first);
        ++ArgIter;
    }

    llvm::BasicBlock* EntryBlock = llvm::BasicBlock::Create(ctx, "entry", Function);
    llvm::IRBuilder<> Builder(EntryBlock);
    Builder.SetInsertPoint(EntryBlock);
    
    GenerateBlock(Node->body, Builder, AllocaMap);
    /*std::string ActualRet = GetStringFromLLVMType(ReturnValue->getType());
    std::cout << "returned a: " << ActualRet << std::endl;
    */
    
    if (ReturnType->isVoidTy()) {
        Builder.CreateRetVoid();
    } else {
        llvm::Value* ReturnValue = Builder.getInt32(0);
        Builder.CreateRet(ReturnValue);
    }

    return Function;
}
