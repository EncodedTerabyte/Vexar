#include "Tokenizer.hh"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

std::vector<Token> Tokenize(const std::map<unsigned int, std::string>& FileContents) {
    std::set<std::string> importedFiles;
    auto processedContent = ProcessImports(FileContents, importedFiles);
    std::vector<Token> tokens;
    bool inBlockComment = false;

    for (const auto& [lineNumber, line] : processedContent) {
        size_t i = 0;
        while (i < line.size()) {
            char c = line[i];

            if (inBlockComment) {
                if (c == '*' && i + 1 < line.size() && line[i + 1] == '/') {
                    inBlockComment = false;
                    i += 2;
                } else {
                    ++i;
                }
                continue;
            }

            if (isspace(c)) {
                ++i;
                continue;
            }

            if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
                break;
            }

            if (c == '/' && i + 1 < line.size() && line[i + 1] == '*') {
                inBlockComment = true;
                i += 2;
                continue;
            }

            Token tok;
            tok.line = lineNumber;
            tok.column = i + 1;

            if (c == '"' || c == '\'') {
                char quote = c;
                size_t start = i;
                ++i;
                while (i < line.size() && line[i] != quote) {
                    if (line[i] == '\\' && i + 1 < line.size()) {
                        i += 2;
                    } else {
                        ++i;
                    }
                }
                std::string literal = (i < line.size()) ? line.substr(start + 1, i - start - 1) : line.substr(start + 1);
                tok.value = literal;
                tok.type = (quote == '\'' && literal.length() == 1) ? TokenType::Character : TokenType::String;
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
                std::string threeChar = (i + 2 < line.size()) ? line.substr(i, 3) : "";
                std::string twoChar = (i + 1 < line.size()) ? line.substr(i, 2) : "";
                std::string oneChar(1, c);
                
                if (!threeChar.empty() && Operators.count(threeChar)) {
                    tok.value = threeChar;
                    tok.type = TokenType::Operator;
                    tokens.push_back(tok);
                    i += 3;
                }
                else if (!twoChar.empty() && Operators.count(twoChar)) {
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
                    Write("Tokenizer", "Unrecognized character: '" + oneChar + "' (ASCII: " + std::to_string((int)c) + ") at line " + std::to_string(lineNumber) + ", column " + std::to_string(i + 1), 1, true, true, "");
                    ++i;
                }
            }
        }
    }

    if (!processedContent.empty()) {
        Token eof;
        eof.type = TokenType::EndOfFile;
        eof.value = "";
        eof.line = processedContent.rbegin()->first + 1;
        eof.column = 1;
        tokens.push_back(eof);
    }

    return tokens;
}

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string content(size, '\0');
    if (!file.read(content.data(), size)) {
        return "";
    }
    
    return content;
}

std::string FindFileWithExtension(const std::string& basePath) {
    for (const std::string& ext : VexarAssociations) {
        std::string fullPath = basePath + "." + ext;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }
    return "";
}

std::vector<std::string> FindAllFilesInDirectory(const std::string& dirPath) {
    std::vector<std::string> files;
    
    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        return files;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            if (!extension.empty()) {
                extension = extension.substr(1);
                if (std::find(VexarAssociations.begin(), VexarAssociations.end(), extension) != VexarAssociations.end()) {
                    files.push_back(entry.path().string());
                }
            }
        }
    }
    
    return files;
}

std::map<unsigned int, std::string> ProcessImports(const std::map<unsigned int, std::string>& FileContents, std::set<std::string>& importedFiles) {
    std::map<unsigned int, std::string> result;
    unsigned int currentLine = 1;
    
    for (const auto& [lineNumber, line] : FileContents) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        if (token == "import") {
            std::string path;
            iss >> path;
            
            if (path.empty()) {
                Write("Tokenizer", "Empty import path on line " + std::to_string(lineNumber), 2, true, true, "");
                continue;
            }
            
            std::vector<std::string> filesToImport;
            
            if (path.length() >= 2 && path.substr(path.length() - 2) == "/*") {
                std::string dirPath = path.substr(0, path.length() - 2);
                filesToImport = FindAllFilesInDirectory(dirPath);
                
                if (filesToImport.empty()) {
                    Write("Tokenizer", "No Vexar files found in directory: " + dirPath, 1, true, true, "");
                    continue;
                }
            } else if (path.back() == '/') {
                std::string dirPath = path.substr(0, path.length() - 1);
                filesToImport = FindAllFilesInDirectory(dirPath);
                
                if (filesToImport.empty()) {
                    Write("Tokenizer", "No Vexar files found in directory: " + dirPath, 1, true, true, "");
                    continue;
                }
            } else {
                std::string resolvedPath;
                
                bool hasExtension = false;
                for (const std::string& ext : VexarAssociations) {
                    if (path.length() >= ext.length() + 1 && 
                        path.substr(path.length() - ext.length() - 1) == "." + ext) {
                        hasExtension = true;
                        break;
                    }
                }
                
                if (hasExtension) {
                    if (std::filesystem::exists(path)) {
                        resolvedPath = path;
                    } else {
                        Write("Tokenizer", "File not found: " + path, 2, true, true, "");
                        continue;
                    }
                } else {
                    resolvedPath = FindFileWithExtension(path);
                    if (resolvedPath.empty()) {
                        Write("Tokenizer", "File not found with any Vexar extension: " + path, 2, true, true, "");
                        continue;
                    }
                }
                
                filesToImport.push_back(resolvedPath);
            }
            
            for (const std::string& fileToImport : filesToImport) {
                std::string canonicalPath = std::filesystem::canonical(fileToImport).string();
                
                if (importedFiles.count(canonicalPath)) {
                    Write("Tokenizer", "Circular import detected, skipping: " + fileToImport, 1, true, true, "");
                    continue;
                }
                
                importedFiles.insert(canonicalPath);
                
                std::string fileContent = ReadFile(fileToImport);
                if (fileContent.empty()) {
                    Write("Tokenizer", "Could not read file: " + fileToImport, 2, true, true, "");
                    continue;
                }
                
                std::istringstream contentStream(fileContent);
                std::string contentLine;
                while (std::getline(contentStream, contentLine)) {
                    result[currentLine] = contentLine;
                    currentLine++;
                }
            }
        } else {
            result[currentLine] = line;
            currentLine++;
        }
    }
    
    return result;
}