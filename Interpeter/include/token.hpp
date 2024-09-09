#pragma once

#include <string>
#include <iostream>

enum class TokenType {
	IDENTIFIER, JUMP, CHAR, DOUBLE, INT, BOOL, STRING, OPERATOR, LPAREN, RPAREN, SEMICOLON, COMMA, CONDITION, LOOP, KEYWORD, MOD, END
};

struct Token {
	TokenType type;
	std::string value;

	bool operator==(TokenType other_type) const {
		return type == other_type;
	}

	bool operator==(const std::string& other_value) const {
		return value == other_value;
	}

	void print() {
		std::cout << static_cast<int>(type) << " " << value << std::endl;
	}
};