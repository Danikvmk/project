#pragma once

#include "ast.hpp"

class Interpreter {
public:
    Interpreter(const char*);
    
    void print();
    void analyze();
    void execute();
private:
    std::vector<declaration> nodes;
};