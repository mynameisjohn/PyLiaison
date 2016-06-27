### PyLiaison

A library that easily allows users to embed python interpreters in C++ applications and facilitates data transfer to and from the interpreter.

`pyl::RunCmd( "print('Hello from Python!')" );`

## Examples

Here is a simple example that shows how you can communicate with the interpreter. The interpreter is initalized, and the main module is retrieved as a pyl::Object (a shared_ptr type meant to mimic the reference counted PyObject system.) 

    // Initialize the python interpreter
    pyl::initialize();

    // Get the main module, which is stored as python object
    pyl::Object obMain = pyl::GetMainModule()

The attrdict of an object can be accessed freely via templated set and get functions.

    // Create a object in the module (an int named iVal)
    int iVal( 12345 );
    obMain.set_attr( "iVal", iVal );

    // Negate the value in python
    pyl::RunCmd( "iVal = -iVal" );

    // Get the value and convert it to an int
    int iNegVal( 0 );
    obMain.get_attr( "iVal", iNegVal'

Finalizing the interpreter

    // Shut down the interpreter
    pyl::finalize();
