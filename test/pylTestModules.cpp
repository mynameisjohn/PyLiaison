#include <pyliaison.h>
#include <iostream>
#include <math.h>

// This function will be called by the interpreter
double MyCos( double d )
{
	return cos( d );
}

// The purpose of this example is to show how 
// C++ code can be exposed to the interpreter
// within a module declared by pyliaison
int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// Create modules - this must be done
		// prior to initializing the interpreter
		// The name of the module can be whatever you like,
		// so long as you don't conflict with anything important
		pyl::ModuleDef * pModDef = pylCreateMod( pylTestModule );

		// Add a function to the module
		pylAddFnToMod( pModDef, MyCos );

		// Initialize the python interpreter
		pyl::initialize();

		// Import the module and call the function
		// (this could be done in a script)
		pyl::run_cmd( "import pylTestModule" );
		pyl::run_cmd( "d = 0." );
		pyl::run_cmd( "print('The cosine of', d, 'is', pylTestModule.MyCos(d))" );

		// We can also store references to python modules
		// Here we'll get the os.path module and use it 
		// to determine the absolute path of this .cpp file
		std::string strCurFile;
		pyl::Object obPathMod = pyl::GetModule( "os.path" );
		obPathMod.call( "abspath", __FILE__ ).convert( strCurFile );
		std::cout << "We are currently in " << strCurFile << " line " << __LINE__ << std::endl;

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