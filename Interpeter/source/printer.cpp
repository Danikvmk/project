#include <iostream>
#include "visitor.hpp"

void Printer::print(std::vector<declaration>& nodes) {
	for (auto& decl : nodes) {
		decl->accept(*this);
		std::cout << "\n";
	} 
}

void Printer::visit(Namespace_decl& root) {
	std::cout << "namespace " + root.name + " {\n";
	for (auto& decl : root.declarations) {
		decl->accept(*this);
		std::cout << std::endl;
	}
	std::cout << "}\n";
}

void Printer::visit(Variables_decl& root) {
	std::cout << root.type << " ";
	for (auto& var : root.vars) {
		std::cout << var.first + " = ";
		var.second->accept(*this);	
	}
	std::cout << ";\n";
}

void Printer::visit(ConstVariable& root) {
	std::cout << "const ";
	root.Variables_decl::accept(*this);
}

void Printer::visit(Functions_decl& root) {
	std::cout << root.type + " " + root.name + "(";
	for (auto it = root.parameters.begin(); it != root.parameters.end();) {
		std::cout << it->first + " " + it->second;
		it++;
		if (it != root.parameters.end()) {
			std::cout << ", ";
		}
	}
	std::cout << ") ";
	root.block_statement->accept(*this);
}

void Printer::visit(Expression_statement& root) {
	root.expr->accept(*this);
}

void Printer::visit(Block_statement& root) {
	std::cout << "{\n";
	for (auto& state : root.body) {
		state->accept(*this);
		std::cout << std::endl;
	}
	std::cout << "}\n";
}

void Printer::visit(Decl_statement& root) { 
	root.var->accept(*this);
}

void Printer::visit(While_statement& root) {
	std::cout << "while(";
	root.cond->accept(*this);
	std::cout << ") ";
	root.body->accept(*this);
	std::cout << "\n";
}

void Printer::visit(For_statement& root) {
	std::cout << "for(";
	root.var->accept(*this);
	root.cond->accept(*this);
	std::cout << "; ";
	root.Expr->accept(*this);
	std::cout << ") ";
	root.body->accept(*this);
	std::cout << "\n";
}

void Printer::visit(ConditionalBlock& root) {
	for (auto& branch : root.branches) {
		branch->accept(*this);
	}
	std::cout << "\n";	
}

void Printer::visit(ConditionalBranches& root) {
	if (root.key != "else") {
		std::cout << "\n" + root.key + "(";
		root.cond->accept(*this);
		std::cout << ") ";
	} else {
		std::cout << "\n" << "else ";
	}
	root.body->accept(*this);
}

void Printer::visit(Continue_statement&) {
	std::cout << "continue;";
}

void Printer::visit(Break_statement&) {
	std::cout << "break;";
}

void Printer::visit(Return_statement& root) {
	std::cout << "return ";
	root.expr->accept(*this);
	std::cout << ";";
}	

void Printer::visit(BinaryNode& root) {
	root.left_branch->accept(*this);
	std::cout << " " + root.op + " ";
	root.right_branch->accept(*this);
}

void Printer::visit(TernaryNode& root) {
	root.cond->accept(*this);
	std::cout << " ? ";
	root.true_expression->accept(*this);
	std::cout << " : ";
	root.false_expression->accept(*this);
}

void Printer::visit(PrefixNode& root) {
	std::cout << root.op;
	root.branch->accept(*this);
}

void Printer::visit(PostfixNode& root) {
	root.branch->accept(*this);
	std::cout << root.op;
}

void Printer::visit(FunctionNode& root) {
	std::cout << root.name + "(";
	for (auto it = root.branches.begin(); it != root.branches.end();) {
		(*it)->accept(*this);
		it++;
		if (it != root.branches.end()) {
			std::cout << ", ";
		}
	}
	std::cout << ")";
}

void Printer::visit(IdentifierNode& root) {
	std::cout << root.name;
}

void Printer::visit(ParenthesizedNode& root) {
	std::cout << "(";
	root.expr->accept(*this);
	std::cout << ")";
}

void Printer::visit(IntNode& root) {
	std::cout << root.value;
}

void Printer::visit(CharNode& root) {
	std::cout << "\'" << root.value << "\'";
}

void Printer::visit(BoolNode& root) {
	root.value ? std::cout << "true" : std::cout << "false";
}

void Printer::visit(StringNode& root) {
	std::cout << "\"" + root.value + "\"";
}

void Printer::visit(DoubleNode& root) {
	std::cout << root.value;
}

