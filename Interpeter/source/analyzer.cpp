#include "visitor.hpp"

void Analyzer::analyze(std::vector<declaration>& nodes) {
	for (auto& decl : nodes) {
		decl->accept(*this);
	} 
}

void Analyzer::visit(Namespace_decl& root) {
	auto name = root.name;
	scopeManager.enterScope();
	for (auto& decl : root.declarations) {
		decl->accept(*this);
	}
	auto newScope = scopeManager.exitScope();
	add(name, std::make_shared<Namespace>(newScope));
}

void Analyzer::visit(Variables_decl& root) {
	auto type = newType(root.type);
	if (auto test = std::dynamic_pointer_cast<VoidType>(type); test) {
		throw std::runtime_error("variable declared void");
	}
	
	for (auto& var : root.vars) {
		auto name = var.first;
		result = nullptr;
		if (var.second) {
			var.second->accept(*this);
			std::shared_ptr<Type> currType;
			if (auto tmp = std::dynamic_pointer_cast<Variable>(result); tmp) {
				currType = tmp->type;
			} else {
				throw std::runtime_error("error in variable declaration entry");
			}
			
			if (std::shared_ptr<Type> left = std::dynamic_pointer_cast<ArithmeticType>(type); left) {
				if (std::shared_ptr<Type> right = std::dynamic_pointer_cast<CompoundType>(currType); right) {
					throw std::runtime_error("invalid conversion from compound type to arithmetic type");
				}
			} else if (left = std::dynamic_pointer_cast<CompoundType>(type); left) {
				if (std::shared_ptr<Type> right = std::dynamic_pointer_cast<ArithmeticType>(currType); right) {
					throw std::runtime_error("invalid conversion from arithmetic type to compound type");
				}
			}
		}
		auto lvalue = std::make_shared<Lvalue>();
		add(name, std::make_shared<Variable>(type, lvalue));	
	}
}


void Analyzer::visit(ConstVariable& root) {
	auto type = newType(root.type);
	if (auto test = std::dynamic_pointer_cast<VoidType>(type); test) {
		throw std::runtime_error("variable declared void");
	}
	
	for (auto& var : root.vars) {
		auto name = var.first;
		
		if (var.second) {
			var.second->accept(*this);
			std::shared_ptr<Type> currType;
			if (auto tmp = std::dynamic_pointer_cast<Variable>(result); tmp) {
				currType = tmp->type;
			} else {
				throw std::runtime_error("error in const variable declaration entry");
			}
			
			if (std::shared_ptr<Type> left = std::dynamic_pointer_cast<ArithmeticType>(type); left) {
				if (std::shared_ptr<Type> right = std::dynamic_pointer_cast<CompoundType>(currType); right) {
					throw std::runtime_error("invalid conversion from compound type to arithmetic type");
				}
			} else if (left = std::dynamic_pointer_cast<CompoundType>(type); left) {
				if (std::shared_ptr<Type> right = std::dynamic_pointer_cast<ArithmeticType>(currType); right) {
					throw std::runtime_error("invalid conversion from arithmetic type to compound type");
				}
			}
		}
		auto rvalue = std::make_shared<Rvalue>();
		add(name, std::make_shared<ConstVar>(type, rvalue));	
	}
}

void Analyzer::visit(Functions_decl& root) {
	auto type = newType(root.type);
	auto name = root.name;
	
	std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> arguments;
	
	for (auto& param : root.parameters) {
		auto paramType = newType(param.first);
		auto paramName = param.second;
		auto value = newValue(param.first);
		auto symbol = std::make_shared<Variable>(paramType, std::make_shared<Lvalue>(value));
		arguments.push_back(std::make_pair(paramName, symbol));
	}

	add(name, std::make_shared<Function>(type, arguments, root.block_statement));

	scopeManager.enterScope();
	for (auto& arg : arguments) {
		auto argName = arg.first;
		auto symbol = arg.second;
		add(argName, symbol);		
	}
	
	returnType = type; returnFlag = false;
	root.block_statement->accept(*this);
	if (auto test = std::dynamic_pointer_cast<VoidType>(type); !test && !returnFlag) throw std::runtime_error("no return statement in function returning non-void");
	returnType = nullptr; returnFlag = false; 
	scopeManager.exitScope();
}

void Analyzer::visit(Expression_statement& root) {
	root.expr->accept(*this);
}

void Analyzer::visit(Block_statement& root) {
	scopeManager.enterScope(); loopCount++;
	for (auto& state : root.body) {
		state->accept(*this);
	}
	scopeManager.exitScope(); loopCount--;
}

void Analyzer::visit(Decl_statement& root) { 
	root.var->accept(*this);
}

void Analyzer::visit(While_statement& root) {
	if (root.cond) {
		if (auto test = std::dynamic_pointer_cast<Expression_statement>(root.cond); !test) {
			throw std::runtime_error("not expression statement after while()");
		}
		root.cond->accept(*this);
		if (auto test = std::dynamic_pointer_cast<Block_statement>(root.body); !test) { 
			scopeManager.enterScope(); loopCount++; root.body->accept(*this); loopCount--; scopeManager.exitScope(); 
		} else {
			root.body->accept(*this);
		}		
	} else {
		throw std::runtime_error("there is no expression in while()");
	}
}

void Analyzer::visit(For_statement& root) {
	if (!root.cond)
		throw std::runtime_error("there is no condition expression if for()");
	scopeManager.enterScope();
	if (root.var) {
		root.var->accept(*this);
	}
	root.cond->accept(*this);
	if (auto test = std::dynamic_pointer_cast<Block_statement>(root.body); !test) { 
		scopeManager.enterScope(); loopCount++; root.body->accept(*this); loopCount--; scopeManager.exitScope(); 
	} else {
		root.body->accept(*this);
	}
	root.Expr->accept(*this);
	scopeManager.exitScope();
}

void Analyzer::visit(ConditionalBlock& root) {
	if (auto test = std::dynamic_pointer_cast<ConditionalBranches>(root.branches[0]); test) {
		if (test->key != "if") {
			throw std::runtime_error("'else' without a previous 'if'");
		}
	}
	
	for (auto& branches : root.branches) {
		branches->accept(*this);				
	}
}

void Analyzer::visit(ConditionalBranches& root) {
	if (auto test = std::dynamic_pointer_cast<Expression_statement>(root.cond); !test && root.key != "else") {
		throw std::runtime_error("not expression statement after " + root.key);	
	}
	if (root.key != "else") {
		root.cond->accept(*this);
	}
	if (auto test = std::dynamic_pointer_cast<Block_statement>(root.body); !test) {
		scopeManager.enterScope(); loopCount++; root.body->accept(*this); loopCount--; scopeManager.exitScope(); 
	} else {
		root.body->accept(*this);
	}
}

void Analyzer::visit(Continue_statement&) {
	if (!loopCount) {
		throw std::runtime_error("continue statement not within a loop");
	}
}

void Analyzer::visit(Break_statement&) {
	if (!loopCount) {
		throw std::runtime_error("break statement not within a loop");
	}
}

void Analyzer::visit(Return_statement& root) {
	if (auto test = std::dynamic_pointer_cast<VoidType>(returnType); test && !root.expr) {
		throw std::runtime_error("return-statement with a value, if function returning 'void'");
	}
	
	result = nullptr;
	root.expr->accept(*this);
	std::shared_ptr<Type> type;
	if (auto tmp = std::dynamic_pointer_cast<Variable>(result); tmp) {
		type = tmp->type;
	} else if (tmp = std::dynamic_pointer_cast<ConstVar>(result); tmp) {
		type = tmp->type;
	} else {
		throw std::runtime_error("error in return statement entry");
	}
		
	if (std::shared_ptr<Type> test = std::dynamic_pointer_cast<ArithmeticType>(returnType); test) {
		if (std::shared_ptr<Type> test2 = std::dynamic_pointer_cast<CompoundType>(type); test2) {
			throw std::runtime_error("invalid conversion from compound type to arithmetic type");
		}
	} else if (test = std::dynamic_pointer_cast<CompoundType>(returnType); test) {
		if (std::shared_ptr<Type> test2 = std::dynamic_pointer_cast<ArithmeticType>(type); test2) {
			throw std::runtime_error("invalid conversion from arithmetic type to compound type");
		}
	}
	returnFlag = true;
}	

void Analyzer::visit(BinaryNode& root) {
	root.left_branch->accept(*this);
	std::shared_ptr<Symbol> first = result; result = nullptr;
	std::shared_ptr<Symbol> second;
	if (root.op != "::") {
		root.right_branch->accept(*this);
		second = result; result = nullptr;
	}

	

	if (assignment_operators.contains(root.op)) {
		std::shared_ptr<Variable> lhs = std::dynamic_pointer_cast<ConstVar>(first);
		auto rhs = std::dynamic_pointer_cast<Variable>(second);
		if (lhs) {
			throw std::runtime_error("assignment of read-only variable");
		} else {
			lhs = std::dynamic_pointer_cast<Variable>(first);
		}
		
	
		if (auto test = std::dynamic_pointer_cast<Rvalue>(lhs->value); test) {
			throw std::runtime_error("attempt to assign a value to a rvalue object");
		}
		if (std::shared_ptr<Type> test = std::dynamic_pointer_cast<ArithmeticType>(lhs->type); test) {
			if (auto test2 = std::dynamic_pointer_cast<CompoundType>(rhs->type)) {
				throw std::runtime_error("incorrect operation with arithmetic type and compound type");
			}
		} else if (test = std::dynamic_pointer_cast<CompoundType>(lhs->type); test) {
			if (auto test2 = std::dynamic_pointer_cast<ArithmeticType>(rhs->type); test2) {
				throw std::runtime_error("incorrect operation with compound type and arithmetic type");
			}
			if (test = std::dynamic_pointer_cast<StringType>(test); test) {
				if (auto test2 = std::dynamic_pointer_cast<StringType>(rhs->type); test2) {
					if (root.op != "=" && root.op != "+=") {
						throw std::runtime_error("Invalid operation to String Type");
					}
				}
			}
			
		}
		result = lhs;
	} else if (binary_operators.contains(root.op)) {
		auto lhs = std::dynamic_pointer_cast<Variable>(first), rhs = std::dynamic_pointer_cast<Variable>(second);
		if (std::shared_ptr<Type> test1 = std::dynamic_pointer_cast<CompoundType>(lhs->type); test1) {
			if (auto test2 = std::dynamic_pointer_cast<CompoundType>(rhs->type); test2) {
				if (test1 = std::dynamic_pointer_cast<StringType>(test1), test2 = std::dynamic_pointer_cast<StringType>(test2); test1 && test2) {
					if (root.op != "+") {
						throw std::runtime_error("Invalid operation between string types");
					}
					result = std::make_shared<Variable>(lhs->type, std::make_shared<Rvalue>());
				} else {
					throw std::runtime_error("");
				}
			} else {
				throw std::runtime_error("Invalid operation between Compound type and Arithmetic Type");
			}
		} else if (test1 = std::dynamic_pointer_cast<ArithmeticType>(lhs->type); test1) {
			if (auto test2 = std::dynamic_pointer_cast<ArithmeticType>(rhs->type); test2) {
				if (test1 = std::dynamic_pointer_cast<DoubleType>(test1); test1) {
					result = std::make_shared<Variable>(lhs->type, std::make_shared<Rvalue>());
				} else if (test2 = std::dynamic_pointer_cast<DoubleType>(test2); test2) {
					result = std::make_shared<Variable>(rhs->type, std::make_shared<Rvalue>());
				} else {
					result = std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<Rvalue>());
				}
			}
		}
	} else if (logical_operators.contains(root.op)) {
		result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<Rvalue>());
	} else if (compare_operators.contains(root.op)) {
		auto lhs = std::dynamic_pointer_cast<Variable>(first), rhs = std::dynamic_pointer_cast<Variable>(second);
		if (std::shared_ptr<Type> test1 = std::dynamic_pointer_cast<ArithmeticType>(lhs->type); test1) {
			if (auto test2 = std::dynamic_pointer_cast<ArithmeticType>(rhs->type); test2) {
				result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<Rvalue>());
			} else {
				throw std::runtime_error("Invalid operation between Compound type and Arithmetic Type");
			}
		} else if (test1 = std::dynamic_pointer_cast<CompoundType>(lhs->type); test1) {
			if (test1 = std::dynamic_pointer_cast<StringType>(test1); test1) {
				if (auto test2 = std::dynamic_pointer_cast<StringType>(rhs->type); test2) {
					if (root.op != "==") {
						throw std::runtime_error("invalid operation between String types");
					} else {
						result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<Rvalue>());	
					}
				} else {
					throw std::runtime_error("unknown operation between such types");
				}
			}
		}
	} else if (root.op == "::") {
		auto nameSpace = std::dynamic_pointer_cast<Namespace>(first);
		if (!nameSpace) {
			throw std::runtime_error("Invalid operation with object is not a namespace");			
		}
		if (expression test1 = std::dynamic_pointer_cast<IdentifierNode>(root.right_branch); !test1) {
			if (test1 = std::dynamic_pointer_cast<FunctionNode>(root.right_branch); !test1) {
				throw std::runtime_error("Invalid appeal");
			}
		}
		scopeManager.enterScope(nameSpace->scope);
		root.right_branch->accept(*this);
		scopeManager.exitScope();	
	}
}

void Analyzer::visit(TernaryNode& root) {
	root.cond->accept(*this);
	root.true_expression->accept(*this);
	root.false_expression->accept(*this);
}

//++ -- - + !

void Analyzer::visit(PrefixNode& root) {
	root.branch->accept(*this);
	if (std::shared_ptr<Variable> var = std::dynamic_pointer_cast<ConstVar>(result); var) {
		if (root.op == "++" || root.op == "--") {
			throw std::runtime_error("unary operation on constant variable");
		}
		result = std::make_shared<Variable>(var->type, std::make_shared<Rvalue>());
	} else if (var = std::dynamic_pointer_cast<Variable>(result); var) {
		if (std::shared_ptr<Type> test = std::dynamic_pointer_cast<CompoundType>(var->type); test) {
			throw std::runtime_error("invalid operation to compound type");
		} else if (test = std::dynamic_pointer_cast<BoolType>(var->type); test) {
		throw std::runtime_error("Invalid unary operations with bool type");
		}
		if (auto test = std::dynamic_pointer_cast<Rvalue>(var->value); test) {
			if (root.op == "++" || root.op == "--") {
				throw std::runtime_error("invalid unary operation");			
			}
			if (root.op == "!") {
				result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<Rvalue>());
			}
		}
		if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) {
			if (root.op == "-" || root.op == "+") {
				result = std::make_shared<Variable>(var->type, std::make_shared<Rvalue>());
			} else if (root.op == "!") {
				result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<Rvalue>());
			}
		}
	}
}

void Analyzer::visit(PostfixNode& root) {
	root.branch->accept(*this);	
	if (root.op != "++" && root.op != "--") {
		throw std::runtime_error("invalid unary operation");
	}
	
	if (auto var = std::dynamic_pointer_cast<ConstVar>(result); var) {
		throw std::runtime_error("unary operation on constant variable");
	}

	
	auto var = std::dynamic_pointer_cast<Variable>(result);
	if (auto test = std::dynamic_pointer_cast<Rvalue>(var->value); test) {
		throw std::runtime_error("invalid unary operation");
	}
	if (std::shared_ptr<Type> test = std::dynamic_pointer_cast<CompoundType>(var->type); test) {
		throw std::runtime_error("invalid operation to compound type");
	} else if (test = std::dynamic_pointer_cast<BoolType>(var->type); test) {
		throw std::runtime_error("Invalid unary operations with bool type");
	}
	result = std::make_shared<Variable>(var->type, std::make_shared<Rvalue>());
}

void Analyzer::visit(FunctionNode& root) {
	if (root.name == "print") {
		for (auto& branch : root.branches) {
			branch->accept(*this);
		}
		result = nullptr;
	} else {
		if (!lookup(root.name)) {
			throw std::runtime_error(root.name + " was not declared");
		}
		result = get_symbol(root.name);
		
		auto func = std::dynamic_pointer_cast<Function>(result);
		if (!func) {
			throw std::runtime_error(root.name + " is not a function");
		}	
		
		if (root.branches.size() != func->arguments.size()) {
			throw std::runtime_error("Incorrect number of arguments to function " + root.name);
		}
	
	
		for (std::size_t i = 0; i != root.branches.size(); i++) {
			root.branches[i]->accept(*this);
			auto arg = std::dynamic_pointer_cast<Variable>(result);
			auto param = std::dynamic_pointer_cast<Variable>(func->arguments[i].second);
			if (std::shared_ptr<Type> test1 = std::dynamic_pointer_cast<ArithmeticType>(arg->type); test1) {
				if (auto test2 = std::dynamic_pointer_cast<CompoundType>(param->type); test2) {
					throw std::runtime_error("Incorrect argument");
				}
			} else if (test1 = std::dynamic_pointer_cast<CompoundType>(arg->type); test1) {
				if (auto test2 = std::dynamic_pointer_cast<ArithmeticType>(param->type); test2) {
					throw std::runtime_error("Incorrect argument");
				}
			}
		}
		
		result = std::make_shared<Variable>(func->returnType, std::make_shared<Rvalue>());
	}
}

void Analyzer::visit(IdentifierNode& root) {
	if (!lookup(root.name)) {
		throw std::runtime_error(root.name + " was not declared");
	}
	result = get_symbol(root.name);
}

void Analyzer::visit(ParenthesizedNode& root) {
	root.expr->accept(*this);
}

void Analyzer::visit(IntNode&) {
	result = std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(0));
}

void Analyzer::visit(CharNode&) {
	result = std::make_shared<Variable>(std::make_shared<CharType>(), std::make_shared<CharValue>());
}

void Analyzer::visit(BoolNode&) {
	result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>());
}

void Analyzer::visit(StringNode&) {
	result = std::make_shared<Variable>(std::make_shared<StringType>(), std::make_shared<StringValue>());
}

void Analyzer::visit(DoubleNode&) {
	result = std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>());
}

///////////////////////////////////////////////////////////////////////////

std::shared_ptr<Type> Analyzer::newType(std::string& type) {
	if (type == "int") {
		return std::make_shared<IntType>();
	} else if (type == "double") {
		return std::make_shared<DoubleType>();
	} else if (type == "char") {
		return std::make_shared<CharType>();
	} else if (type == "bool") {
		return std::make_shared<BoolType>();
	} else if (type == "string") {
		return std::make_shared<StringType>();
	} else {
		return std::make_shared<VoidType>();
	}
}

std::shared_ptr<Value> Analyzer::newValue(std::string& v) {
	if (v == "int") {
		return std::make_shared<IntValue>();
	} else if (v == "double") {
		return std::make_shared<DoubleValue>();
	} else if (v == "char") {
		return std::make_shared<CharValue>();
	} else if (v == "bool") {
		return std::make_shared<BoolValue>();
	} else if (v == "string") {
		return std::make_shared<StringValue>();
	} else {
		return nullptr;
	}
}

void Analyzer::add(std::string& name, const std::shared_ptr<Symbol>& symbol) {
	scopeManager.scopes.top()->add(name, symbol);
}

bool Analyzer::lookup(std::string& name) {
	return scopeManager.scopes.top()->lookup(name);
}

std::shared_ptr<Symbol> Analyzer::get_symbol(std::string& name) {
	return scopeManager.scopes.top()->get_symbol(name);;
}

//////////////////////////////////////////////////////////////////////////////

const std::unordered_set<std::string> Analyzer::assignment_operators = {"=", "+=", "-=", "/=", "*="};
const std::unordered_set<std::string> Analyzer::binary_operators = {"+", "-", "*", "/"};
const std::unordered_set<std::string> Analyzer::compare_operators = {"==", "!=", ">", ">=", "<", "<="};
const std::unordered_set<std::string> Analyzer::logical_operators = {"||", "&&"};
