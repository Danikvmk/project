#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <stack>
#include <iostream>

#include "type.hpp"
#include "value.hpp"
#include "ast.hpp"

struct Symbol {
    virtual ~Symbol() noexcept = default;
};

using SymbolTable = std::unordered_map<std::string, std::shared_ptr<Symbol>>;

struct Scope {
    SymbolTable table;
    std::shared_ptr<Scope> parent;

    Scope(const std::shared_ptr<Scope> parent = nullptr) : parent(parent) {}

    void add(const std::string& name, const std::shared_ptr<Symbol>& symbol) {
        if(table.contains(name)) {
            throw std::runtime_error("Redeclaration of symbol " + name + ".");
        }
        table[name] = symbol;
    }

    bool lookup(const std::string& name) {
        if (!table.contains(name)) {
            if (parent == nullptr) {
                return false;
            }
            return parent->lookup(name);
        }
        return true;
    }
    
    std::shared_ptr<Symbol> get_symbol(const std::string& name) {
    	if (!table.contains(name)) {
    	    if (parent == nullptr) {
    	    	return nullptr;
    	    }
    	    return parent->get_symbol(name);
    	}
    	return table[name];
    }
};

struct ScopeManager {
    std::stack<std::shared_ptr<Scope>> scopes;
    std::shared_ptr<Scope> global;

    ScopeManager() {
        scopes.push(std::make_shared<Scope>());
        global = scopes.top();
    }

    void enterScope() {
        scopes.push(std::make_shared<Scope>(scopes.top()));
    }

    void enterScope(std::shared_ptr<Scope>& scope) {
    	if (scope->parent != global)
	    	scope->parent = scopes.top();
    	scopes.push(scope);
    }

    std::shared_ptr<Scope> exitScope(){
    	auto tmp = scopes.top();
        scopes.pop();
        return tmp;
    }
};

struct Namespace : public Symbol {
	std::shared_ptr<Scope> scope;
	Namespace(std::shared_ptr<Scope>& scope) : scope(scope) {}
};

struct Variable : public Symbol {
    std::shared_ptr<Type> type;
    std::shared_ptr<Value> value;
    
    Variable(const std::shared_ptr<Type>& type = nullptr, const std::shared_ptr<Value>& value = nullptr)
    	: type(type), value(value) {}
};

struct ConstVar : public Variable {
    ConstVar(const std::shared_ptr<Type>& type = nullptr, const std::shared_ptr<Value>& value = nullptr)
	: Variable(type, value) {}
};

struct Function : public Symbol {
    std::shared_ptr<Type> returnType;
    std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> arguments;
    statement body;
    
    Function(std::shared_ptr<Type>& returnType, std::vector<std::pair<std::string, std::shared_ptr<Symbol>>> arguments, statement& body)
    	: returnType(returnType), arguments(arguments), body(body) {}  
};

