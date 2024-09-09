
const double a = 0.23;
double x = 2.4;
bool b = true;

namespace A {
	const int n = 6;

	int fact(int n) {
		return n == 0 ? 1 : n * fact(n - 1);
	}

	double sum(double a, double b) {
		return a + b;
	}

	double mul(double a, double b) {
		return a * b;
	}
}

void f() {
	print("call f()");	
	while(b) {
		print("check while in f()");
		b = false;
	}
}

int g(int n) {
	for (int i = 0; i < n; i++) {
		print(i, "for in g()");
		if (i > 5) {
			print("check break");
			break;
		} else if (i < 5) {
			print("check continue");
			continue;
		} else {
		}
	}
	return 0;
}

int main() {
	int res = A::fact(A::n);
	const double res1 = A::mul(A::sum(a, x), 1.2);
	print(res, res1);
	char x = 'a';
	f();
	print(g(A::n));
	print(x);
	return 0;
}
