#include "Mapping.hh"

void PushScope(ScopeStack& Stack) {
    Stack.push_back(AllocaSymbols());
}

void PopScope(ScopeStack& Stack) {
    if (!Stack.empty()) {
        Stack.pop_back();
    }
}

llvm::AllocaInst* FindInScopes(ScopeStack& Stack, const std::string& Name) {
    for (auto it = Stack.rbegin(); it != Stack.rend(); ++it) {
        auto found = it->find(Name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}