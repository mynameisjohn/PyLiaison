#include <iostream>
using namespace std;

#include "pyliason.h"

#include "pyl_overloads.h"

// Call this function from python
int testArgs(int x, int y) {
	return x + y;
}

// Expose this class in python
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

// You can pass one of these back and forth
// between the interpreter and host program
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
	float& operator[](int idx){
		return ((float *)this)[idx];
	}
};

// Implementation of conversion/allocation functions
namespace Python
{
	// Convert a PyObjectobject to a Vector3
	// this won't work unless the PyObject
	// is really a list with at least 3 elements
    bool convert(PyObject * o, Vector3& v){
		return convert_buf(o, (float *)&v, 3); 
    }

	// Convert a Vector3 to a PyList
	PyObject * alloc_pyobject(Vector3 v){
		PyObject * pyVec = PyList_New(3);
		for (int i=0; i<3; i++)
			PyList_SetItem(pyVec, i, alloc_pyobject(v[i]));
		return pyVec;
	}
}

// You can call this function
// from python by passing a list
// with at least three floats
// It'll normalize them and return
// the result as a list of three floats
Vector3 testOverload(Vector3 v){
	Vector3 nrm_v = v;
	for (int i=0; i<3; i++)
		nrm_v[i] /= v.len();
	return nrm_v;
}

// We'll be exposing this instance to Python
Foo g_Foo;

bool ExposeFuncs() {
	// add testArgs(x, y): to the PyLiaison module
	Py_Add_Func("testArgs", testArgs, "test adding two args");

	// add testOverload(v): to the PyLiaison module
    Py_Add_Func("testOverload", testOverload, "where do I have to implement it?");
 
	// Register the Foo class
	Python::Register_Class<Foo, __LINE__>("Foo");

	// Register Foo::getFloat(int) as a member function of Foo
	std::function<float(Foo *, int)> fooFn(&Foo::getFloat);
	Python::Register_Mem_Function<Foo, __LINE__>("getFloat", fooFn, "Testing a member function");

	// Same for all these
	std::function<void(Foo *, int)> fooFn2(&Foo::testVoid1);
	Python::Register_Mem_Function<Foo, __LINE__>("testVoid1", fooFn2, "Testing a member function");

	Python::Register_Mem_Function<Foo, __LINE__>("testVoid2", &Foo::testVoid2, "Testing a member function");

	Python::Register_Mem_Function<Foo, int, __LINE__>("testVoid3", &Foo::testVoid3, "Testing a member function");

	return true;
}

int main() {
	// Call the above function
	ExposeFuncs();

	// All exposed functions should be exposed before this call
	Python::initialize();

	// Call testArgs with 1 and 2
	Python::RunCmd("print(testArgs(1,2))");

	// Call testOverload, passing in a Vector3 and getting one back
	Python::RunCmd("print(testOverload([1.,2.,3.]))");

	// Expose the g_Foo instance of Foo
	Python::Expose_Object(&g_Foo, "g_Foo");

	// TODO test reference counts
	std::cout << &g_Foo << std::endl;
	Python::RunCmd("print(g_Foo)");
	Python::RunCmd("print(g_Foo())");

	// Test member functions of Foo
	Python::RunCmd("print(g_Foo.getFloat(2))");
	Python::RunCmd("print(g_Foo.testVoid1(2))");
	Python::RunCmd("print(g_Foo.testVoid2())");
	Python::RunCmd("print(g_Foo.testVoid3())");

	return 0;
}
