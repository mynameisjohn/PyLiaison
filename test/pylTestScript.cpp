#include <pyliaison.h>
#include <iostream>

/* Here is the a copy of the script used in this program

def HelloWorld():
print('Hello World from', __name__)

def narrow2wide(narrowStr):
	if not(isinstance(narrowStr, bytes)):
		raise RuntimeError('Error: Byte string input needed!')
	return narrowStr.decode('utf8')

def delimit(str, d):
	return str.split(d)
*/

// The purpose of this example is to show how code 
// written in a python script can be used in C++ code 
int main( int argc, char ** argv )
{	
	// We may get an exception from the interpreter if something is amiss
	try
	{
		// Initialize the python interpreter
		pyl::initialize();

		// Construct object from script 
		// Because the script file is next to this .cpp file, use the
		// os.path module to construct a path to the script
		pyl::Object obPath = pyl::GetModule( "os.path" );
		std::string strDirectory = obPath.call( "dirname", __FILE__ );
		std::string strScriptPath = obPath.call( "join", strDirectory, "pylTestScript.py" );
		pyl::Object obScript( strScriptPath );
		
		// Call a function in the script
		obScript.call( "HelloWorld" );

		// Pass an argument into a script function
		// (convert to wide string, in this case)
		std::wstring strWide = obScript.call("narrow2wide", "Python is helpful");

		std::string strIn = "My name is John";
		std::vector<std::string> vOut = obScript.call( "delimit", strIn, " " );

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