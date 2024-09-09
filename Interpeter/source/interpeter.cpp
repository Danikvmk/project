#include "interpreter.hpp"

#include "readmanager.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include "visitor.hpp"

Interpreter::Interpreter(const char* input) {
    readManager readmanager{std::string(input)};
	auto buf = readmanager.get();

    Lexer l(buf);
    auto tokens = l.tokenize();

    Parser p(tokens);
    nodes = p.parse();
}

void Interpreter::print() {
    Printer printer;
    printer.print(nodes);
}

void Interpreter::analyze() {
    Analyzer analyzer;
    analyzer.analyze(nodes);
}

void Interpreter::execute() {
    Executor executor;
    executor.execute(nodes);
}
