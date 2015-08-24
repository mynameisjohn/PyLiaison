#include <iostream>
using namespace std;

#include "pyliason.h"

int testArgs(int x, float y){
    return x+int(y);
}

struct Foo{
    float getFloat(int x){
        return float(x);
    }
};

bool ExposeFuncs(){
    Py_Add_Func("testArgs", testArgs, "test adding two args");
    
    std::function<float(Foo *, int)> fooFn(&Foo::getFloat);
    Python::_add_Func<__LINE__>("Foo_getFloat", fooFn, METH_VARARGS, "Testing a member function");
    
    return true;
}

int main(){
    ExposeFuncs();
    
    // All exposed functions should be exposed before this call
    Python::initialize();
    
    Python::RunCmd("print spam.testArgs(1,2)");
    Python::RunCmd("print spam.Foo_getFloat(2)");
    
	cout << "hello world" << endl;
	return 0;
}
