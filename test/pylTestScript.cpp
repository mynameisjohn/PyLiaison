#include <pyliaison.h>
#include <iostream>
#include <list>

// We'll be sending argv to a python script following the argparse example
// which expects a sequence of integers that it will sum or find the max of
int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// Init interpreter
		pyl::initialize();

		// We're going to import a script as an object
		pyl::Object obScript = pyl::Object::from_script( "./script.py" );

		// This script has the argparse exampel in it, so let's 
		// forward our arguments to the script. Put all args in a list
		std::list<char *> liArgs( argv, argv + argc );

		// Call the script function
		obScript.call( "ParseCPPArgs", std::list<char *>( { argv, argv + argc } ) );

		// Scripts and modules are like any objects - they contain an
		// attrDict and can have attribute set via set_attr
		obScript.set_attr( "iVal", 12345 );
		int iVal( 0 );
		if ( obScript.get_attr( "iVal", iVal ) == false )
			throw pyl::runtime_error( "Error getting int val from script!" );

		// Finalize interpreter
		pyl::finalize();
	}
	// These exceptions are thrown when something in pyliaison
	// goes wrong, but they're a child ofof std::runtime_error
	catch ( pyl::runtime_error )
	{
		pyl::print_error();
		pyl::finalize();
		return EXIT_FAILURE;
	}
}