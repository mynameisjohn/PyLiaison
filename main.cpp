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
};

Foo g_Foo;

bool ExposeFuncs() {
	Py_Add_Func("testArgs", testArgs, "test adding two args");

	std::function<float(Foo *, int)> fooFn(&Foo::getFloat);
	Python::Register_Class<Foo>("Foo");
	Python::_add_Func<__LINE__, Foo>("getFloat", fooFn, METH_VARARGS, "Testing a member function");

	return true;
}

int main() {
	ExposeFuncs();

	// All exposed functions should be exposed before this call
	Python::initialize();

	Python::RunCmd("print spam.testArgs(1,2)");

	Python::Expose_Object(&g_Foo, "g_Foo");
	Python::RunCmd("print g_Foo()");
	Python::RunCmd("print g_Foo.getFloat(2)");
	Python::RunCmd("print spam.Foo_getFloat(g_Foo(), 2)");

	cout << "hello world" << endl;

	cout << (Python::ExposedClasses.begin()->second.classDef) << endl;
	return 0;
}
