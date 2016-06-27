# PyLiaison

A library that easily allows users to embed python interpreters in C++ applications and facilitates data transfer to and from the interpreter.

```C++
pyl::RunCmd( "print('Hello from Python!')" );
```

## Purpose

The purpose of this library is to allow for projects that make use of both dynamic and compiled code to obtain a balance between performance and iteration time. C++ code can be exposed to and driven by python scripts via the use of custom modules created at runtime, and python objects (including modules and functions) can be encapsulated into C++ objects for scripted interfaces and components. 

Data can also be passed to and from the interpreter freely, and custom overloads can be written to convert python objects to C++ types or allocate python objects from C++ types. 

## Examples

Here is a simple example that shows how you can communicate with the interpreter. The interpreter is initalized, and the main module is retrieved as a pyl::Object (a shared ptr type meant to mimic mutable python types.) 

```C++
// Initialize the python interpreter
pyl::initialize();

// Get the main module, which is stored as python object
pyl::Object obMain = pyl::GetMainModule()

// The attributes of an object can be accessed freely via templated set and get functions.

// Create a object in the module (an int named iVal)
int iVal( 12345 );
obMain.set_attr( "iVal", iVal );

// Negate the value in python
pyl::RunCmd( "iVal = -iVal" );

// Get the value and convert it to an int
int iNegVal( 0 );
obMain.get_attr( "iVal", iNegVal'

// Shut down the interpreter
pyl::finalize();
```

### Modules

Custom modules can be declared in C++. The modules can contain interfaces to functions, class definitions, and member functions that can be imported by the interpreter.

```C++
// We'll declare this function in a custom module
double mycos( double d ) {
    return cos( d );
}

// Define a module named pylFuncs
pyl::ModuleDef * pModDef = CreateMod( pylFuncs );

// Add the Mycos function to the module
AddFnToMod( pModDef, mycos );

// Initialize the interpreter
pyl::initialize();

//Import our module and invoke the function
pyl::RunCmd( "import pylFuncs" );
pyl::RunCmd( "print('The cosine of 0 is', pylFuncs.Mycos(0.))" );
```

Like real python modules, custom modules can be wrapped by a pyl::Object and manipulated like anything else. 

```C++
// Get the os.path module as an pyl::Object, which is backed by a shared_ptr 
pyl::RunCmd( "from os import path" );
pyl::Object obPathMod = pyl::GetMainModule().get_attr( "path" );

// Invoke the abspath function on the current file, store value
std::string filePath;
obPathMod.call("abspath", __FILE__).convert(filePath);
```

### Functions and Scripts
Suppose I write a script (in a file named script.py) with a function that delimits strings, returning the individual strings in a list:

```python
# script.py
def delimit(str, d):
    return str.split(d)
```
    
Scripts can be loaded into pyl::Objects. Any object with a callable variable can be accessed by using a pyl::Object's call function. 

```C++
// Load the script from disk into a pyl::Object
pyl::Object obScript = pyl::Object::from_script("script.py");

// We'll turn this sentence into a vector of words
std::string sentence = "My name is john";
std::vector<std::string> vWords;

// Call the function, convert the return value into a vector of strings
obScript.call("delimit", sentence, " ").convert(vWords);
```

### Objects and Classes
    
A pyl::Object is meant to behave similarly to a PyObject, which is the general purpose type used to contain just about every variable available within the python interpreter. They own a std::shared_ptr to the object, meaning the object will stay alive for as long as it is passed around (like a mutable python object.)

If I were to declare a Foo instance f in this script, I could store it in C++ inside a pyl::Object

```python
# Foo.py
class Foo:
    def __init__(self):
        pass
        
    def doSomething(self):
        print('Something')
        
f = Foo()
```

The object can be stored as a C++ variable or declared into another pyl::Object. 

```C++
// Get the object from the script and store it in a pyl::Object
pyl::Object obFooInst = pyl::Object::from_script("Foo.py").get_attr("f");

// Invoke a member function using the instance
obFooInst.call("doSomething");

// Declare a reference to the original object in the main module
pyl::GetMainModule().set_attr("f", obFooInst);
```

Instances of C++ structs and classes can also be exposed into the interpreter by reference.
The definitions of the object are stored in a custom module like so:

```C++
// The C++ Bar class definition
class Bar{
    int x;
public:
    Bar() : x(0) {}
    void setX(int x) { this->x = x);
    int getX() return { x);
};

// Create a pyl module
pyl::ModuleDef * pBarMod = CreateMod( pylBar );

// Add the class definiiton and member functions
AddClassDefToMod(pBarMod, Bar);
AddMemFnTomod(pBarMod, setX, void, int);
AddMemFnTomod(pBarMod, getX, int);

// Now I can use the module definition to declare
// wrapper variable to a Bar instance named cppBar
Bar B;
pBarMod->Expose_Object("cppBar", B);
```

With the Bar instance B exposed into the interpreter,
I can execute the following python code

```python
# In some python environment
x = cppBar.getX()
cppBar.setX(1)
```

The cppBar variable is tied to the Bar instance B that we exposed,
and will be valid as long as that instance's address is valid.

C++ class instances can also be exposed via their pointer

```C++
// Expose a pointer to Bar instance B into the main module
pyl::GetMainModule.set_attr("ptrBar", &B);

// Assuming the Bar class definition has been imported, 
// the wrapper variable can be declared
pyl::RunCmd("cppB = pylBar.Bar(ptrB");
```
