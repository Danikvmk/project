#include "parser.hpp"
#include <stdexcept>

#define MIN_PRECEDENCE 0

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), offset(0) {}

std::vector<declaration> Parser::parse() {
	return parse_declaration_list();
}

std::vector<declaration> Parser::parse_declaration_list() {
	std::vector<declaration> decl_list;
	while (!match(TokenType::END)) {
		decl_list.push_back(parse_declaration());
	}
	return decl_list;
}

declaration Parser::parse_declaration() {
	bool const_var = false;
	if (match(TokenType::MOD)) {
		const_var = true;
		++offset;
	}
	
	auto type = extract(TokenType::KEYWORD);
	if (type == "namespace") {
		return parse_namespace_declaration();
	} else {
		++offset;
		if (match("(")) {
			--offset;
			auto name = extract(TokenType::IDENTIFIER);
			std::vector<std::pair<std::string, std::string>> parameters;	
			extract(TokenType::LPAREN);
			while (!match(")")) {
				auto param_type = extract(TokenType::KEYWORD);
				auto param_name = extract(TokenType::IDENTIFIER);
				parameters.push_back(std::make_pair(param_type, param_name));
				if (match(TokenType::COMMA)) {
					extract(TokenType::COMMA);
				}
			}
			extract(TokenType::RPAREN);
			
			if (!match("{")) {
				throw std::runtime_error("incorrect declaration function " + name);
			}
			auto block_statement = parse_statement();
			return std::make_shared<Functions_decl>(type, name, parameters, block_statement);		
		} else {
			--offset;
			std::vector<std::pair<std::string, expression>> vars;
			while (!match(TokenType::SEMICOLON)) {
				auto name = extract(TokenType::IDENTIFIER);
				if (match(TokenType::SEMICOLON)) {
					vars.push_back(std::make_pair(name, nullptr));
					break;
				}		
				extract("=");
				auto expr = parse_binary_expression(MIN_PRECEDENCE);
				vars.push_back(std::make_pair(name, expr));
				if (match(TokenType::COMMA)) extract(TokenType::COMMA);								
			}
			extract(TokenType::SEMICOLON);
			if (const_var) return std::make_shared<ConstVariable>(type, vars); 
			else return std::make_shared<Variables_decl>(type, vars);
		}
	} 
}

declaration Parser::parse_namespace_declaration() {
	auto name = extract(TokenType::IDENTIFIER);
	extract("{");
	std::vector<declaration> declarations;
	while (!match("}")) {
		declarations.push_back(parse_declaration());
	}
	extract(TokenType::RPAREN);
	return std::make_shared<Namespace_decl>(name, declarations);
}


statement Parser::parse_statement() {
	if (match("{")) {
		extract(TokenType::LPAREN);
		std::vector<statement> body;
		while (!match("}")) {
			body.push_back(parse_statement());
			if (match(TokenType::SEMICOLON)) extract(TokenType::SEMICOLON);
		}
		extract(TokenType::RPAREN);
		return std::make_shared<Block_statement>(body);
	} else if (match(TokenType::KEYWORD) || match(TokenType::MOD)) {
		if (match("namespace")) throw std::runtime_error("'namespace' definition is not allowed here");
		return parse_declaration_statement();
	} else if (match(TokenType::CONDITION)) {
		return parse_condition_statements();
	} else if (match(TokenType::LOOP)) {
		return parse_loop_statement();
	} else if (match(TokenType::JUMP)) {
		return parse_jump_statement();
	} else {
		return parse_expression_statement();
	}
}

statement Parser::parse_declaration_statement() {
	return std::make_shared<Decl_statement>(parse_declaration());
}

statement Parser::parse_condition_statements() {
	std::vector<statement> branches;
	while(match(TokenType::CONDITION)) {
		branches.push_back(parse_condition_statement(std::string("")));
	}
	return std::make_shared<ConditionalBlock>(branches);
}


statement Parser::parse_condition_statement(std::string key) {
	auto tmp = extract(TokenType::CONDITION);
	if (tmp == "if") {
		extract(TokenType::LPAREN);
		auto cond = parse_statement();
		if (auto test = std::dynamic_pointer_cast<Expression_statement>(cond); !test) {
			throw std::runtime_error("incorrect condition in the conditional statement");
		}
		extract(TokenType::RPAREN);
		
		auto body = parse_statement();
		if (match(TokenType::SEMICOLON)) extract(TokenType::SEMICOLON);
		return std::make_shared<ConditionalBranches>(key + "if", cond, body);
	} else if (tmp == "else") {
		if (key == "else ") throw std::runtime_error("invalid notation of conditional operator");
		if (match("if")) return parse_condition_statement("else ");
		auto body = parse_statement();
		return std::make_shared<ConditionalBranches>("else", nullptr, body);	
	} else {
		throw std::runtime_error("");
	}
}

statement Parser::parse_loop_statement() {
	return extract(TokenType::LOOP) == "for" ? parse_for_statement() : parse_while_statement();
}

statement Parser::parse_for_statement() {
	extract("(");
	auto var = parse_statement();
	if (match(TokenType::SEMICOLON)) extract(TokenType::SEMICOLON);
	auto cond = parse_statement();
	if (match(TokenType::SEMICOLON)) extract(TokenType::SEMICOLON);
	auto Expr = parse_statement();
	extract(")");
	auto body = parse_statement();
	return std::make_shared<For_statement>(var, cond, Expr, body);
}

statement Parser::parse_while_statement() {
	extract("(");
	auto cond = parse_statement();
	extract(")");
	auto body = parse_statement();
	return std::make_shared<While_statement>(cond, body);
}

statement Parser::parse_jump_statement() {
	auto jump = extract(TokenType::JUMP);
	if (jump == "break")
		return std::make_shared<Break_statement>();
	else if (jump == "continue")
		return std::make_shared<Continue_statement>();
	else
		return std::make_shared<Return_statement>(parse_binary_expression(MIN_PRECEDENCE));
}

statement Parser::parse_expression_statement() {
	return std::make_shared<Expression_statement>(parse_binary_expression(MIN_PRECEDENCE));
}

expression Parser::parse_binary_expression(int min_precedence) {
	auto lhs = parse_base_expression();
	if (match("++") || match("--")) {
		lhs = std::make_shared<PostfixNode>(extract(TokenType::OPERATOR), lhs);
	}

	auto op = tokens[offset].value;
	for (; operators.contains(op) && operators.at(op) >= min_precedence; op = tokens[offset].value) {
		if (op == "?") {
			return lhs;
		}
		++offset;
		auto rhs = parse_binary_expression(operators.at(op));
		lhs = std::make_shared<BinaryNode>(op, lhs, rhs);
		if (tokens[offset] == "?") {
			op = tokens[offset].value;
			break;
		}
	}

	if (op == "?") {
		offset++; 
		auto true_expr = parse_binary_expression(MIN_PRECEDENCE);
		extract(":");
		auto false_expr = parse_binary_expression(MIN_PRECEDENCE);
		lhs = std::make_shared<TernaryNode>(lhs, true_expr, false_expr);
	}

	return lhs;
}

expression Parser::parse_base_expression() {
	if (match(TokenType::CHAR) || match(TokenType::BOOL) || match(TokenType::DOUBLE) || match(TokenType::INT) || match(TokenType::STRING)) {
		return parse_literal();
	} else if (match(TokenType::IDENTIFIER)) {
		if (auto identifier = extract(TokenType::IDENTIFIER); match(TokenType::LPAREN)) {
			return std::make_shared<FunctionNode>(identifier, parse_function_expression());
		} else {
			return std::make_shared<IdentifierNode>(identifier);
		}
	} else if (unary.contains(tokens[offset].value)) {
		std::string op = extract(TokenType::OPERATOR);
		return std::make_shared<PrefixNode>(op, parse_base_expression());
	} else if (match(TokenType::LPAREN)) {
		return parse_parenthesized_expression();
	} else if (match(TokenType::SEMICOLON)) {
		return nullptr;
	} else {
		std::cout << tokens[offset - 2].value + " " + tokens[offset - 1].value + " " + tokens[offset].value;
		std::cout << std::endl;
		throw std::runtime_error("Incorrect base expression");
	}
	return nullptr;
}

expression Parser::parse_literal() {
	auto literal = tokens[offset++];
	switch (literal.type) {
		case TokenType::CHAR:
			return std::make_shared<CharNode>(literal.value[0]);
		case TokenType::DOUBLE:
			return std::make_shared<DoubleNode>(std::stod(literal.value));
		case TokenType::INT:
			return std::make_shared<IntNode>(std::stoi(literal.value));
		case TokenType::BOOL:
			if (literal.value == "true")
				return std::make_shared<BoolNode>(true);
			else
				return std::make_shared<BoolNode>(false);
		case TokenType::STRING:
			return std::make_shared<StringNode>(literal.value);
		default:
			return nullptr;
	}
}

std::vector<expression> Parser::parse_function_expression() {
	extract(TokenType::LPAREN);
	std::vector<expression> args;
	if (!match(TokenType::RPAREN)) {
		while(true) {
			args.push_back(parse_binary_expression(MIN_PRECEDENCE));
			if (match(TokenType::COMMA)) {
				extract(TokenType::COMMA);
			} else {
				extract(TokenType::RPAREN);
				break;
			}
		}
	} else extract(TokenType::RPAREN);
	return args;
}

expression Parser::parse_parenthesized_expression() {
	extract(TokenType::LPAREN);
	auto node = parse_binary_expression(MIN_PRECEDENCE);
	extract(TokenType::RPAREN);
	return std::make_shared<ParenthesizedNode>(node);	
}

//////////////////////////////////////////////////////////////

bool Parser::match(const std::string& expected_str) const {
	return tokens[offset] == expected_str;
}

bool Parser::match(TokenType expected_type) const {
	return tokens[offset] == expected_type;
}

std::string Parser::extract(TokenType expected_type) {
	if (!match(expected_type)) {
		throw std::runtime_error("Unexpected token " + tokens[offset].value);
	}
	return tokens[offset++].value;
}

std::string Parser::extract(const std::string& expected_str) {
	if (!match(expected_str)) {
		throw std::runtime_error("Unexpected string " + expected_str);
	}
	return tokens[offset++].value;
}
////////////////////////////////////////////////////////////

const std::unordered_set<std::string> Parser::unary = {"+", "-", "++", "--", "!"};

const std::unordered_map<std::string, int> Parser::operators = {
	{"+=", 0}, {"-=", 0}, {"*=", 0}, {"/=", 0}, {"=", 0},
	{"||", 1}, {"&&", 2},
	{"==", 3}, {"!=", 3},
	{">", 4}, {">=", 4}, {"<", 4}, {"<=", 4},
	{"+", 5}, {"-", 5},
	{"*", 6}, {"/", 6},
	{"::", 7},
	{"?", 8}
};







