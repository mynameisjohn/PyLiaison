#include <pyliaison.h>
#include <iostream>

/* Here is the a copy of the script used in this program

# pylTestScript.py

def HelloWorld():
    print('Hello World from', __name__)

def NarrowToWide(narrowStr):
    if not(isinstance(narrowStr, bytes)):
        raise RuntimeError('Error: Byte string input needed!')
    return narrowStr.decode('utf8')

class Foo:
    def __init__(self):
        self.x = 0
        self.y = 0

    def SetX(self, x):
        self.x = x

    def SetY(self, y):
        self.y = y

    def Print(self):
        print('My values are', self.x, self.y)

fooInst = Foo()
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

		// Construct object from script (path is relative)
		pyl::Object obScript( "./pylTestScript.py" );
		
		// Call a function in the script
		obScript.call( "HelloWorld" );

		// Pass an argument into a script function
		std::string strNarrow = "Python is helpful";
		std::wstring strWide;
		obScript.call( "NarrowToWide", strNarrow ).convert( strWide );

		// Objects declared in scripts can be retreived and
		// stored as pyl::Objects, which can be converted to C types
		// In this case, get the Foo class instance fooInst from the script
		pyl::Object obFooInst = obScript.get_attr( "fooInst" );

		// Set it's 'x' member, call SetY, and call Print
		obFooInst.set_attr( "x", 12345 );
		obFooInst.call( "SetY", 54321 );
		obFooInst.call( "Print" );

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