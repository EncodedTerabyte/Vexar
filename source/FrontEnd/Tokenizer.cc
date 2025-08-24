#include "Tokenizer.hh"

std::vector<Token> Tokenize(const std::map<unsigned int, std::string>& FileContents) {
    std::vector<Token> tokens;

    for (const auto& [lineNumber, line] : FileContents) {
        size_t i = 0;
        while (i < line.size()) {
            char c = line[i];

            if (isspace(c)) {
                ++i;
                continue;
            }

            Token tok;
            tok.line = lineNumber;
            tok.column = i + 1;

            if (c == '"' || c == '\'') {
                char quote = c;
                size_t start = i;
                ++i;
                while (i < line.size() && line[i] != quote) ++i;
                std::string literal = (i < line.size()) ? line.substr(start + 1, i - start - 1) : line.substr(start + 1);
                tok.value = literal;
                tok.type = (quote == '\'' && literal.size() == 1) ? TokenType::Character : TokenType::String;
                if (i < line.size()) ++i;
                tokens.push_back(tok);
            }
            else if (isalpha(c) || c == '_') {
                size_t start = i;
                while (i < line.size() && (isalnum(line[i]) || line[i] == '_')) ++i;
                tok.value = line.substr(start, i - start);
                tok.type = Keywords.count(tok.value) ? TokenType::Keyword : TokenType::Identifier;
                tokens.push_back(tok);
            }
            else if (isdigit(c)) {
                size_t start = i;
                bool isFloat = false;
                while (i < line.size() && isdigit(line[i])) ++i;
                if (i < line.size() && line[i] == '.') {
                    ++i;
                    isFloat = true;
                    while (i < line.size() && isdigit(line[i])) ++i;
                }
                tok.value = line.substr(start, i - start);
                tok.type = isFloat ? TokenType::Float : TokenType::Number;
                tokens.push_back(tok);
            }
            else {
                std::string twoChar = (i + 1 < line.size()) ? line.substr(i, 2) : "";
                std::string oneChar(1, c);
                if (!twoChar.empty() && Operators.count(twoChar)) {
                    tok.value = twoChar;
                    tok.type = TokenType::Operator;
                    tokens.push_back(tok);
                    i += 2;
                }
                else if (Operators.count(oneChar)) {
                    tok.value = oneChar;
                    tok.type = TokenType::Operator;
                    tokens.push_back(tok);
                    ++i;
                }
                else if (Delimiters.count(c)) {
                    tok.value = oneChar;
                    tok.type = TokenType::Delimiter;
                    tokens.push_back(tok);
                    ++i;
                }
                else {
                    ++i;
                }
            }
        }
    }

    Token eof;
    eof.type = TokenType::EndOfFile;
    eof.value = "";
    eof.line = FileContents.rbegin()->first + 1;
    eof.column = 1;
    tokens.push_back(eof);

    return tokens;
}
