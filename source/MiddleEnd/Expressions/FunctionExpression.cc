#include "FunctionExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../ParseExpression.hh"

namespace FunctionExpression {

    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        Token inline_q = parser.peek(-1);

        parser.advance();

        Token nameTok = parser.peek();
        parser.advance();

        auto funcNode = std::make_unique<FunctionNode>();
        funcNode->type = NodeType::Function;
        funcNode->name = nameTok.value;

        funcNode->isInlined = false;
        funcNode->alwaysInline = false;

        if (inline_q.value == "inline") {
            funcNode->isInlined = true;
        } else if (inline_q.value == "always_inline") {
            funcNode->alwaysInline = true;
        }
        
        if (parser.peek().value == "(") {
            parser.advance();
            while (parser.peek().value != ")") {
                Token paramName = parser.peek();
                parser.advance();
                parser.advance();
                Token paramType = parser.peek();
                parser.advance();
                funcNode->params.push_back({paramName.value, paramType.value});
                if (parser.peek().value == ",") parser.advance();
            }
            if (parser.peek().value == ")") parser.advance();
        }

        if (parser.peek().value == ":") {
            parser.advance();
            Token returnType = parser.peek();
            parser.advance();
            funcNode->returnType = returnType.value;
        }

        funcNode->body = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(BlockNodeContainer::ParseBlock(parser).release())
        );

        return funcNode;
    }

}
