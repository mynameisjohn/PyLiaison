#include <iostream>
using namespace std;

#include "pyliason.h"

#include "pyl_overloads.h"

#include "pylTestClasses.h"

// Expose things to Python via the
// PyLiaison module; see implementation
// at bottom of file
void ExposeFuncs();

const static std::string ModName = "PyLiaison";

int main()
{
	// Expose functions and class functions
	ExposeFuncs();

	pyl::ModuleDef * constantsModule = pyl::ModuleDef::CreateModuleDef<struct constantModule>( "Constants", "Store constants here" );

	// All exposed functions should be exposed before this call
	pyl::initialize();

	// Declare a float called fPI in the constants module
	// with the value set below (can be done before or after import)
	float fPI = 3.14f;
	constantsModule->AsObject().set_attr( "fPI", fPI );

	// Import our custom modules
	pyl::RunCmd( "from " + ModName + " import *" );
	pyl::RunCmd( "import Constants" );

	// Doc test
	pyl::RunCmd( "print(help(Constants))" );

	// Test the float declared above. Note that this is differnent from
	// exposing an object, there is a standalone float value inside this module
	pyl::RunCmd( "print('This should be 3.14f: ', Constants.fPI)" );

	// Declare an int in the constants module called MAGIC
	// (we are proving that you can declare things after import)
	int MAGIC = 42;
	constantsModule->AsObject().set_attr( "MAGIC", MAGIC );

	// Prove that the int was exposed
	pyl::RunCmd( "print('The magic number 42 ==', Constants.MAGIC)" );

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
	pyl::ModuleDef * modH = pyl::ModuleDef::GetModuleDef( ModName );
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
		nrm_v[i] /= v.len();
	return nrm_v;
}


// Test unpacking a PyTuple
void testPyTup( pyl::Object obj )
{
	// If you make a mistake here it's on you
	std::tuple<int, float, int> t;
	obj.convert( t );
	std::cout << "We've received <" << std::get<0>( t ) << ", " << std::get<1>( t ) << ", " << std::get<2>( t ) << "> from a python tuple" << std::endl;
}

// Expose all Python Functions
void ExposeFuncs()
{
	pyl::ModuleDef * mod = pyl::ModuleDef::CreateModuleDef<struct MyMod>( ModName );

	// add testArgs(x, y): to the PyLiaison module
//	Py_Add_Func("testArgs", testArgs, "test adding two args");
	mod->RegisterFunction<struct testAddArgsT>( "testAddArgs",
												pyl::make_function( testAddArgs ), "test passing args" );

	// add testOverload(v): to the PyLiaison module
	//Py_Add_Func("testOverload", pyl::make_function(testArgs), "where do I have to implement it?");
	mod->RegisterFunction<struct testDataTransferT>( "testOverload",
													 pyl::make_function( testDataTransfer ), "test overloading a PyObject * conversion" );

	mod->RegisterFunction<struct testPyTupT>( "testPyTup",
											  pyl::make_function( testPyTup ), "test passing something the host converts" );

	// Register the Foo class
	mod->RegisterClass<Foo>( "Foo" );

	// Register member functions of Foo
	std::function<Vector3( Foo * )> Foo_getVec( &Foo::getVec );
	mod->RegisterMemFunction<Foo, struct Foo_getVecT>( "getVec", Foo_getVec,
													   "Get the m_Vec3 member of a Foo instance" );

	std::function<void( Foo *, Vector3 )> Foo_setVec( &Foo::setVec );
	mod->RegisterMemFunction<Foo, struct Foo_setVecT>( "setVec", Foo_setVec,
													   "Set the m_Vec3 member of a Foo instance with a list" );

	std::function<void( Foo * )> Foo_nrmVec( &Foo::normalizeVec );
	mod->RegisterMemFunction<Foo, struct Foo_nrmVecT>( "normalizeVec", Foo_nrmVec,
													   "normalize the m_Vec3 member of a Foo instance" );

	// Expose one function of Vector3, just to prove it's possible
	mod->RegisterClass<Vector3>( "Vector3" );

	std::function<float( Vector3 * )> Vec3_len( &Vector3::len ); // len is a keyword
	mod->RegisterMemFunction<Vector3, struct Vec3_lenT_>( "length()", Vec3_len,
														  "get the length, or magnitude, of the vector" );
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
