## PyLiaison

A library that easily allows users to embed python interpreters in C++ applications and facilitates data transfer to and from the interpreter.

`pyl::RunCmd( "print('Hello from Python!')" );`

### Example

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
    
### Modules

Custom modules can be declared in C++. The modules can contain interfaces to functions, class definitions, and member functions from within modules. Like real python modules, custom modules can be wrapped by a pyl::Object and manipulated like anything else. 

    // We'll declare this function in a custom module
    double mycos( double d ) {
        return cos( d );
    }

    // Create the module def (pylFuncs is the name of the module)
    pyl::ModuleDef * pModDef = CreateMod( pylFuncs );

    // Add the Mycos function to the module
    AddFnToMod( pModDef, mycos );

    // Initialize the interpreter
    pyl::initialize();

    //Import our module and invoke the function
    pyl::RunCmd( "import pylFuncs" );
    pyl::RunCmd( "print('The cosine of 0 is', pylFuncs.Mycos(0.))" );
    
Modules can also be loaded from python and stored in pyl::Objects

    // Get the os.path module as an pyl::Object, which is backed by a shared_ptr 
    pyl::RunCmd( "from os import path" );
    pyl::Object obPathMod = pyl::GetMainModule().get_attr( "path" );

    // Invoke the abspath function on the current file, store value
    std::string filePath;
    obPathMod.call("abspath", __FILE__).convert(filePath);
    
### Functions and Scripts
Suppose I write a script (in a file named script.py) with a function that delimits strings, returning the individual strings in a list:

    # script.py
    def delimit(str, d):
        return str.split(d)
    
Scripts can be loaded into pyl::Objects. Any object with a callable variable can be accessed by using a pyl::Object's call function. 

    // Load the script from disk into a pyl::Object
    pyl::Object obScript = pyl::Object::from_script("script.py");

    // We'll turn this sentence into a vector of words
    std::string sentence = "My name is john";
    std::vector<std::string> vWords;

    // Call the function, convert the return value into a vector of strings
    obScript.call("delimit", sentence, " ").convert(vWords);

### Objects and Classes
    
A pyl::Object is meant to behave similarly to a PyObject, which is the general purpose type used to contain just about every variable available within the python interpreted. They own a std::shared_ptr to the object, meaning the object will stay alive for as long as it is passed around. This holds true for objects declared in the interprer as well

If I were to declare a Foo instance f in this script, I could store it in C++ inside a pyl::Object

    # Foo.py
    class Foo:
        def __init__(self):
            pass
            
        def doSomething(self):
            print('Something')
            
    f = Foo()

The object can be stored as a C++ variable or declared into another pyl::Object. 

    // Get the object from the script and store it in f
    pyl::Object f = pyl::Object::from_script("Foo.py").get_attr("f");

    // Invoke a function in f
    f.call("doSomething");

    // Declare f in the main module
    pyl::GetMainModule().set_attr("f", f);
    
Instances of C++ structs and classes can also be exposed into the interpreter by reference
The definitions of the object are stored in a custom module

class Foo{
    int x;
public:
    Foo() : x(0) {}
    void setX(int x) { this->x = x);
    int getX() return { x);
};

pyl::ModuleDef * pModDef = CreateMod( pylmod );
AddClassDefToMod(pModDef, Foo);
AddMemFnTomod(pModDef, setX, void, int);
AddMemFnTomod(pModDef, getX, int);

Foo F;

pyl::Object obMain = pyl::GetMainModule();
obMain.setAttr("ptrF", &F);
pyl::RunCmd("F = op
