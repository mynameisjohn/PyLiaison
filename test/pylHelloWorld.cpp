#include <pyliaison.h>
#include <iostream>

int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// Initialize the python interpreter
		pyl::initialize();

		// You can execute commands in the interpreter via RunCmd
		pyl::run_cmd( "print('Hello World!')" );

		// Import sys, get the interpeter version
		pyl::run_cmd( "import sys" );
		pyl::run_cmd( "print(sys.version_info)" );

		// Shut down the interpreter
		pyl::finalize();

		return EXIT_SUCCESS;
	}
	// These exceptions are thrown when something in pyliaison
	// goes wrong, but they're a child of std::runtime_error
	catch ( pyl::runtime_error )
	{
		return EXIT_FAILURE;
	}
}