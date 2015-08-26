#include "pyl_misc.h"
#include "pyl_classes.h"

#include <iostream>
#include <typeindex>
#include <string>

#include <Python.h>

// I don't know how to declare all these without
// explicity specializing the templates... Unforunate

#define Py_Add_Func(name, fn, docs)\
	Python::_add_Func<__LINE__>(std::string(name), Python::make_function(fn), METH_VARARGS, docs)

namespace Python
{
	// Every python module function looks like this
	using PyFunc = std::function<PyObject *(PyObject *, PyObject *)>;

	// The idea is to make a module where all your functions live
	const std::string ModuleName = "spam";

	// Externs
	extern std::map<std::type_index, ExposedClass> ExposedClasses;
	extern std::string ClassesDef; // Do I still need this?
	extern std::list<PyFunc> ExposedFunctions;
	extern MethodDefinitions MethodDef;
	extern PyObject * Py_ErrorObj;

	template <size_t idx>
	void _add_Method_Def(PyFunc pFn, std::string methodName, int methodFlags, std::string docs)
	{
		ExposedFunctions.push_back(pFn);

		// now make the function pointer (TODO figure out these ids)
		PyCFunction fnPtr = get_fn_ptr<idx>(ExposedFunctions.back());

		// You can key the methodName string to a std::function
		MethodDef.AddMethod(methodName, fnPtr, methodFlags, docs);// , doc.empty() ? NULL : doc.c_str() );
	}

	// Case 1: a straight up function that would look like : R fn( Args... ) { ... return R(); }
	template <size_t idx, typename R, typename ... Args>
	static void _add_Func(std::string methodName, std::function<R(Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			PyObject * ret = nullptr;

			std::tuple<Args...> tup;
			convert(a, tup);
			R rVal = call<R>(fn, tup);

			return alloc_pyobject(rVal);
		};

		_add_Method_Def<idx>(pFn, methodName, methodFlags, docs);
	}

	// Case 2: like above, but void return : void fn( Args... ) { ... return; }
	template <size_t idx, typename ... Args>
	static void _add_Func(std::string methodName, std::function<void(Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			std::tuple<Args...> tup;
			convert(a, tup);
			call<void>(fn, tup);

			Py_INCREF(Py_None);
			return Py_None;
		};

		_add_Method_Def<idx>(pFn, methodName, methodFlags, docs);
	}

	// Case 3: returns R, no args : R fn() { ... return R(); }
	template <size_t idx, typename R>
	static void _add_Func(std::string methodName, std::function<R()> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			PyObject * ret = nullptr;

			R rVal = fn();

			return alloc_pyobject(R);
		};

		_add_Method_Def<idx>(pFn, methodName, methodFlags, docs);
	}

	// Case 4: returns void, no args : void fn() { ... hello world; }
	template <size_t idx>
	static void _add_Func(std::string methodName, std::function<void()> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			fn();

			Py_INCREF(Py_None);
			return Py_None;
		};

		_add_Method_Def<idx>(pFn, methodName, methodFlags, docs);
	}

	// Now the same as above, except for class member functions
	// these functions generally call into their counterparts above, but the
	// function definitions have to be appended to their corresponding class definition

	template <class C>
	std::string _getClassFunctionDef(std::string methodName, size_t numArgs = 0)
	{
		auto classIt = ExposedClasses.find(typeid(C));
		if (classIt == ExposedClasses.end())
			return "Error creating class!";

		std::string pyModMethodName = classIt->second.pyname + "_" + methodName;

		std::string pyArgs;
		const char diff = char(0x7A - 0x61); //'a' => 'z'
		for (int i = 0; i < numArgs - 1; i++)
		{
			std::string prePend;
			for (int p = 0; p < (i / diff); p++)
				prePend.append("_");
			char id = (char(0x61) + char(i % diff));
			pyArgs.append(prePend);
			pyArgs += id;
			if (i + 2 < numArgs)
				pyArgs.append(", ");
		}

		// I honestly can't explain most of these tabs. You'll need a good way if indenting
		std::string fnDef;
		fnDef += getTabs(1) + "def " + methodName + "( self, " + pyArgs + "):\n";
		fnDef += getTabs(2) + "return " + ModuleName + "." + pyModMethodName + "(self._self, " + pyArgs + ")\n";

		std::cout << fnDef << std::endl;

		classIt->second.classDef.append(fnDef);

		return pyModMethodName;
	}

	// Case 1
	template <size_t idx, class C, typename R, typename ... Args>
	static void _add_Func(std::string methodName, std::function<R(Args...)> fn, int methodFlags, std::string docs = "")
	{
		std::string pyModMethodName = _getClassFunctionDef<C>(methodName, sizeof...(Args));
		Python::_add_Func<idx>(pyModMethodName, fn, methodFlags, docs);
	}

	// Case 2
	template <size_t idx, class C, typename ... Args>
	static void _add_Func(std::string methodName, std::function<void(Args...)> fn, int methodFlags, std::string docs = "")
	{
		std::string pyModMethodName = _getClassFunctionDef<C>(methodName, sizeof...(Args));
		Python::_add_Func<idx>(pyModMethodName, fn, methodFlags, docs);
	}

	// Case 3
	template <size_t idx, class C, typename R>
	static void _add_Func(std::string methodName, std::function<R()> fn, int methodFlags, std::string docs = "")
	{
		std::string pyModMethodName = _getClassFunctionDef<C>(methodName);
		Python::_add_Func<idx>(pyModMethodName, fn, methodFlags, docs);
	}

	// Case 4
	template <size_t idx, class C>
	static void _add_Func(std::string methodName, std::function<void()> fn, int methodFlags, std::string docs = "")
	{
		std::string pyModMethodName = _getClassFunctionDef<C>(methodName);
		Python::_add_Func<idx>(pyModMethodName, fn, methodFlags, docs);
	}


	// This function generates a python class definition
	template <class C>
	static void Register_Class(std::string className) {
		if (ExposedClasses.find(typeid(C)) != ExposedClasses.end())
			return;

		// I believe four spaces are preferred to \t...
		std::string classDef;
		classDef += "class " + className + ":\n";

		classDef += getTabs(1) + "def __init__(self, c_ptr): \n";
		classDef += getTabs(2) + "self._self = c_ptr \n";

		classDef += getTabs(1) + "def __call__(self): \n";
		classDef += getTabs(2) + "return self._self \n";

		ExposedClasses[std::type_index(typeid(C))] = ExposedClass(className, classDef, std::vector<PyObject *>());
	}

	// This will expose a specific C++ object instance as a Python
	// object, giving it a pointer to the original object (which better stay live)
	template <class C>
	static void Expose_Object(C * instance, std::string name) {
		// Make sure it's a vlid pointer
		if (!instance)
			return;

		// If we haven't declared the class, we can't expose it
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		// Make a PyCObject from the void * to the instance
		PyObject* newPyObject = PyCObject_FromVoidPtr((void *)instance, nullptr);

		// Make a dummy variable, assign it to the ptr
		PyObject * module = PyImport_ImportModule("__main__");
		PyRun_SimpleString("ecsPtr = 0");
		PyObject_SetAttrString(module, "ecsPtr", newPyObject);

		// Construct the python class
		std::string pythonCall;
		pythonCall.append(name).append(" = ").append(it->second.pyname).append("(ecsPtr)");
		PyRun_SimpleString(pythonCall.c_str());

		// Get rid of the dummy var
		PyRun_SimpleString("del ecsPtr");

		// decref and return
		Py_DECREF(module);
		Py_DECREF(newPyObject);

		// Why does it need to be a pyObject?
		PyObject * pyObject = static_cast<PyObject *>((void *)instance);
		it->second.instances.push_back(pyObject);
	}
}