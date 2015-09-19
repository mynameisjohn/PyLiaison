#include <iostream>
using namespace std;

#include "pyliason.h"

int testArgs(int x, int y) {
	return x + y;
}

struct Foo {
	float getFloat(int x) {
		return float(x);
	}
	void testVoid1(int x) {
		x++;
	}
	void testVoid2() {
		int x(0);
		x++;
	}
	int testVoid3() {
		return 1;
	}
};

Foo g_Foo;

bool ExposeFuncs() {
	Py_Add_Func("testArgs", testArgs, "test adding two args");
    
	Python::Register_Class<Foo, __LINE__>("Foo");

	std::function<float(Foo *, int)> fooFn(&Foo::getFloat);
	Python::Register_Mem_Function<Foo, __LINE__>("getFloat", fooFn, "Testing a member function");

	// TODO fix void functions
	std::function<void(Foo *, int)> fooFn2(&Foo::testVoid1);
	Python::Register_Mem_Function<Foo, __LINE__>("testVoid1", fooFn2, "Testing a member function");

	Python::Register_Mem_Function<Foo, __LINE__>("testVoid2", &Foo::testVoid2, "Testing a member function");

	Python::Register_Mem_Function<Foo, int, __LINE__>("testVoid3", &Foo::testVoid3, "Testing a member function");
	return true;
}

int main() {
	ExposeFuncs();

	// All exposed functions should be exposed before this call
	Python::initialize();

	Python::RunCmd("print(testArgs(1,2))");

	Python::Expose_Object(&g_Foo, "g_Foo");

	// TODO test reference counts
	//std::cout << Python::GetTotalRefCount() << std::endl;
	Python::RunCmd("print(g_Foo)");
	Python::RunCmd("print(g_Foo())");
	Python::RunCmd("print(g_Foo.getFloat(2))");
	
	Python::RunCmd("print(g_Foo.testVoid1(2))");
	Python::RunCmd("print(g_Foo.testVoid2())");
	Python::RunCmd("print(g_Foo.testVoid3())");

	return 0;
}
