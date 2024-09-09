#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "ast.hpp"
#include "token.hpp"

class Parser {
public:
	Parser(const std::vector<Token>&);
	std::vector<declaration> parse();
private:
	std::vector<declaration> parse_declaration_list();
	declaration parse_declaration();
	declaration parse_namespace_declaration();
	
	statement parse_statement();
	statement parse_declaration_statement();
	statement parse_condition_statements();
	statement parse_condition_statement(std::string);
	statement parse_loop_statement();
	statement parse_for_statement();
	statement parse_while_statement();
	statement parse_jump_statement();
	statement parse_expression_statement();
	
	expression parse_binary_expression(int);
	expression parse_base_expression();
	expression parse_literal();
	std::vector<expression> parse_function_expression();
	expression parse_parenthesized_expression();

	bool match(TokenType) const;
	bool match(const std::string&) const;
	std::string extract(TokenType);
	std::string extract(const std::string&);

		
	static const std::unordered_map<std::string, int> operators;
	static const std::unordered_set<std::string> unary;
	
	std::vector<Token> tokens;
	std::size_t offset;
};
