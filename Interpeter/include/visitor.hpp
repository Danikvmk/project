#pragma once

#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "ast.hpp"
#include "symbol.hpp"

using symbol = std::shared_ptr<Symbol>;

class Visitor {
public:
	virtual void visit(Namespace_decl&) = 0;
	virtual void visit(Variables_decl&) = 0;
	virtual void visit(ConstVariable&) = 0;
	virtual void visit(Functions_decl&) = 0;

	virtual void visit(Expression_statement&) = 0;
	virtual void visit(Block_statement&) = 0;
	virtual void visit(Decl_statement&) = 0;
	virtual void visit(While_statement&) = 0;
	virtual void visit(For_statement&) = 0;
	virtual void visit(ConditionalBlock&) = 0;
	virtual void visit(ConditionalBranches&) = 0;
	virtual void visit(Continue_statement&) = 0;
	virtual void visit(Break_statement&) = 0;
	virtual void visit(Return_statement&) = 0;

	virtual void visit(BinaryNode&) = 0;
	virtual void visit(TernaryNode&) = 0;
	virtual void visit(PrefixNode&) = 0;
	virtual void visit(PostfixNode&) = 0;
	virtual void visit(FunctionNode&) = 0;
	virtual void visit(IdentifierNode&) = 0;
	virtual void visit(ParenthesizedNode&) = 0;
	virtual void visit(IntNode&) = 0;
	virtual void visit(CharNode&) = 0;
	virtual void visit(BoolNode&) = 0;	
	virtual void visit(StringNode&) = 0;
	virtual void visit(DoubleNode&) = 0;
};

class Printer : public Visitor {
public:
	void visit(Namespace_decl&);
	void visit(Variables_decl&);
	void visit(ConstVariable&);
	void visit(Functions_decl&);

	void visit(Expression_statement&);
	void visit(Block_statement&);
	void visit(Decl_statement&);
	void visit(While_statement&);
	void visit(For_statement&);
	void visit(ConditionalBlock&);
	void visit(ConditionalBranches&);
	void visit(Continue_statement&);
	void visit(Break_statement&);
	void visit(Return_statement&);

	void visit(BinaryNode&);
	void visit(TernaryNode&);
	void visit(PrefixNode&);
	void visit(PostfixNode&);
	void visit(FunctionNode&);
	void visit(IdentifierNode&);
	void visit(ParenthesizedNode&);
	void visit(IntNode&);
	void visit(CharNode&);
	void visit(BoolNode&);	
	void visit(StringNode&);
	void visit(DoubleNode&);

	void print(std::vector<declaration>&);
};

class Analyzer : public Visitor {
public:
	void analyze(std::vector<declaration>&);

	void visit(Namespace_decl&);
	void visit(Variables_decl&);
	void visit(ConstVariable&);
	void visit(Functions_decl&);

	void visit(Expression_statement&);
	void visit(Block_statement&);
	void visit(Decl_statement&);
	void visit(While_statement&);
	void visit(For_statement&);
	void visit(ConditionalBlock&);
	void visit(ConditionalBranches&);
	void visit(Continue_statement&);
	void visit(Break_statement&);
	void visit(Return_statement&);

	void visit(BinaryNode&);
	void visit(TernaryNode&);
	void visit(PrefixNode&);
	void visit(PostfixNode&);
	void visit(FunctionNode&);
	void visit(IdentifierNode&);
	void visit(ParenthesizedNode&);
	
	void visit(IntNode&);
	void visit(CharNode&);
	void visit(BoolNode&);	
	void visit(StringNode&);
	void visit(DoubleNode&);

private:
	std::shared_ptr<Type> newType(std::string&);
	std::shared_ptr<Value> newValue(std::string&);
	void add(std::string&, const std::shared_ptr<Symbol>&);
	bool lookup(std::string&);
	std::shared_ptr<Symbol> get_symbol(std::string&);

	static const std::unordered_set<std::string> assignment_operators;
	static const std::unordered_set<std::string> binary_operators;
	static const std::unordered_set<std::string> compare_operators;
	static const std::unordered_set<std::string> logical_operators;
	
	
	std::size_t loopCount = 0;
	
	bool returnFlag; 
	std::shared_ptr<Type> returnType;
	
	std::shared_ptr<Symbol> result;
	ScopeManager scopeManager;
};


class Executor : public Visitor {
public:
	void execute(std::vector<declaration>&);

	void visit(Namespace_decl&);
	void visit(Variables_decl&);
	void visit(ConstVariable&);
	void visit(Functions_decl&);

	void visit(Expression_statement&);
	void visit(Block_statement&);
	void visit(Decl_statement&);
	void visit(While_statement&);
	void visit(For_statement&);
	void visit(ConditionalBlock&);
	void visit(ConditionalBranches&);
	void visit(Continue_statement&);
	void visit(Break_statement&);
	void visit(Return_statement&);

	void visit(BinaryNode&);
	void visit(TernaryNode&);
	void visit(PrefixNode&);
	void visit(PostfixNode&);
	void visit(FunctionNode&);
	void visit(IdentifierNode&);
	void visit(ParenthesizedNode&);
	
	void visit(IntNode&);
	void visit(CharNode&);
	void visit(BoolNode&);	
	void visit(StringNode&);
	void visit(DoubleNode&);

private:
	std::shared_ptr<Type> newType(std::string&);
	std::shared_ptr<Value> newValue(std::string&);
	void add(std::string&, const symbol&);
	symbol get_symbol(std::string&);
	bool check_condition();

	static const std::unordered_map<std::string, std::function<void(symbol)>> InOutFunctions;
	static const std::unordered_map<std::string, std::function<symbol(symbol, symbol)>> binary_operations;
	static const std::unordered_map<std::string, std::function<symbol(symbol)>> prefix_operations;
	static const std::unordered_map<std::string, std::function<symbol(symbol)>> postfix_operations;
	
	bool returnFlag = false, continueFlag = false, breakFlag = false, condFlag = false; 
	
	symbol result;
	ScopeManager scopeManager;
};
