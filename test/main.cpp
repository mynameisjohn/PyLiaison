#include <iostream>
using namespace std;

#include "pyliason.h"

#include "pyl_overloads.h"

#include "pylTestClasses.h"

void Expose();	// Called prior to initializing interpreter
void Test();	// Called after initializing interpreter

const static std::string g_strModName = "PyLiaison";

int AddArgs( int a, int b )
{
	return a + b;
}

struct Foo
{
	int x{ 0 };
	void Bar()
	{
		std::cout << "Calling Foo::Bar on Foo instance " << this << std::endl;
	}
	int SetX( int nX )
	{
		int oldX = x;
		x = nX;
		return oldX;
	}
};

int main()
{
	pyl::ModuleDef * pFuncsModule = CreateModuleWithDocs( pylFuncs, "Functions exposed from C++" );

	AddFnToMod( pFuncsModule, AddArgs );

	AddClassToMod( pFuncsModule, Foo );
	AddMemFnToMod( pFuncsModule, Foo, Bar, void );
	AddMemFnToMod( pFuncsModule, Foo, SetX, int, int );

	pyl::initialize();
	pyl::RunCmd( "from pylFuncs import *" );
	
	int a1(1), a2(2), ret( -1 );
	pyl::GetMainModule().call_function( "AddArgs", a1, a2 ).convert( ret );
	std::cout << ret << " better be " << (a1 + a2) << std::endl;

	Foo f;
	pyl::GetMainModule().set_attr( "pFoo", &f );
	pyl::RunCmd( "cFoo = Foo(pFoo)" );
	pyl::Object pyFoo = pyl::GetMainModule().get_attr( "cFoo" );
	pyFoo.call_function( "Bar" );
	pyFoo.call_function( "SetX", ret );
}

int main()
{
	// Expose functions and class functions
	Expose();

	// As an initial test, create a module called Constants storing some constant data
	pyl::ModuleDef * pConstantsModule = CreateModuleWithDocs( pylConstants, "Store constants here" );

	// When the module is imported, the (optional) function below is called with the
	// module object itself as an argument and several attributes are declared.
	// Attributes can be declared at any point after import, but this is a convenience. 
	pConstantsModule->SetCustomModuleInit( [] ( pyl::Object obConstantsModule )
	{
		obConstantsModule.set_attr( "MAGIC", 42 );	
		obConstantsModule.set_attr( "fPI", 3.14f );	
	} );

	// All exposed functions should be exposed before this call,
	// which starts the python interpreter and allows variables to be exposed
	pyl::initialize();

	// Import our constants module directly
	pyl::RunCmd( "import pylConstants" );

	// Doc test
	pyl::RunCmd( "print(help(pylConstants))" );

	// Test the two variables we've put into the module
	pyl::RunCmd( "print('Does', pylConstants.MAGIC, '== 42?')" );
	pyl::RunCmd( "print('Does', pylConstants.fPI, '== 3.14?')" );

	// Proof that variables can be exposed at any time
	int testVal1( 2 );
	pyl::GetMainModule().set_attr( "testVal", testVal1 );
	pyl::RunCmd( "print('Does ', testVal, '== " + std::to_string( testVal1 ) + "?'" );

	// We can get data back as well
	int testVal2( 0 );
	pyl::GetMainModule().get_attr( "testVal" ).convert( testVal2 );
	std::cout << "And does " << testVal1 << " == " << testVal2 << "?" << std::endl;

	return testVal1 == testVal2 ? 0 : -1;

	// Call testArgs with 1 and 2
	pyl::RunCmd( "print('1 + 2 =', testAddArgs(1,2))" );

	// Call testOverload, passing in a Vector3 and getting one back
	pyl::RunCmd( "print(testOverload([1.,2.,3.]))" );

	// We expect to see this tuple in the testPyTup function
	pyl::RunCmd( "testPyTup((1,2.,3))" );

	// Make a foo instance
	Foo fOne;

	// Expose the Foo instance fOne into the
	// main module under the name g_Foo
	pyl::ModuleDef * modH = pyl::ModuleDef::GetModuleDef( g_strModName );
	pyl::Object modObj = modH->AsObject();
	modH->Expose_Object( &fOne, "g_Foo" );

	// Print some info
	pyl::RunCmd( "print(g_Foo)" );

	// The () operator returns the address
	pyl::RunCmd( "print(g_Foo())" );

	// Test member functions of Foo
	pyl::RunCmd( "g_Foo.setVec([1.,3.,5.])" );
	pyl::RunCmd( "print('Vector was ', g_Foo.getVec())" );
	pyl::RunCmd( "g_Foo.normalizeVec()" );
	pyl::RunCmd( "print('Vector is now ', g_Foo.getVec())" );

	// We can also expose objects directly into the PyLiaison module
	// As long as we do this before a module that imports PyLiaison
	// has been loaded, we have access to any variables predeclared
	// into the module. So let's expose it into the PyLiaison module here
	modH->Expose_Object( &fOne, "modFoo", modObj.get() );

	// Now load up our custom module
	// note that the relative path may be wrong
	auto scriptMod = pyl::Object::from_script( "../expose.py" );

	// Hello world
	scriptMod.call_function( "SayHello" );

	// Verify that the object we exposed into the PyLiaison module is there
	scriptMod.call_function( "TestModuleDecl" );

	// Alternatively, you can expose the Foo object into this script
	modH->Expose_Object( &fOne, "c_Foo", scriptMod.get() );

	// Verify that it was exposed
	bool success( false );
	scriptMod.call_function( "checkFoo" ).convert( success );
	if ( success )
		std::cout << "Foo object successfully exposed!" << std::endl;
	else
		std::cout << "Host code was unable to expose Foo object!" << std::endl;

	// Set the vec3 object in the module
	float x( 1 ), y( 0 ), z( 0 );
	scriptMod.call_function( "SetVec", x, y, z );

	// Retrieve it from the module and see what we got
	Vector3 pyVec;
	scriptMod.get_attr( "g_Vec3" ).convert( pyVec );
	std::cout << pyVec[0] << " better be " << x << std::endl;

	// You can construct Foo objects within the module
	// without having to expose them
	Foo fTwo;
	scriptMod.call_function( "FooTest", &fTwo );

	return EXIT_SUCCESS;
}

// Callable from python
int testAddArgs( int x, int y )
{
	return x + y;
}

// You can call this function; it
// takes in a list of three floats
// from python, interprets it as a Vector3,
// and normalizes it.
// See the implementation
// of the c++ <==> python conversions
// at the bottom of this file
Vector3 testDataTransfer( Vector3 v )
{
	Vector3 nrm_v = v;
	for ( int i = 0; i < 3; i++ )
		nrm_v[i] /= v.length();
	return nrm_v;
}


// Test unpacking a PyTuple
void testPyObj( pyl::Object obj )
{
	// If you make a mistake here it's on you
	std::tuple<int, float, int> t;
	obj.convert( t );
	std::cout << "We've received <" << std::get<0>( t ) << ", " << std::get<1>( t ) << ", " << std::get<2>( t ) << "> from a python tuple" << std::endl;
}

// Expose all Python Functions
void Expose()
{
	pyl::ModuleDef * mod = pyl::ModuleDef::Create<struct MyMod>( g_strModName );

	// add testArgs(x, y): to the PyLiaison module
	AddFnToMod( mod, testAddArgs, int, int, int );

	// add testOverload(v): to the PyLiaison module
	AddFnToMod( mod, testDataTransfer, Vector3, Vector3 );

	// Expose a function that takes a python object for us to deal with
	AddFnToMod( mod, testPyObj, void, pyl::Object );

	// Register the Foo class
	mod->RegisterClass<Foo>( "Foo" );

	// Register member functions of Foo
	AddMemFnToMod( mod, Foo, getVec, Vector3 );
	AddMemFnToMod( mod, Foo, setVec, void, Vector3 );
	AddMemFnToMod( mod, Foo, normalizeVec, void );

	// Expose one function of Vector3, just to prove it's possible
	mod->RegisterClass<Vector3>( "Vector3" );
	AddMemFnToMod( mod, Vector3, length, float );
}

// Implementation of conversion/allocation functions
namespace pyl
{
	// Convert a PyObjectobject to a Vector3
	// this won't work unless the PyObject
	// is really a list with at least 3 elements
	bool convert( PyObject * o, Vector3& v )
	{
		return convert_buf( o, (float *) &v, 3 );
	}

	// Convert a Vector3 to a PyList
	PyObject * alloc_pyobject( Vector3& v )
	{
		PyObject * pyVec = PyList_New( 3 );
		for ( int i = 0; i < 3; i++ )
			PyList_SetItem( pyVec, i, alloc_pyobject( v[i] ) );
		return pyVec;
	}
}
