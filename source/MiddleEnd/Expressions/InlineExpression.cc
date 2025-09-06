#include "InlineExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include <map>
#include <set>
#include <memory>
#include <string>

std::string CaptureRawCodeBlock(Parser& parser) {
    std::string result;
    int brace_count = 1;
    Token prevTok;
    
    while (brace_count > 0 && !parser.isAtEnd()) {
        const Token& tok = parser.peek();
        
        if (tok.value == "{") {
            brace_count++;
        } else if (tok.value == "}") {
            brace_count--;
        }
        
        if (brace_count > 0) {
            if (tok.type == TokenType::String) {
                result += "\"" + tok.value + "\"";
            } else {
                result += tok.value;
            }
            
            if (tok.value != "," && tok.value != "(" && tok.value != ")" && 
                tok.value != "[" && tok.value != "]" && tok.value != "$" && tok.value != "%" && tok.value != ":" && tok.value != "#" &&
                parser.peek(1).value != "(" && parser.peek(1).value != ")" &&
                parser.peek(1).value != ",") {
                result += " ";
            }
        }
        
        parser.advance();
    }
    
    return result;
}

namespace InlineExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        const Token& tok = parser.peek();

        if (tok.value == "inline") { 
            parser.advance();
            const Token& next_tok = parser.peek();
            
            
            if (next_tok.value == "asm" || next_tok.value == "assembly" || next_tok.value == "__asm__") {
                auto Node = std::make_unique<InlineCodeNode>();
                Node->line = tok.line;
                Node->column = tok.column;
                Node->type = NodeType::InlineCodeBlock;
                
                parser.consume(TokenType::Identifier, next_tok.value);
                parser.consume(TokenType::Delimiter, "{");
                
                Node->raw_code = CaptureRawCodeBlock(parser);
                Node->lang = "assembly";
                
                return Node;
            }
            
            else if (next_tok.value == "c" || next_tok.value == "C" || next_tok.value == "__c__") {
                auto Node = std::make_unique<InlineCodeNode>();
                Node->line = tok.line;
                Node->column = tok.column;
                Node->type = NodeType::InlineCodeBlock;
                
                parser.consume(TokenType::Identifier, next_tok.value);
                parser.consume(TokenType::Delimiter, "{");
                
                Node->raw_code = CaptureRawCodeBlock(parser);
                Node->lang = "c";

                return Node;
            }
            
            else if (next_tok.value == "cxx" || next_tok.value == "cpp" || next_tok.value == "C++" ||
                     next_tok.value == "__c++__" || next_tok.value == "__C++__" || next_tok.value == "__CC__" ||
                     next_tok.value == "__cc__" || next_tok.value == "__cxx__" || next_tok.value == "__Cxx__") {
                auto Node = std::make_unique<InlineCodeNode>();
                Node->line = tok.line;
                Node->column = tok.column;
                Node->type = NodeType::InlineCodeBlock;
                
                parser.consume(TokenType::Identifier, next_tok.value);
                parser.consume(TokenType::Delimiter, "{");
                
                Node->raw_code = CaptureRawCodeBlock(parser);
                Node->lang = "cxx";

                return Node;
            }
            else {
                auto Node = std::make_unique<InlinePreprocessor>();
                Node->line = tok.line;
                Node->column = tok.column;
                Node->type = NodeType::InlinePreProc;
                return Node;
            }
        }

        auto Node = std::make_unique<InlinePreprocessor>();
        Node->line = tok.line;
        Node->column = tok.column;
        Node->type = NodeType::InlinePreProc;
        parser.consume(TokenType::Keyword, tok.value);

        return Node;
    }
}