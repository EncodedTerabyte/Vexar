#include "ProgramParser.hh"
#include <vector>
#include <memory>

std::vector<std::unique_ptr<TypeNode>> GenerateBuiltinTypes() {
    std::vector<std::unique_ptr<TypeNode>> types;

    auto makeType = [](const std::string& name) -> std::unique_ptr<TypeNode> {
        auto type = std::make_unique<TypeNode>();
        type->name = name;
        type->isBuiltin = true;
        return type;
    };
    
    types.push_back(makeType("auto"));      // handled by 'generic type handler'
    types.push_back(makeType("int"));       // handled by 'integer type handler'
    types.push_back(makeType("uint"));      // handled by 'uint    type handler'
    types.push_back(makeType("float"));     // handled by 'float   type handler'
    types.push_back(makeType("char"));      // handled by 'char    type handler'
    types.push_back(makeType("string"));    // handled by 'string  type handler'
    types.push_back(makeType("true"));      // handled by 'boolean type handler'
    types.push_back(makeType("false"));     // handled by 'boolean type handler'

    return types;
}

std::unique_ptr<ProgramNode> ParseProgram(std::vector<Token> ProgramTokens) {
    Parser parser(ProgramTokens);
    auto root = std::make_unique<ProgramNode>();
    root->type = NodeType::Program;

    auto builtinTypes = GenerateBuiltinTypes();
    for (auto& t : builtinTypes) {
        //std::cout << "registering type: " + t->name << std::endl; 
        root->statements.push_back(std::move(t));
    }

    while (parser.peek().type != TokenType::EndOfFile && 
           parser.peek().value != "=" && 
           parser.peek().value != ";") 
    {
        auto stmt = std::make_unique<ExpressionStatementNode>();
        stmt->expression = Main::ParseExpression(parser);
        stmt->type = NodeType::ExpressionStatement;
        root->statements.push_back(std::move(stmt));
    }

    return root;
}
