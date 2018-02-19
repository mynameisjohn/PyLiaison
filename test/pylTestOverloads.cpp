// We'll convert a python list to one of these,
// and convert one into a python list
struct Vector4 
{
	float x, y, z, w;
	float& operator[](int idx) const { return ((float *)this)[idx]; }
};

// Declare these conversions before including pyliaision.h
// This is an unfortunate necessity, because template
// functions must be made aware of their existence
#include <Python.h>
namespace pyl 
{
	// Convert a python list of 4 elements into a Vector4
	bool convert(PyObject * pObj, Vector4& v4);

	// Construct a python list from a Vector4 instance
	PyObject * alloc_pyobject(const Vector4& v4);
}

#include <pyliaison.h>
#include <iostream>
#include <iomanip>

// The purpose of this example is to show how code 
// written in a python script can be used in C++ code 
int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// Initialize the python interpreter
		pyl::initialize();

		// Allocate this as a python list
		// This invokes our pyl::alloc_pyobject
		Vector4 v4{ 1.f, 2.f, 3.f, 4.f };
		pyl::main().set_attr("v4", v4);
		pyl::run_cmd("print('Forward: ', v4)");

		// Modify in the interpreter, convert back to C struct
		pyl::run_cmd("v4.reverse()");
		pyl::main().get_attr("v4", v4);

		// Print, don't worry about the iomanip frosting
		std::cout << "Reversed: " << std::setiosflags(std::ios::fixed) << std::setprecision(1) << "[";
		for (int i = 0; i < 4; i++)
			std::cout << v4[i] << (i == 3 ? "]" : ", ");
		std::cout << std::endl;

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

// Implementations of the conversion / allocation functions
namespace pyl
{
	// Convert a python list of 4 elements into a Vector4
	bool convert( PyObject * pObj, Vector4& v4 )
	{
		// Ensure it's a list of 4 elements
		if ( PyList_Check( pObj ) && PyList_Size( pObj ) == 4 )
		{
			for ( int i = 0; i < 4; ++i )
			{
				// Convert from PyFloat to float, 
				// return false if we fail
				float val;
				if ( !convert( PyList_GetItem( pObj, i ), val ) )
					return false;
				v4[i] = val;
			}
			return true;
		}

		return false;
	}

	// Construct a python list from a Vector4 instance
	PyObject * alloc_pyobject( const Vector4& v4 )
	{
		if ( PyObject * pListRet = PyList_New( 4 ) ) // Create a list with 4 elements
		{
			for ( int i = 0; i < 4; i++ )
			{
				// Convert from float to PyFloat
				if ( PyObject * pVal = PyFloat_FromDouble( (double) v4[i] ) )
				{
					// Set the list elements
					if ( PyList_SetItem( pListRet, i, pVal ) < 0 )
					{
						// Delete and return null if failure
						PyObject_Del( pListRet );
						return nullptr;
					}
				}
				else
				{
					// Delete and return null if failure
					PyObject_Del( pListRet );
					return nullptr;
				}
			}

			return pListRet;
		}

		return nullptr;
	}
}
