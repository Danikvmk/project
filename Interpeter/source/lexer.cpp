#include "lexer.hpp"

Lexer::Lexer(const std::string& input) : input(input) {}

std::vector<Token> Lexer::tokenize() {
	std::vector<Token> tokens;
	for(; input[offset];) {
		if (std::isspace(input[offset])) {
			++offset;
		} else if (std::isdigit(input[offset]) || input[offset] == '.') {
			tokens.push_back(extract_number());
		} else if (std::isalpha(input[offset]) || input[offset] == '_') {
			tokens.push_back(extract_identifier());
		} else if (metachars.contains(input[offset])) {
			tokens.push_back(extract_operator());
		} else if (input[offset] == '(' || input[offset] == '[' || input[offset] == '{') {
			tokens.push_back(Token{TokenType::LPAREN, {input[offset++]}});
		} else if (input[offset] == ')' || input[offset] == ']' || input[offset] == '}') {
			tokens.push_back(Token{TokenType::RPAREN, {input[offset++]}});
		} else if (input[offset] == ',') {
			++offset;
			tokens.push_back(Token{TokenType::COMMA, ","});
		} else if (input[offset] == ';') {
			++offset;
			tokens.push_back(Token{TokenType::SEMICOLON, ";"});
		} else if (input[offset] == '\''){
			tokens.push_back(extract_char());
		} else if (input[offset] == '\"') {
			tokens.push_back(extract_string());
		} else {
			throw std::runtime_error("Unknown symbol");
		}
	}
	tokens.push_back(Token{TokenType::END, ""});
	return tokens;
}

Token Lexer::extract_string() {
	offset++;
	std::size_t i = 0;
	for (; input[offset + i] != '\"';) {
		++i;
		if (input[offset + i] == '\0') {
			throw std::runtime_error("missing terminating \" character");		
		}
	}

	Token token{TokenType::STRING, std::string(input, offset, i)};
	offset += i + 1;
	return token;
}

Token Lexer::extract_identifier() {
	std::size_t i = 0;
	for(; std::isalnum(input[offset + i]) || input[offset + i] == '_'; ++i);

	Token token{TokenType::IDENTIFIER, std::string(input, offset, i)};
	offset += i;
	if (keyWords.contains(token.value)) {
		token.type = TokenType::KEYWORD;
	} else if (conditionals.contains(token.value)) {
		token.type = TokenType::CONDITION;
	} else if (jumps.contains(token.value)) {
		token.type = TokenType::JUMP;
	} else if (loops.contains(token.value)) {
		token.type = TokenType::LOOP;
	} else if (bools.contains(token.value)) {
		token.type = TokenType::BOOL;
	} else if (modifications.contains(token.value)) {
		token.type = TokenType::MOD;	
	}

	return token;
}

Token Lexer::extract_char() {
	offset++;
	Token token{TokenType::CHAR, {input[offset++]}};
	if (input[offset] != '\'') {
		throw std::runtime_error("incorrect entry of the char constant");
	}
	offset++;
	return token;
}

Token Lexer::extract_number() {
	std::size_t i = 0;
	for(; std::isdigit(input[offset + i]); ++i);
	auto int_len = i;
	if (input[offset + i] == '.') {
		++i;
		auto j = i;
		for (; std::isdigit(input[offset + i]); ++i);
		if (i - j == 0 && int_len == 0) {
			throw std::runtime_error("Missing int and float part of number");
		}
		Token token{TokenType::DOUBLE, std::string(input, offset, i)};
		offset += i;
		return token;		
	}
	Token token{TokenType::INT, std::string(input, offset, i)};
	offset += i;
	return token;		
}

Token Lexer::extract_operator() {
	std::size_t i = 0;
	std::string op;
	for(; metachars.contains(input[offset + i]); ++i) {
		op += input[offset + i];
		if (op == ":") continue;
		if (!operators.contains(op)) {
			if (i == 0) {
				throw std::runtime_error("Invalid operator " + op);
			}
			op.pop_back();
			break;
		}
	}
	Token token{TokenType::OPERATOR, op};
	offset += i;
	return token;
}

const std::string Lexer::metachars = "+-*/=!|&<>:?";
const std::unordered_set<std::string> Lexer::operators = {"+", "-", "*", "/", "=", "+=", "-=", "*=", "/=", "==", "!","!=", "||", "&&", "++", "--", ">", ">=", "<", "<=", "::", ":", "?"};
const std::unordered_set<std::string> Lexer::keyWords = {"int", "double", "char", "void", "bool", "string", "namespace"};
const std::unordered_set<std::string> Lexer::conditionals = {"if", "else"};
const std::unordered_set<std::string> Lexer::loops = {"while", "for"};
const std::unordered_set<std::string> Lexer::jumps = {"return", "break", "continue"};
const std::unordered_set<std::string> Lexer::bools = {"true", "false"};
const std::unordered_set<std::string> Lexer::modifications = {"const"};
