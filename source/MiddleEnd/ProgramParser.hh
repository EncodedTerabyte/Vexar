#pragma once
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../Token.hh"

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <set>

#include "ParseExpression.hh"
#include "Parser.hh"
#include "AST.hh"

std::unique_ptr<ProgramNode> ParseProgram(std::vector<Token> ProgramTokens);