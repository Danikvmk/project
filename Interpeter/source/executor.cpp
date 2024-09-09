#include "visitor.hpp"
void Executor::execute(std::vector<declaration>& nodes) {
	for (auto& decl : nodes) {
		decl->accept(*this);
	} 
}

void Executor::visit(Namespace_decl& root) {
	auto name = root.name;
	scopeManager.enterScope();
	for (auto& decl : root.declarations) {
		decl->accept(*this);
	}
	auto newScope = scopeManager.exitScope();
	add(name, std::make_shared<Namespace>(newScope));
}

void Executor::visit(Variables_decl& root) {
	auto type = newType(root.type);
	for (auto& var : root.vars) {
		auto name = var.first;
		result = nullptr;
		if (var.second) {
			var.second->accept(*this);
		}
		auto lvalue = std::make_shared<Lvalue>(newValue(root.type));
		add(name, std::make_shared<Variable>(type, lvalue));	
	}
}


void Executor::visit(ConstVariable& root) {
	auto type = newType(root.type);
	for (auto& var : root.vars) {
		auto name = var.first;
		result = nullptr;
		if (var.second) {
			var.second->accept(*this);
		}
		auto lvalue = std::make_shared<Lvalue>(newValue(root.type));
		add(name, std::make_shared<ConstVar>(type, lvalue));	
	}
}

void Executor::visit(Functions_decl& root) {
	auto type = newType(root.type);
	auto name = root.name;
	
	std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> arguments;
	result = nullptr;
	for (auto& param : root.parameters) {
		auto paramType = newType(param.first);
		auto paramName = param.second;
		auto value = newValue(param.first);
		auto symbol = std::make_shared<Variable>(paramType, std::make_shared<Lvalue>(value));
		arguments.push_back(std::make_pair(paramName, symbol));
	}

	if (name == "main") {
		root.block_statement->accept(*this);		
	}

	add(name, std::make_shared<Function>(type, arguments, root.block_statement));

}

void Executor::visit(Expression_statement& root) {
	root.expr->accept(*this);
}

void Executor::visit(Block_statement& root) {
	scopeManager.enterScope();
	for (auto& state : root.body) {
		state->accept(*this);
		if (continueFlag || breakFlag || returnFlag) break; 
	}
	scopeManager.exitScope(); 
}

void Executor::visit(Decl_statement& root) { 
	root.var->accept(*this);
}

void Executor::visit(While_statement& root) {
	root.cond->accept(*this);
	while (check_condition()) {
		if (auto test = std::dynamic_pointer_cast<Block_statement>(root.body); !test) { 
			scopeManager.enterScope(); root.body->accept(*this); scopeManager.exitScope(); 
		} else { root.body->accept(*this); }
		
		if (continueFlag) {continueFlag = false; root.cond->accept(*this); continue;}
		else if (breakFlag) {breakFlag = false; break;}
		else if (returnFlag) {break;}			

		root.cond->accept(*this);
	}
}

void Executor::visit(For_statement& root) {
	scopeManager.enterScope();
	if (root.var) {
		root.var->accept(*this);
	}
	root.cond->accept(*this);
	while (check_condition()) {
		if (auto test = std::dynamic_pointer_cast<Block_statement>(root.body); !test) { 
			scopeManager.enterScope(); root.body->accept(*this); scopeManager.exitScope(); 
		} else { root.body->accept(*this); }
		if (continueFlag) {continueFlag = false; root.Expr->accept(*this); root.cond->accept(*this); continue;}
		else if (breakFlag) {breakFlag = false; break;}
		else if (returnFlag) break;
		root.Expr->accept(*this);
		root.cond->accept(*this);
	}
	scopeManager.exitScope();
}

void Executor::visit(ConditionalBlock& root) {
	for (auto& branches : root.branches) {
		branches->accept(*this);
		if (condFlag) break;				
	}
	condFlag = false;
}

void Executor::visit(ConditionalBranches& root) {
	if (root.key != "else") {
		root.cond->accept(*this);
	}
	if (check_condition()) {
		if (auto test = std::dynamic_pointer_cast<Block_statement>(root.body); test) {
			root.body->accept(*this);
		} else {
			scopeManager.enterScope(); root.body->accept(*this); scopeManager.exitScope(); 
		}
		condFlag = true;
	}
}

void Executor::visit(Continue_statement&) {
	continueFlag = true;
}

void Executor::visit(Break_statement&) {
	breakFlag = true;
}

void Executor::visit(Return_statement& root) {
	if (root.expr) root.expr->accept(*this);
	returnFlag = true;
}	

void Executor::visit(BinaryNode& root) {
	if (root.op == "::") {
		root.left_branch->accept(*this);
		auto space = std::dynamic_pointer_cast<Namespace>(result);
		scopeManager.enterScope(space->scope);
		root.right_branch->accept(*this);
		scopeManager.exitScope();
	} else {
		root.left_branch->accept(*this);
		auto lhs = result;
		root.right_branch->accept(*this);
		auto rhs = result;	
		result = nullptr;
		result = binary_operations.at(root.op)(lhs, rhs);
	}
}

void Executor::visit(TernaryNode& root) {
	root.cond->accept(*this);
	if (check_condition()) {
		root.true_expression->accept(*this);
	} else {
		root.false_expression->accept(*this);
	}
}

void Executor::visit(PrefixNode& root) {
	root.branch->accept(*this);
	result = prefix_operations.at(root.op)(result);
}

void Executor::visit(PostfixNode& root) {
	root.branch->accept(*this);	
	result = postfix_operations.at(root.op)(result);
}

void Executor::visit(FunctionNode& root) {
	if (root.name == "print" || root.name == "input") {
		for (auto& branch : root.branches) {
			result = nullptr;
			branch->accept(*this);
			InOutFunctions.at(root.name)(result);
		}
		result = nullptr;
	} else {
		result = get_symbol(root.name);
		auto func = std::dynamic_pointer_cast<Function>(result);
		scopeManager.enterScope(); returnFlag = false;
		for (std::size_t i = 0; i < root.branches.size(); i++) {
			root.branches[i]->accept(*this);
			add(func->arguments[i].first, result);
			result = nullptr;
		}
		func->body->accept(*this);
		scopeManager.exitScope(); returnFlag = false;
	}
}

void Executor::visit(IdentifierNode& root) {
	result = get_symbol(root.name);
}

void Executor::visit(ParenthesizedNode& root) {
	root.expr->accept(*this);
}

void Executor::visit(IntNode& root) {
	result = std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(root.value));
}

void Executor::visit(CharNode& root) {
	result = std::make_shared<Variable>(std::make_shared<CharType>(), std::make_shared<CharValue>(root.value));
}

void Executor::visit(BoolNode& root) {
	result = std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(root.value));
}

void Executor::visit(StringNode& root) {
	result = std::make_shared<Variable>(std::make_shared<StringType>(), std::make_shared<StringValue>(root.value));
}

void Executor::visit(DoubleNode& root) {
	result = std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(root.value));
}

///////////////////////////////////////////////////////////////////////////

std::shared_ptr<Type> Executor::newType(std::string& type) {
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

std::shared_ptr<Value> Executor::newValue(std::string& v) {
	auto var = std::dynamic_pointer_cast<Variable>(result);
	double n = {};
	std::string s = {};
	if (result) {
		if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
			auto v = std::dynamic_pointer_cast<IntValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) auto v = std::dynamic_pointer_cast<IntValue>(test->value);
			n = v->value;
		} else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
			auto v = std::dynamic_pointer_cast<DoubleValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) auto v = std::dynamic_pointer_cast<DoubleValue>(test->value);
			n = v->value;
		} else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
			auto v = std::dynamic_pointer_cast<CharValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) auto v = std::dynamic_pointer_cast<CharValue>(test->value);
			n = v->value;
		} else if (check = std::dynamic_pointer_cast<BoolType>(var->type); check) {
			auto v = std::dynamic_pointer_cast<BoolValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) auto v = std::dynamic_pointer_cast<BoolValue>(test->value);
			n = v->value;
		} else if (check = std::dynamic_pointer_cast<StringType>(var->type); check) {
			auto v = std::dynamic_pointer_cast<StringValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) auto v = std::dynamic_pointer_cast<StringValue>(test->value);
			s = v->value;
		}
	}
	if (v == "int") {
		return std::make_shared<IntValue>(n);
	} else if (v == "double") {
		return std::make_shared<DoubleValue>(n);
	} else if (v == "char") {
		return std::make_shared<CharValue>(n);
	} else if (v == "bool") {
		return std::make_shared<BoolValue>(n);
	} else if (v == "string") {
		return std::make_shared<StringValue>(s);
	} else {
		return nullptr;
	}
}

void Executor::add(std::string& name, const symbol& newSymbol) {
	scopeManager.scopes.top()->add(name, newSymbol);
}

symbol Executor::get_symbol(std::string& name) {
	return scopeManager.scopes.top()->get_symbol(name);;
}

bool Executor::check_condition() {
	auto var = std::dynamic_pointer_cast<Variable>(result);
	if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
		auto tmp = std::dynamic_pointer_cast<IntValue>(var->value);
		if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<IntValue>(test->value);
		if (tmp->value) return true;
	} else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
		auto tmp = std::dynamic_pointer_cast<CharValue>(var->value);
		if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<CharValue>(test->value);
		if (tmp->value) return true;	
	} else if (check = std::dynamic_pointer_cast<BoolType>(var->type); check) {
		auto tmp = std::dynamic_pointer_cast<BoolValue>(var->value);
		if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<BoolValue>(test->value);
		if (tmp->value) return true;
	} else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
		auto tmp = std::dynamic_pointer_cast<DoubleValue>(var->value);
		if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<DoubleValue>(test->value);
		if (tmp->value) return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
#include<iostream>
const std::unordered_map<std::string, std::function<void(symbol)>> Executor::InOutFunctions = {
	{"print", [](symbol arg) {
		auto v = std::dynamic_pointer_cast<Variable>(arg);
		if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(v->type); check) {
			if (auto test = std::dynamic_pointer_cast<Lvalue>(v->value); test) {
				std::cout << std::dynamic_pointer_cast<IntValue>(test->value)->value << std::endl;
			} else std::cout << std::dynamic_pointer_cast<IntValue>(v->value)->value << std::endl;
		} else if (check = std::dynamic_pointer_cast<DoubleType>(v->type); check) {
			if (auto test = std::dynamic_pointer_cast<Lvalue>(v->value); test) {
				std::cout << std::dynamic_pointer_cast<DoubleValue>(test->value)->value << std::endl;
			} else std::cout << std::dynamic_pointer_cast<DoubleValue>(v->value)->value << std::endl;
		} else if (check = std::dynamic_pointer_cast<CharType>(v->type); check) {
			if (auto test = std::dynamic_pointer_cast<Lvalue>(v->value); test) {
				std::cout << std::dynamic_pointer_cast<CharValue>(test->value)->value << std::endl;
			} else std::cout << std::dynamic_pointer_cast<CharValue>(v->value)->value << std::endl;
		} else if (check = std::dynamic_pointer_cast<BoolType>(v->type); check) {
			if (auto test = std::dynamic_pointer_cast<Lvalue>(v->value); test) {
				std::cout << std::dynamic_pointer_cast<BoolValue>(test->value)->value << std::endl;
			} else std::cout << std::dynamic_pointer_cast<BoolValue>(v->value)->value << std::endl;
		} else if (check = std::dynamic_pointer_cast<StringType>(v->type); check) {
			if (auto test = std::dynamic_pointer_cast<Lvalue>(v->value); test) {
				std::cout << std::dynamic_pointer_cast<StringValue>(test->value)->value << std::endl;
			} else std::cout << std::dynamic_pointer_cast<StringValue>(v->value)->value << std::endl;
		}
	}}
};

const std::unordered_map<std::string, std::function<symbol(symbol, symbol)>> Executor::binary_operations = {
	{"+", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			}

		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);

			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);				
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value + v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value + v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<StringType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<StringValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<StringValue>(test->value);
			auto v2 = std::dynamic_pointer_cast<StringValue>(var2->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v2 = std::dynamic_pointer_cast<StringValue>(test->value);
			return std::make_shared<Variable>(std::make_shared<StringType>(), std::make_shared<StringValue>(v1->value + v2->value));
		}
		return nullptr;
	}},
	{"-", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value - v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value - v2->value));
			}
		}
		return nullptr;	
	}},
	{"*", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value * v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value * v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (!v1->value) {
				if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
					return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(0));
				} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
					return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(0));
				} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
					return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(0));
				} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
					return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(0));
				}
			} else {
				return var2;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value * v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value * v2->value));
			}
		}
		return nullptr;
	}},
	{"/", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(v1->value / v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(v1->value / v2->value));
			}
		}
		return nullptr;
	}},
	{"==", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<StringType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<StringValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<StringValue>(test->value);
			auto v2 = std::dynamic_pointer_cast<StringValue>(var2->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<StringValue>(test->value);
			return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value == v2->value));
		}
		return nullptr;
	}},
	{"!=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value != v2->value));
			}
		}
		return nullptr;
	}},
	{">", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value > v2->value));
			}
		}
		return nullptr;
	}},
	{">=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value >= v2->value));
			}
		}
		return nullptr;
	}},
	{"<", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value < v2->value));
			}
		}
		return nullptr;
	}}, 
	{"<=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value <= v2->value));
			}
		}
		return nullptr;
	}},
	{"||", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value || v2->value));
			}
		}
		return nullptr;
	}},
	{"&&", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(v1->value && v2->value));
			}
		}
		return nullptr;
	}},
	{"=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value = v2->value;
				return var1;	
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value = v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value = v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value = v2->value;
				return var1;		
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value = v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value = v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<StringType>(var1->type); checkFirst){
			auto v1 = std::dynamic_pointer_cast<StringValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<StringValue>(test->value);
			auto v2 = std::dynamic_pointer_cast<StringValue>(var2->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<StringValue>(test->value);
			v1->value = v2->value;
			return var1;	
		}
		return nullptr;
	}},
	{"+=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value += v2->value;
				return var1;	
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value += v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value += v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value += v2->value;
				return var1;		
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value += v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value += v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<StringType>(var1->type); checkFirst){
			auto v1 = std::dynamic_pointer_cast<StringValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<StringValue>(test->value);
			auto v2 = std::dynamic_pointer_cast<StringValue>(var2->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v1 = std::dynamic_pointer_cast<StringValue>(test->value);
			v1->value += v2->value;
			return var1;	
		}
		return nullptr;
	}},
	{"-=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value -= v2->value;
				return var1;	
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value -= v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value -= v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value -= v2->value;
				return var1;		
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value -= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value -= v2->value;
				return var1;
			}
		}
		return nullptr;
	}},
	{"*=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value *= v2->value;
				return var1;	
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value *= v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value = var2->value ? true : false;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value = var2->value ? true : false;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value = var2->value ? true : false;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value = var2->value ? true : false;;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value *= v2->value;
				return var1;		
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value *= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value *= v2->value;
				return var1;
			}
		}
		return nullptr;
	}},
	{"/=", [](symbol arg1, symbol arg2)->symbol{
		auto var1 = std::dynamic_pointer_cast<Variable>(arg1), var2 = std::dynamic_pointer_cast<Variable>(arg2);
		if (std::shared_ptr<Type> checkFirst = std::dynamic_pointer_cast<IntType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<IntValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<IntValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value /= v2->value;
				return var1;	
			}
		} else if (checkFirst = std::dynamic_pointer_cast<CharType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<CharValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<CharValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value /= v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<BoolType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<BoolValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<BoolValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value /= v2->value;
				return var1;
			}
		} else if (checkFirst = std::dynamic_pointer_cast<DoubleType>(var1->type); checkFirst) {
			auto v1 = std::dynamic_pointer_cast<DoubleValue>(var1->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var1->value); test) v1 = std::dynamic_pointer_cast<DoubleValue>(test->value);
			if (std::shared_ptr<Type> checkSecond = std::dynamic_pointer_cast<IntType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<IntValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<IntValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<DoubleType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<DoubleValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<DoubleValue>(test->value);
				v1->value /= v2->value;
				return var1;		
			} else if (checkSecond = std::dynamic_pointer_cast<CharType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<CharValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<CharValue>(test->value);
				v1->value /= v2->value;
				return var1;
			} else if (checkSecond = std::dynamic_pointer_cast<BoolType>(var2->type); checkSecond) {
				auto v2 = std::dynamic_pointer_cast<BoolValue>(var2->value);
				if (auto test = std::dynamic_pointer_cast<Lvalue>(var2->value); test) v2 = std::dynamic_pointer_cast<BoolValue>(test->value);
				v1->value /= v2->value;
				return var1;
			}
		}
		return nullptr;
	}}
};

const std::unordered_map<std::string, std::function<symbol(symbol)>> Executor::prefix_operations = {
    {"++", [](symbol arg)->symbol{
        auto var = std::dynamic_pointer_cast<Variable>(arg);
		std::shared_ptr<Lvalue> v = std::dynamic_pointer_cast<Lvalue>(var->value);
        if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<IntValue>(v->value);
            tmp->value++;
            return var;
        } else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<DoubleValue>(v->value);
            tmp->value++;
            return var;
        } else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<CharValue>(v->value);
            tmp->value++;
            return var;
        } else {
            return var;
        }
    }}, 
    {"--", [](symbol arg)->symbol{
        auto var = std::dynamic_pointer_cast<Variable>(arg);
		std::shared_ptr<Lvalue> v = std::dynamic_pointer_cast<Lvalue>(var->value);		
        if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<IntValue>(v->value);
            tmp->value--;
            return var;
        } else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<DoubleValue>(v->value);
            tmp->value--;
            return var;
        } else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<CharValue>(v->value);
            tmp->value--;
            return var;
        } else {
            return var;
        }
    }}, 
    {"-", [](symbol arg)->symbol{
        auto var = std::dynamic_pointer_cast<Variable>(arg);
        if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<IntValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<IntValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(-tmp->value));
        } else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<DoubleValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<DoubleValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(-tmp->value));
        } else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<CharValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<CharValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<CharType>(), std::make_shared<CharValue>(-tmp->value));
        } else if (check = std::dynamic_pointer_cast<BoolType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<BoolValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<BoolValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(-tmp->value));
        } else {
            return var;
        }
    }},
    {"+", [](symbol arg)->symbol{
        return arg;
    }}, 
    {"!", [](symbol arg)->symbol{
        auto var = std::dynamic_pointer_cast<Variable>(arg);
        if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<IntValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<IntValue>(test->value);
            if (tmp->value)
                return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(true));
        } else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<DoubleValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<DoubleValue>(test->value);
            if (tmp->value)
                return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(true));
        } else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<CharValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<CharValue>(test->value);
            if (tmp->value)
                return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(true));
        } else if (check = std::dynamic_pointer_cast<BoolType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<BoolValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<BoolValue>(test->value);
            if (tmp->value)
                return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(true));
        }
        return std::make_shared<Variable>(std::make_shared<BoolType>(), std::make_shared<BoolValue>(false));
    }}
};

const std::unordered_map<std::string, std::function<symbol(symbol)>> Executor::postfix_operations = {
	{"++", [](symbol arg)->symbol{
		auto var = std::dynamic_pointer_cast<Variable>(arg);
        if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<IntValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<IntValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(tmp->value++));
        } else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<DoubleValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<DoubleValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(tmp->value++));
        } else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<CharValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<CharValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<CharType>(), std::make_shared<CharValue>(tmp->value++));
        } else {
            return var;
        }
	}},
	{"--", [](symbol arg)->symbol{
		auto var = std::dynamic_pointer_cast<Variable>(arg);
        if (std::shared_ptr<Type> check = std::dynamic_pointer_cast<IntType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<IntValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<IntValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<IntType>(), std::make_shared<IntValue>(tmp->value--));
        } else if (check = std::dynamic_pointer_cast<DoubleType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<DoubleValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<DoubleValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<DoubleType>(), std::make_shared<DoubleValue>(tmp->value--));
        } else if (check = std::dynamic_pointer_cast<CharType>(var->type); check) {
            auto tmp = std::dynamic_pointer_cast<CharValue>(var->value);
			if (auto test = std::dynamic_pointer_cast<Lvalue>(var->value); test) tmp = std::dynamic_pointer_cast<CharValue>(test->value);
            return std::make_shared<Variable>(std::make_shared<CharType>(), std::make_shared<CharValue>(tmp->value--));
        } else {
            return var;
        }
	}}
};