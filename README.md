# PyLiaison

A library that easily allows users to embed python interpreters in C++ applications and facilitates data transfer to and from the interpreter.

```C++
pyl::RunCmd( "print('Hello from Python!')" );
```

## Purpose

The purpose of this library is to allow for projects that make use of both dynamic and compiled code to obtain a balance between performance and iteration time. C++ code can be exposed to and driven by python scripts via the use of custom modules, and python objects (including modules and functions) can be stored as C++ objects for scripted interfaces and components. 

Data can also be passed to and from the interpreter freely, and custom overloads can be written to convert python objects to C++ types or allocate python objects from C++ types. 

## Examples

### The Interpreter
Pyliaison works by instantiating a python interpreter and communicating with it from C++. We can execute commands with the intrepreter as if we were working with a python console. 

```C++
// Initialize the python interpreter
pyl::initialize();

// You can execute commands in the interpreter via run_cmd
pyl::run_cmd( "print('Hello World!')" );

// Import sys, get the interpeter version
pyl::run_cmd( "import sys" );
pyl::run_cmd( "print(sys.version_info)" );

// Shut down the interpreter
pyl::finalize();
```

When we run code like this we're working with what's called the main module. We can access the main module via ```pyl::main()``` and declare variables. Here we declare a C++ variable ```x```, negate it in python, and then retrieve it and store it another C++ variable ```y```;

```C++
// y = -12345
int x = 12345;
pyl::main().set_attr( "x", x );
pyl::run_cmd( "y = -x" );
int y = pyl::main().get_attr( "y" );
```

This demonstrates how we can convert C++ data to a python variable, work with it, and then retrieve it as a from the interpreter. Still, this is a bit contrived, and we can do better.

### Modules

Here we get the ```os.path``` module and use it to find the location of the current .cpp file. We do this by calling the ```dirname``` function on our current .cpp file path and converting its return value to a string. 

```C++
// strDirectory is the folder containing this .cpp file
std::string strDirectory = pyl::GetModule( "os.path" ).call( "dirname", __FILE__ );
```

Almost everything in python is an Object, which means we can even store module functions as objects and invoke them at will. 

```C++
// Get the random.gauss function as an object
pyl::Object RndGauss = pyl::GetModule( "random" ).get_attr( "gauss" );

// Get 100 points along the gaussian distribution
double mean = 0, sigma = 0.125;
std::vector<double> vGaussDist( 100 );
for ( double& g : vGaussDist )
	g = RndGauss( mean, sigma );
```

We can also declare custom modules that invoke code we write in C++. These modules can provide python code with access to C++ functions and classes (including member functions. )

```C++
// We'll add this function to a custom module
double my_cos( double d ) {
    return cos( d );
}

  // Define a module named pylMod
  pyl::ModuleDef * pModDef = pylCreateMod( pylMod );

  // Add the my_cos function to the module
  pylAddFnToMod( pModDef, my_cos );

  // Initialize the interpreter
  // (pylMod is now a built in module)
  pyl::initialize();

  // Import our module and invoke the function
  pyl::RunCmd( "import pylMod" );
  pyl::RunCmd( "print('The cosine of 0 is', pylMod.my_cos(0.))" );
```

### Scripts
Scripts can be treated like python modules, which behave the same as any python object. Here we have a script with two useful string operations - one to convert narrow to wide, and one to delimit a string by some character. 

```python
# script.py
def narrow2wide(narrow):
    return narrow.decode('utf8')
    
def delimit(str, d):
    return str.split(d)
```
    
Scripts can be loaded into pyl::Objects. Any object with a callable variable can be accessed by using a pyl::Object's call function. 

```C++
// Load the script from disk into a pyl::Object
pyl::Object script("script.py");

// convert the string to wide characters
std::string strSentence = "My name is John";
std::wstring strSentenceW = script.call("narrow2wide", strSentence);

// Split the string by space, store as a vector
// vWords = {"My", "name", "is", "John"}
std::vector<std::string> vWords = script.call( "delimit", strIn, " " );
```

### Conversions
This last example demonstrates an implicit conversion from the return value of ```str.delimit```, which is a python list of strings, to a C++ ```std::vector<std::string>```. Many of these conversions are already implemented - for example we can turn a python ```dict``` into a std::map.

```
pyl::run_cmd( "charDict = {ord(c) : c for c in 'My name is John'}" );
std::map<int, std::string> charMap = pyl::main().get_attr( "charDict" );
```
These conversions may fail - for example we cannot convert a python list to an int. The code below will raise a ```pyl::runtime_error```, which is an exception type derived from ```std::runtime_error```. 
```C++
try
{
    pyl::run_cmd( "data = [1, 2, 3, 4]" );
    int i = pyl::main().get_attr( "data" );
}
catch ( pyl::runtime_error e )
{
	pyl::print_error();
}
```
If we aren't sure about a conversion, we can invoke a special ```convert``` function that returns a bool indicating if the conversion was successful. 
```C++
int i;
pyl::run_cmd( "data = [1, 2, 3, 4]" );
if ( !pyl::main().get_attr( "data" ).convert( i ) )
	std::cout << "data was not an int..." << std::endl;
    
std::vector<int> iData;
if (pyl::main().get_attr( "data" ).convert(iData))
	std::cout << "data was an int list, though" << std::endl;
```

We can go in the other direction to, and allocate python objects from C++ data. 
```C++
std::map<std::string, int> wordMap {
	{ "One",   1 },
	{ "Two",   2 },
	{ "Three", 3 },
};        
pyl::main().set_attr( "wordDict", wordMap );
pyl::run_cmd( "print(wordDict)" );
// {b'One': 1, b'Three': 3, b'Two': 2}
```

Custom conversions can also be defined provided you have sufficent knowledge of the python API. See the <a href="https://github.com/mynameisjohn/PyLiaison/blob/master/test/pylTestOverloads.cpp">```pylTestOverloads.cpp```</a> for an example of how this works. 

### Classes
    
Because almost anything in python is considered an object it's straightforward for us to get at python class instances from the interpreter and treat them as objects. For example, if I have a script called Foo.py with this class definition:

```python
# Foo.py
import random
class Foo:
    def __init__(self):
        self.value = random.randint(0, 100)     
    def getValue(self):
        return self.value        
    def setValue(self, value):
        return self.value = value
    def print(self):
       print(self.value)
```

Then I can get instances of Foo from the interpreter and work with them in C++. 

```C++
// Instantiate a Foo instance and retrieve it as a pyl::Object
pyl::run_cmd("from Foo import Foo");
pyl::run_cmd("f = Foo()");
pyl::Object f = pyl::main().get_attr("f");

// Invoke member functions using the instance
int val = f.call("getValue");
f.call("setValue", -val);
f.call("print");
```

Instances of C++ structs and classes can also be exposed into the interpreter by reference. To do that we must create a python class definition for our C++ class in a custom module like so:

```C++
class Foo
{
	int x, y;
public:
	Foo() : x(0), y(0) {}
	
	void SetX( int x ) { this->x = x; }
	int GetX() const { return x; }

	void SetY( int x ) { this->y = x; }
	int GetY() const { return y; }
};

// Declare module with class definition
pyl::ModuleDef * pFooMod = pylCreateMod( pylFoo );
pylAddClassToMod( pFooMod, Foo );

// Add Foo's member function to its module definition
// Because of limitiations with my template programming, 
// the return type must be specified followed by all arguments
pylAddMemFnToMod( pFooMod, Foo, SetX, void, int );
pylAddMemFnToMod( pFooMod, Foo, GetX, int );
pylAddMemFnToMod( pFooMod, Foo, SetY, void, int );
pylAddMemFnToMod( pFooMod, Foo, GetY, int );
```

Now that we have a python module with Foo as a class type, we can construct a python Foo wrapper from a pointer to a real Foo instance. Once we have it in python we can rive it from the script. 

```python
# script.py

# import our custom module with the Foo class definition
from pylFoo import Foo

# This function takes a pointer to a Foo instance
def handleFoo(pFoo):
	# Construct f from pointer
	f = Foo(pFoo)
    
    # call member functions
    f.SetX(1)
    f.SetY(-f.GetX())
    print(f.GetY())
```
We can call handleFoo from C++ with a pointer to a Foo instance. 
```C++
// Declare a Foo instance and pass its address to the handleFoo function
Foo f;
pyl::Object("script.py").call("handleFoo", &f);
```
Note that we aren't actually constructing a new Foo instance in the intrepreter and are instead driving a real Foo instance. I'm working on a way to instantiate C++ objects in Python, but I don't yet have it figured out. 

We can also expose class instances into a module or script directly; for example we can declare a foo instance and then access it in the main module like so. 

```C++
// Expose the instance into the main module
// Syntax is (definitionModule, instance, targetModule)
Foo f;
pylExposeClassInMod( pylFoo, f, pyl::main() );

// Now f is accessible from the main module
pyl::run_cmd( "print(f1)" );
pyl::run_cmd( "f1.SetX(12345)" );
pyl::run_cmd( "print(f1.GetX())" );
```

I find the method of constructing classes from pointers easier, but I'm working on making this a smoother process. 
```C++
pyl::run_cmd("import pFoo");
Foo f;
pyl::main().set_attr("pFoo", &f);
pyl::run_cmd("f = pylFoo.Foo(pFoo)");
pyl::run_cmd("print(f.SetY(12345))");
pyl::run_cmd("print(f.GetY())");
```
