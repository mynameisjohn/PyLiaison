#include <pyliaison.h>
#include <iostream>

// This is the class we'll be exposing 
// to the interpreter. Exposing classes
// to the interpeter does not actually 
// construct a new instance of the class,
// so the pointer the interpreter gets
// must remain valid. 
// Could the memory be my responsibility? 
// Only if they provide a deleter function,
// I guess.... and who would delete?
class Foo
{
	int x, y;
public:
	Foo() {}
	
	void SetX( int x ) { this->x = x; }
	int GetX() const { return x; }

	void SetY( int x ) { this->y = x; }
	int GetY() const { return y; }
};

// The purpose of this example is to show how code 
// written in a python script can be used in C++ code 
int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// In order to expose a class to the python interpreter,
		// it must be exposed as part of a module. Create the module
		// and add the class definition to it (not a specific instance)
		pyl::ModuleDef * pFooMod = pylCreateMod( pylFoo );
		pylAddClassToMod( pFooMod, Foo );

		// Add Foo's member function to its class
		// definition within the module. Because of
		// limitiations with my template programming, 
		// the return value must be specified followed by
		// any and all argument types
		pylAddMemFnToMod( pFooMod, Foo, SetX, void, int );
		pylAddMemFnToMod( pFooMod, Foo, GetX, int );
		pylAddMemFnToMod( pFooMod, Foo, SetY, void, int );
		pylAddMemFnToMod( pFooMod, Foo, GetY, int );

		// Initialize the python interpreter
		pyl::initialize();

		// Declare a foo instance - this is what we'll
		// be controlling in the interpreter
		Foo f1;

		// Import the module containing the foo
		// definition into the main module 
		pyl::run_cmd( "import pylFoo" );

		// Expose the instance into the main module
		// We go through the module definition to ensure that the python object's 
		// function calls are backed by their C++ counterparts
		pylExposeClassInMod( pylFoo, f1, pyl::GetMainModule() );

		// Make some calls
		pyl::run_cmd( "print(f1)" );
		pyl::run_cmd( "f1.SetX(12345)" );
		pyl::run_cmd( "print(f1.GetX())" );

		// We can also expose objects into the interpreter
		// via pointer - this is my preferred method
		Foo f2;
		pyl::GetMainModule().set_attr( "p_f2", &f2 );
		pyl::run_cmd( "f2 = pylFoo.Foo(p_f2)" );
		pyl::run_cmd( "f2.SetY(12345)" );
		pyl::run_cmd( "print(f2.GetY())" );

		// Shut down the interpreter
		pyl::finalize();

		return EXIT_SUCCESS;
	}
	// These exceptions are thrown when something in pyliaison
	// goes wrong, but they're a child of std::runtime_error
	catch ( pyl::runtime_error e )
	{
		std::cout << e.what() << std::endl;
		pyl::print_error();
		pyl::finalize();
		return EXIT_FAILURE;
	}
}