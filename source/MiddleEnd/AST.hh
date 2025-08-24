#pragma once
#include "../Token.hh"
#include <string>
#include <vector>
#include <memory>
#include <sstream>

struct NodeType {
    static constexpr int Number = 0;
    static constexpr int Float = 1;
    static constexpr int String = 2;
    static constexpr int Character = 3;
    static constexpr int Identifier = 4;
    static constexpr int BinaryOp = 5;
    static constexpr int UnaryOp = 6;
    static constexpr int ExpressionStatement = 7;
    static constexpr int Program = 8;
    static constexpr int Variable = 9;
    static constexpr int Assignment = 10;
    static constexpr int Return = 11;
    static constexpr int Paren = 12;
    static constexpr int Boolean = 13;
    static constexpr int While = 14;
    static constexpr int If = 15;
    static constexpr int Function = 16;
    static constexpr int FunctionCall = 17;
    static constexpr int Block = 18;
    static constexpr int Condition = 19;
};

struct ASTNode {
    int type;
    Token token;
    virtual ~ASTNode() = default;
    virtual std::string get(const std::string& prefix = "", bool isLast = true) const = 0;
protected:
    static std::string branch(const std::string& prefix, bool isLast) {
        return prefix + (isLast ? "\\--" : "|--");
    }
    static std::string nextPrefix(const std::string& prefix, bool isLast) {
        return prefix + (isLast ? "    " : "|   ");
    }
    static std::string to_string(const std::string& s) {
        return s;
    }
};

struct ParenNode : ASTNode {
    std::unique_ptr<ASTNode> inner;
    ParenNode() { type = -1; }

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Paren]";
        if (inner) {
            oss << "\n" << inner->get(nextPrefix(prefix, isLast), true);
        }
        return oss.str();
    }
};

struct NumberNode : ASTNode {
    double value;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        return branch(prefix, isLast) + "[Number]: " + std::to_string(value);
    }
};

struct FloatNode : ASTNode {
    double value;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        return branch(prefix, isLast) + "[Float]: " + std::to_string(value);
    }
};

struct StringNode : ASTNode {
    std::string value;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        return branch(prefix, isLast) + "[String]: \"" + value + "\"";
    }
};

struct CharacterNode : ASTNode {
    char value;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        return branch(prefix, isLast) + "[Char]: '" + std::string(1, value) + "'";
    }
};

struct IdentifierNode : ASTNode {
    std::string name;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        return branch(prefix, isLast) + "[Identifier]: " + name;
    }
};

struct BinaryOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string op;
    
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Binary Op]: " << op;
        std::string childPrefix = nextPrefix(prefix, isLast);
        if (left) oss << "\n" << left->get(childPrefix, false) << "\n";
        if (right) oss << right->get(childPrefix, true);
        return oss.str();
    }
};

struct UnaryOpNode : ASTNode {
    std::unique_ptr<ASTNode> operand;
    std::string op;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Unary Op]: " << op;
        if (operand) oss << "\n" << operand->get(nextPrefix(prefix, isLast), true);
        return oss.str();
    }
};

struct ExpressionStatementNode : ASTNode {
    std::unique_ptr<ASTNode> expression;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Expression Node]";
        if (expression) oss << "\n" << expression->get(nextPrefix(prefix, isLast), true);
        return oss.str();
    }
};

struct ProgramNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << "\n\n" << branch(prefix, isLast) << "[Program Node]";
        for (size_t i = 0; i < statements.size(); ++i) {
            bool last = (i == statements.size() - 1);
            oss << "\n" << statements[i]->get(nextPrefix(prefix, isLast), last);
        }
        return oss.str();
    }
};

struct TypeNode : ASTNode {
    std::string name;
    std::vector<std::string> fields;
    std::shared_ptr<TypeNode> baseType;
    bool isBuiltin = false;

    TypeNode() { type = -1; }
    TypeNode(const std::string& n) : name(n) { type = -1; }

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Type]: " << name;
        if (isBuiltin) oss << " (builtin)";
        std::string childPrefix = nextPrefix(prefix, isLast);
        for (size_t i = 0; i < fields.size(); ++i) {
            bool lastField = (i == fields.size() - 1 && !baseType);
            oss << "\n" << branch(childPrefix, lastField) << fields[i];
        }
        if (baseType) {
            oss << "\n" << baseType->get(childPrefix, true);
        }
        return oss.str();
    }
};

struct VariableNode : ASTNode {
    std::string name;
    TypeNode varType;
    std::unique_ptr<ASTNode> value;

    VariableNode() : varType("auto") {}

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::string result = branch(prefix, isLast) + "[Variable]: " + name + " : " + varType.name;
        if (value) {
            result += "\n" + value->get(nextPrefix(prefix, isLast), true);
        }
        return result;
    }
};

struct AssignmentOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Assignment]: ";
        std::string childPrefix = nextPrefix(prefix, isLast);
        if (left) oss << "\n" << left->get(childPrefix, false) << "\n";
        if (right) oss << right->get(childPrefix, true);
        return oss.str();
    }
};

struct CompoundAssignmentOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string op; // "+=", "-=", "*=", etc.

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Compound Assignment]: " + op;
        std::string childPrefix = nextPrefix(prefix, isLast);
        if (left) oss << "\n" << left->get(childPrefix, false) << "\n";
        if (right) oss << right->get(childPrefix, true);
        return oss.str();
    }
};

struct IncrementOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Increment]: ";
        std::string childPrefix = nextPrefix(prefix, isLast);
        if (left) oss << "\n" << left->get(childPrefix, false) << "\n";
        if (right) oss << right->get(childPrefix, true);
        return oss.str();
    }
};

struct DecrementOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Decrement]: ";
        std::string childPrefix = nextPrefix(prefix, isLast);
        if (left) oss << "\n" << left->get(childPrefix, false) << "\n";
        if (right) oss << right->get(childPrefix, true);
        return oss.str();
    }
};

struct BlockNode : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Block]: ";
        for (size_t i = 0; i < statements.size(); ++i) {
            bool last = (i == statements.size() - 1);
            oss << "\n" << statements[i]->get(nextPrefix(prefix, isLast), last);
        }
        return oss.str();
    }
};

struct ConditionNode : ASTNode {
    std::unique_ptr<ASTNode> expression;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Condition]: ";
        if (expression)
            oss << "\n" << expression->get(nextPrefix(prefix, isLast), true);
        return oss.str();
    }
};

struct IfNode : ASTNode {
    enum class BranchType { MainIf, ElseIf, Else };

    struct Branch {
        std::unique_ptr<ConditionNode> condition;
        std::unique_ptr<BlockNode> block;
        BranchType type;
        size_t order = 0;
    };

    std::vector<Branch> branches;
    std::unique_ptr<BlockNode> elseBlock;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[If]:";

        for (size_t i = 0; i < branches.size(); ++i) {
            const auto& br = branches[i];
            bool lastBranch = (i + 1 == branches.size() && !elseBlock);

            std::string label = (br.type == BranchType::MainIf) ? "MainIf" :
                                (br.type == BranchType::ElseIf) ? "ElseIf " + std::to_string(br.order) : "Else";

            std::string branchPrefix = nextPrefix(prefix, isLast);
            oss << "\n" << branch(branchPrefix, lastBranch) << "[" << label << "]";

            if (br.condition)
                oss << "\n" << br.condition->get(nextPrefix(branchPrefix, lastBranch), lastBranch);

            oss << "\n" << br.block->get(nextPrefix(branchPrefix, lastBranch), lastBranch);
        }

        if (elseBlock) {
            std::string elsePrefix = nextPrefix(prefix, isLast);
            oss << "\n" << branch(elsePrefix, true) << "[Else]";
            oss << "\n" << elseBlock->get(nextPrefix(elsePrefix, true), true);
        }

        return oss.str();
    }
};

struct WhileNode : ASTNode {
    std::unique_ptr<ConditionNode> condition;
    std::unique_ptr<BlockNode> block;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[While]";
        if (condition) oss << "\n" << condition->get(nextPrefix(prefix, isLast), false);
        if (block) oss << "\n" << block->get(nextPrefix(prefix, isLast), true);
        return oss.str();
    }
};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> value;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Return]";
        if (value) {
            oss << "\n" << value->get(nextPrefix(prefix, isLast), true);
        }
        return oss.str();
    }
};

struct FunctionNode : ASTNode {
    std::string name;
    std::vector<std::pair<std::string, std::string>> params;
    std::string returnType = "void";
    std::unique_ptr<BlockNode> body;
    bool isInlined = false;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Function]: " << name << " : " << returnType;

        if (!params.empty()) {
            std::string paramsPrefix = nextPrefix(prefix, isLast);
            oss << "\n" << branch(paramsPrefix, body == nullptr) << "[Parameters]";
            for (size_t i = 0; i < params.size(); ++i) {
                bool lastParam = (i == params.size() - 1);
                oss << "\n" << branch(nextPrefix(paramsPrefix, body == nullptr), lastParam)
                    << params[i].first << " : " << params[i].second;
            }
        }

        if (body) {
            oss << "\n" << body->get(nextPrefix(prefix, isLast), true);
        }

        return oss.str();
    }
};

struct FunctionCallNode : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        std::ostringstream oss;
        oss << branch(prefix, isLast) << "[Function Call]: " << name;
        std::string childPrefix = nextPrefix(prefix, isLast);
        for (size_t i = 0; i < arguments.size(); ++i) {
            bool lastArg = (i == arguments.size() - 1);
            oss << "\n" << arguments[i]->get(childPrefix, lastArg);
        }
        return oss.str();
    }
};

struct BooleanNode : ASTNode {
    bool value;

    BooleanNode(bool v) : value(v) {
        type = NodeType::Boolean;
    }

    std::string get(const std::string& prefix = "", bool isLast = true) const override {
        return branch(prefix, isLast) + "[Boolean]: " + (value ? "true" : "false");
    }
};
