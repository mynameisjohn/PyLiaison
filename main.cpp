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
	Python::_add_Func<__LINE__, Foo>("getFloat", fooFn, METH_VARARGS, "Testing a member function");

	//std::function<void(Foo *, int)> fooFn2(&Foo::testVoid1);
	//Python::_add_Func<__LINE__, Foo>("testVoid1", fooFn2, METH_VARARGS, "Testing a member function");

	//std::function<void(Foo *)> fooFn3(&Foo::testVoid2);
	//Python::_add_Func<__LINE__, Foo>("testVoid2", fooFn3, METH_VARARGS, "Testing a member function");

	//std::function<int(Foo *)> fooFn4(&Foo::testVoid3);
	//Python::_add_Func<__LINE__, Foo>("testVoid3", fooFn4, METH_VARARGS, "Testing a member function");

	return true;
}

int main() {
	ExposeFuncs();

	// All exposed functions should be exposed before this call
	Python::initialize();

	Python::RunCmd("print(testArgs(1,2))");

	Python::Expose_Object(&g_Foo, "g_Foo");
	Python::RunCmd("print(g_Foo())");
	Python::RunCmd("print(g_Foo.getFloat(2))");
	Python::RunCmd("print(Foo_getFloat(g_Foo(), 2))");
	
	Python::RunCmd("print(g_Foo.testVoid1(2))");
	Python::RunCmd("print(g_Foo.testVoid2())");
	Python::RunCmd("print(g_Foo.testVoid3())");

    // Your options for exposing objects within these things are to
    // a) pass the pointer and call the module version of the function directly
    // b) somehow get the class constructor in the module (?),
    //    set attrs within Python::Object module
    // a) is easy, b) is preferable, the two are not mutually exclusive
    // It seems that python does allow type definitions within modules,
    // so b) may be possible
    auto script = Python::Object::from_script("script.py");
    script.call_function("SayHello");
    script.call_function("FooTest", &g_Foo);
    
	cout << "hello world" << endl;

	return 0;
}
