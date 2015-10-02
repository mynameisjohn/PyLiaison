#include <iostream>
using namespace std;

#include "pyliason.h"

#include "pyl_overloads.h"

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

// We'll convert a PyInt to this
struct Vector3
{
    float x, y, z;
public:
    Vector3(int X=0.f, float Y=0.f, float Z=0.f) : 
		x(X), y(Y), z(Z)
	{}
	float len(){
		return sqrt(x*x + y*y + z*z);
	}
};

// Bar conversion implementation
namespace Python
{
	// Convert a PyObjectobject to a Vector3
	// this won't work unless the PyObject
	// is really a list with at least 3 elements
    bool convert(PyObject * o, Vector3& v){
		return convert_buf(o, (float *)&v, 3); 
    }
}

// You can call this function
// from python by passing an int
// in place of b 
float testOverload(Vector3 v){
	std::cout << v.len() << std::endl;
	return v.len();
}

Foo g_Foo;

bool ExposeFuncs() {
	Py_Add_Func("testArgs", testArgs, "test adding two args");

    Py_Add_Func("testOverload", testOverload, "where do I have to implement it?");
 
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
	std::cout << &g_Foo << std::endl;
	Python::RunCmd("print(g_Foo)");
	Python::RunCmd("print(g_Foo())");
	Python::RunCmd("print(g_Foo.getFloat(2))");


	Python::RunCmd("print(g_Foo.testVoid1(2))");
	Python::RunCmd("print(g_Foo.testVoid2())");
	Python::RunCmd("print(g_Foo.testVoid3())");

	Python::RunCmd("print(testOverload([1.,2.,3.]))");

	return 0;
}
