#include <pyliaison.h>
#include <iostream>
#include <vector>
#include <math.h>

// We'll be calling this function from python
double Mycos( double d )
{
	return cos( d );
}

int main( int argc, char ** argv )
{
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// In order to expose a function into python, we have to declare it
		// within a custom module. Custom modules should be defined before the
		// interpreter is initialized (though I don't know if this is necessary)

		// Create the module def (pylFuncs is the name of the module)
		// This macro returns a pointer to the module def, which is stored in a std::map
		pyl::ModuleDef * pModDef = CreateMod( pylFuncs );
		if ( pModDef == nullptr )
			throw pyl::runtime_error( "Error creating module!" );

		// Add the Mycos function to the module
		AddFnToMod( pModDef, Mycos );

		// Initialize the interpreter
		pyl::initialize();

		// Import our module and invoke the function
		pyl::RunCmd( "import pylFuncs" );
		pyl::RunCmd( "print('The cosine of 0 is', pylFuncs.Mycos(0.))" );

		// We can also invoke python functions from C++
		// Declare a function that delimits strings
		pyl::RunCmd( "def delimit(str, d):\n\
					     return str.split(d)" );

		// Get the main module and call that function
		pyl::Object obMain = pyl::GetMainModule();

		// The function returns a list of strings, so
		// we will convert it to a std::vector of strings
		// (though any sequence container should work)		
		std::string strSentence = "My name is John";
		std::vector<std::string> vWords;
		if ( obMain.call( "delimit", strSentence, " " ).convert( vWords ) == false )
			throw pyl::runtime_error( "Error getting string back!" );

		// Print out the strings we got
		std::cout << "We turned " << strSentence << " into\n";
		for ( std::string& str : vWords )
			std::cout << str << "\n";
		std::cout << std::endl;

		// finalie the interpeter
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