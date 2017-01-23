#include <pyliaison.h>
#include <iostream>

int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// Initialize the python interpreter
		pyl::initialize();

		// Get the main module, which is stored as python object
		// (these are backed by std::shared_ptr<PyObject>)
		pyl::Object obMain = pyl::GetMainModule();

		// You can execute commands in the interpreter via RunCmd
		pyl::run_cmd( "print('Hello from Python!')" );

		// Import sys, get the interpeter version
		pyl::run_cmd( "import sys" );
		pyl::run_cmd( "print(sys.version_info)" );

		// Declare a string in the main module called stringFromCPP
		// and print it in the interpreter
		obMain.set_attr( "stringFromCPP", "Hello World from C++!" );
		pyl::run_cmd( "print(stringFromCPP)" );

		// Create a variable in the main module called iVal, storing
		// an integer value 12345, and print out the value
		int iVal( 12345 );
		obMain.set_attr( "iVal", iVal );
		pyl::run_cmd( "print('We got', iVal, 'from CPP')" );

		// Negate the value in python
		pyl::run_cmd( "iVal = -iVal" );

		// Get the value and convert it to an int
		// (conversions return false if they fail)
		int iNegVal( 0 );
		if ( obMain.get_attr( "iVal", iNegVal ) == false || iNegVal != -iVal )
			throw pyl::runtime_error( "Error getting iVal back!" );
		std::cout << "We sent " << iVal << ", we got " << iNegVal << " back!" << std::endl;

		// Shut down the interpreter
		pyl::finalize();

		return EXIT_SUCCESS;
	}
	// These exceptions are thrown when something in pyliaison
	// goes wrong, but they're a child ofof std::runtime_error
	catch ( pyl::runtime_error )
	{
		return EXIT_FAILURE;
	}
}