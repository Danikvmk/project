#include "interpreter.hpp"

int main(int, char* argv[]) {
	Interpreter inpreteter(argv[1]);

	inpreteter.print();
	inpreteter.analyze();
	inpreteter.execute();

	return 0;
}
