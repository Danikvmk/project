#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>

#include "token.hpp"

class Lexer {
public:
	Lexer(const std::string&);
	std::vector<Token> tokenize();
private:
	Token extract_identifier();
	Token extract_char();
	Token extract_string();
	Token extract_number();
	Token extract_operator();

	static const std::string metachars;
	static const std::unordered_set<std::string> operators;
	static const std::unordered_set<std::string> keyWords;
	static const std::unordered_set<std::string> conditionals;
	static const std::unordered_set<std::string> loops;
	static const std::unordered_set<std::string> jumps;
	static const std::unordered_set<std::string> bools;
	static const std::unordered_set<std::string> modifications;

	std::string input;
	std::size_t offset;
};
