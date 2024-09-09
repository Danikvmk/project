#pragma once

#include <string>
#include <vector>
#include <memory>
#include <string>

class Visitor;

struct ASTNode {
	virtual void accept(Visitor&) = 0;
	virtual ~ASTNode() = default;
};

///////////////////////////////////////////////////////////////////////////

struct Declaration : public ASTNode {
	virtual void accept(Visitor&) = 0;
	virtual ~Declaration() = default;
};

using declaration = std::shared_ptr<Declaration>;

struct Statement : public ASTNode {
	virtual void accept(Visitor&) = 0;
	virtual ~Statement() = default;
};

using statement = std::shared_ptr<Statement>;

struct Expression : public ASTNode {
	virtual void accept(Visitor&) = 0;
	virtual ~Expression() = default;
};

using expression = std::shared_ptr<Expression>;

///////////////////////////////////////////////////////////////////////////

struct Namespace_decl : public Declaration {
	std::string name;
	std::vector<declaration> declarations;
	
	Namespace_decl(const std::string& name, std::vector<declaration>& declarations)
		: name(name), declarations(declarations) {} 
	void accept(Visitor&);
};

struct Variables_decl : public Declaration {
	std::string type;
	std::vector<std::pair<std::string, expression>> vars;
	Variables_decl(const std::string& type, const std::vector<std::pair<std::string, expression>>& vars)
		: type(type), vars(vars) {}
	void accept(Visitor&);
};

struct ConstVariable : public Variables_decl {
	ConstVariable(const std::string& type, const std::vector<std::pair<std::string, expression>>& vars)
		: Variables_decl(type, vars) {}
	void accept(Visitor&);	
};

struct Functions_decl : public Declaration {
	std::string type;
	std::string name;
	std::vector<std::pair<std::string, std::string>> parameters;
	statement block_statement;
	Functions_decl(std::string& type, std::string& name, std::vector<std::pair<std::string, std::string>> parameters, const statement& block_statement)
		: type(type), name(name), parameters(parameters), block_statement(block_statement) {}
	void accept(Visitor&);
};

///////////////////////////////////////////////////////////////////////////

struct Expression_statement : public Statement {
	expression expr;
	Expression_statement(const expression& expr) : expr(expr) {}
	void accept(Visitor&);
};

struct Block_statement : public Statement {
	std::vector<statement> body;
	Block_statement(const std::vector<statement>& body) : body(body) {}
	void accept(Visitor&);
};

struct Decl_statement : public Statement {
	declaration var;
	Decl_statement(const declaration& var) : var(var) {}
	void accept(Visitor&);
};

struct Loop_statement : public Statement {
	virtual void accept(Visitor&) = 0;
	virtual ~Loop_statement() = default;
};

struct While_statement : public Loop_statement {
	statement cond;
	statement body;
	While_statement(const statement& cond, const statement& body)
		: cond(cond), body(body) {}
	void accept(Visitor&);
};

struct For_statement : public Loop_statement {
	statement var, cond, Expr, body;
	For_statement(const statement& var, const statement& cond, const statement& Expr, const statement& body)
		: var(var), cond(cond), Expr(Expr), body(body) {}
	void accept(Visitor&);
};

struct Conditional_statement : public Statement {
	virtual void accept(Visitor&) = 0;
	virtual ~Conditional_statement() = default;
};

struct ConditionalBlock : public Conditional_statement {
	std::vector<statement> branches;
	
	ConditionalBlock(const std::vector<statement> branches)
		: branches(branches) {}
	void accept(Visitor&);
};

struct ConditionalBranches : public Conditional_statement {
	std::string key;
	statement cond;
	statement body;
	
	ConditionalBranches(std::string key, const statement& cond, const statement& body)
		: key(key), cond(cond), body(body) {}
	void accept(Visitor&);		
};

struct Jump_statement : public Statement {
	virtual void accept(Visitor&) = 0;
	virtual ~Jump_statement() = default;
};

struct Continue_statement : public Jump_statement {
	void accept(Visitor&);
};

struct Break_statement : public Jump_statement {
	void accept(Visitor&);
};

struct Return_statement : public Jump_statement {
	expression expr;
	Return_statement(const expression& expr) : expr(expr) {}
	void accept(Visitor&);
};

///////////////////////////////////////////////////////////////////////////

struct BinaryNode : public Expression {
	std::string op;
	expression left_branch, right_branch;

	BinaryNode(const std::string& op, const expression& left_branch, const expression& right_branch)
		: op(op), left_branch(left_branch), right_branch(right_branch) {}
	void accept(Visitor&);
};

struct TernaryNode : public Expression {
	expression cond, true_expression, false_expression;
	TernaryNode(const expression& cond, const expression& true_expression, const expression& false_expression) : cond(cond), true_expression(true_expression), false_expression(false_expression) {}
	void accept(Visitor&);
}; 

struct UnaryNode : public Expression {
	virtual void accept(Visitor&) = 0;
	virtual ~UnaryNode() = default;
};

struct PrefixNode : public UnaryNode {
	std::string op;
	expression branch;

	PrefixNode(const std::string& op, const expression& branch) : op(op), branch(branch) {}
	void accept(Visitor&);
};

struct PostfixNode : public UnaryNode {
	std::string op;
	expression branch;

	PostfixNode(const std::string& op, const expression& branch) : op(op), branch(branch) {}
	void accept(Visitor&);
};

struct FunctionNode : public Expression {
	std::string name;
	std::vector<expression> branches;

	FunctionNode(const std::string& name, const std::vector<expression>& branches) : name(name), branches(branches) {}
	void accept(Visitor&);
};

struct IdentifierNode : public Expression {
	std::string name;
	IdentifierNode(const std::string& name) : name(name) {}
	void accept(Visitor&);
};

struct ParenthesizedNode: public Expression {
	expression expr;
	ParenthesizedNode(const expression& expr) : expr(expr) {}
	void accept(Visitor&);
};

struct Literal : public Expression {
	virtual void accept(Visitor&) = 0;
	virtual ~Literal() = default;
};

struct IntNode : public Literal {
	int value;
	IntNode(int value) : value(value) {}
	void accept(Visitor&);	
};

struct CharNode : public Literal {
	char value;
	CharNode(char value) : value(value) {}
	void accept(Visitor&);
};

struct BoolNode: public Literal {
	bool value;
	BoolNode(bool value) : value(value) {}
	void accept(Visitor&);
};

struct StringNode: public Literal {
	std::string value;
	StringNode(std::string value) : value(value) {}
	void accept(Visitor&);
};

struct DoubleNode : public Literal {
	double value;
	DoubleNode(double value) : value(value) {}
	void accept(Visitor&);
};