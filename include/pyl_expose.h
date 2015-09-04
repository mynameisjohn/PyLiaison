#pragma once

#include <iostream>
#include <typeindex>
#include <string>

#include <Python.h>

#include "pyl_misc.h"
#include "pyl_classes.h"

// I don't know how to declare all these without
// explicity specializing the templates... Unforunate

// TODO make a member function equivalent of this
#define Py_Add_Func(name, fn, docs)\
	Python::_add_Func<__LINE__>(std::string(name), Python::make_function(fn), METH_VARARGS, docs)

namespace Python
{
	// The idea is to make a module where all your functions live
	const std::string ModuleName = "PyLiaison";

	// Externs
	extern std::map<std::type_index, ExposedClass> ExposedClasses;
	extern std::string ClassesDef; // Do I still need this?
	extern std::list<PyFunc> ExposedFunctions;
	extern MethodDefinitions MethodDef;
	extern PyObject * Py_ErrorObj;

	// These are the black sheep for now
	extern PyModuleDef ModDef;
	extern std::string ModDocs;
   
	// Add a new method def fo the Method Definitions of the module
	template <size_t idx>
	void _add_Method_Def(PyFunc pFn, std::string methodName, int methodFlags, std::string docs)
	{
        // We need to store these where they won't move
		ExposedFunctions.push_back(pFn);

		// now make the function pointer (TODO figure out these ids, or do something else)
		PyCFunction fnPtr = get_fn_ptr<idx>(ExposedFunctions.back());

		// You can key the methodName string to a std::function
		MethodDef.AddMethod(methodName, fnPtr, methodFlags, docs);// , doc.empty() ? NULL : doc.c_str() );
	}

	template<class C>
	void _add_Member(std::string name, size_t offset, int flags, std::string docs) {
		const int type = T_INT; // What do I do for other types? Make a map?
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		it->second.AddMember(name, type, offset, flags, doc);
	}

	template <size_t idx, class C>
	void _add_Mem_Fn_Def(std::string methodName, PyFunc pFn, int methodFlags, std::string docs)
	{
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		// We need to store these where they won't move
		ExposedFunctions.push_back(pFn);

		// now make the function pointer (TODO figure out these ids, or do something else)
		PyCFunction fnPtr = get_fn_ptr<idx>(ExposedFunctions.back());

		// Add function
		it->second.AddMemberFn(methodName, fnPtr, methodFlags,  docs);

		// You can key the methodName string to a std::function
		//MethodDef.AddMethod(methodName, fnPtr, methodFlags, docs);// , doc.empty() ? NULL : doc.c_str() );
	}

	// Case 1: a straight up function that would look like : R fn( Args... ) { ... return R(); }
	template <size_t idx, typename R, typename ... Args>
	static void _add_Func(std::string methodName, std::function<R(Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
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
			R rVal = fn();

			return alloc_pyobject(rVal);
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

	// If I add these to the module, none of this is necessary
	//template <class C>
	//std::string _getClassFunctionDef(std::string methodName, size_t numArgs = 0)
	//{
 //       // Only expose member functions for familiar classes
	//	auto classIt = ExposedClasses.find(typeid(C));
	//	if (classIt == ExposedClasses.end())
	//		return "Error creating class!";

 //       // Foo::doSomething() becomes Foo_doSomething()
	//	std::string pyModMethodName = classIt->second.PyClassName + "_" + methodName;

 //       // We need to give the fn definition the correct number of arguments
	//	std::string pyArgs;
	//	const char diff = char(0x7A - 0x61); //'a' => 'z'
	//	for (int i = 0; i < numArgs - 1; i++)
	//	{
 //           // preprend a '_' for every time we've looped a->z
	//		std::string prePend;
	//		for (int p = 0; p < (i / diff); p++)
	//			prePend.append("_");
 //           
 //           // 'a' + i % ('z' - 'a')
	//		char id = (char(0x61) + char(i % diff));
	//		pyArgs.append(prePend);
	//		pyArgs += id;
 //           
 //           // Don't put a comma on the last one
	//		if (i + 2 < numArgs)
	//			pyArgs.append(", ");
	//	}

 //       // The actual function definition
	//	//std::string fnDef;
	//	//fnDef += getTabs(1) + "def " + methodName + "( self, " + pyArgs + "):\n";
	//	//fnDef += getTabs(2) + "return " + pyModMethodName + "(self._self, " + pyArgs + ")\n";

	//	//classIt->second.ClassDef.append(fnDef);

	//	return pyModMethodName;
	//}

	// Case 1
	template <size_t idx, class C, typename R, typename ... Args>
	static void _add_Func(std::string methodName, std::function<R(Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			std::tuple<Args...> tup;
			convert(a, tup);
			R rVal = call<R>(fn, tup);

			return alloc_pyobject(rVal);
		};
		//std::string pyModMethodName = _getClassFunctionDef<C>(methodName, sizeof...(Args));
		Python::_add_Mem_Fn_Def<idx, C>(methodName, pFn, methodFlags, docs);
	}

	// Case 2
	template <size_t idx, class C, typename ... Args>
	static void _add_Func(std::string methodName, std::function<void(Args...)> fn, int methodFlags, std::string docs = "")
	{
		//std::string pyModMethodName = _getClassFunctionDef<C>(methodName, sizeof...(Args));
		Python::_add_Mem_Fn_Def<idx, C>(methodName, fn, methodFlags, docs);
	}

	// Case 3
	template <size_t idx, class C, typename R>
	static void _add_Func(std::string methodName, std::function<R()> fn, int methodFlags, std::string docs = "")
	{
		//std::string pyModMethodName = _getClassFunctionDef<C>(methodName, sizeof...(Args));
		Python::_add_Mem_Fn_Def<idx, C>(methodName, fn, methodFlags, docs);
	}

	// Case 4
	template <size_t idx, class C>
	static void _add_Func(std::string methodName, std::function<void()> fn, int methodFlags, std::string docs = "")
	{
		//std::string pyModMethodName = _getClassFunctionDef<C>(methodName, sizeof...(Args));
		Python::_add_Mem_Fn_Def<idx, C>(methodName, fn, methodFlags, docs);
	}
	
	// This function generates a python class definition
	template <class C, size_t idx>
	static void Register_Class(std::string className) {
		if (ExposedClasses.find(typeid(C)) != ExposedClasses.end())
			return;

		ExposedClasses[typeid(C)] = ExposedClass(className);

      // The actual class definition, with constructor and () overload
		//std::string classDef;
		//classDef += "class " + className + ":\n";

		//classDef += getTabs(1) + "def __init__(self, c_ptr): \n";
		//classDef += getTabs(2) + "self._self = c_ptr \n";

		//classDef += getTabs(1) + "def __call__(self): \n";
		//classDef += getTabs(2) + "return self._self \n";
		// By default you gotta make a constructor and a __call__ function... how?

		PyTypeObject obj = { 0 };
		
		// We've got to get the void c ptr out of args and 
		// store it in some member of self... so what is self?
		// is it pointing to an instance of C? Not on my watch...
		
		// This is literally the constructor (literally)
		ExposedClasses[typeid(C)].m_Init = [](PyObject * self, PyObject * args, PyObject * kwds) {
			// In the example the first arg isn't a PyObject *, but... idk man
			MemberDefinitions::Pointer * bsPtr = static_cast<MemberDefinitions::Pointer *>((void *)self);
			// The first argument is the capsule object
			PyObject * c = PyTuple_GetItem(args, 0);
			// Or at least it better be
			if (c && PyCapsule_CheckExact(c)) 
			{
				// If we got it, set the actual object pointer
				PyObject * tmp = bsPtr->capsule;
				Py_INCREF(c);
				bsPtr->capsule = c;
				Py_XDECREF(tmp);
			}
			//else ?

			// TODO what about other members?
			
			return 0;
		};

		ExposedClasses[typeid(C)].m_TypeObject.tp_init = get_fn_ptr<idx>(ExposedClasses[typeid(C)].m_Init);
	}

	// This will expose a specific C++ object instance as a Python
	// object, giving it a pointer to the original object (which better stay live)
    // TODO
    // Make it so you can expose object in any module, not just main
    // I wouldn't worry about resource management on the python end; if you really want something
    // to go out of scope, limit it to a particular module; presumable destroying that module
    // destroys the object. Assume C++ objects will stay live forever, or at least for the
    // lifetime of the module
	template <class C>
	static void Expose_Object(C * instance, std::string name) {
		// Make sure it's a valid pointer
		if (!instance)
			return;

		// If we haven't declared the class, we can't expose it
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;


		// Right now I don't know why we should keep them
		// in the future I may have a virtual C++ object
		// whose destructor calls del on the pyobject
		voidptr_t obj = static_cast<voidptr_t>(instance);
		it->second.Instances.push_back({ obj, name });
		ExposedClass::Instance& instRef = it->second.Instances.back();

		// Make a PyCObject from the void * to the instance (I'd give it a name, but why?
		PyObject* newPyObject = PyCapsule_New(instRef.c_ptr, NULL, NULL); // instRef.pyname.c_str());

		// Make a dummy variable, assign it to the ptr
		PyObject * module = PyImport_ImportModule("__main__");
		PyRun_SimpleString("c_ptr = 0");
		PyObject_SetAttrString(module, "c_ptr", newPyObject);

		// Construct the python class
		std::string pythonCall;
		pythonCall.append(name).append(" = ").append(it->second.PyClassName).append("(c_ptr)");
		PyRun_SimpleString(pythonCall.c_str());

		// Get rid of the dummy var
		PyRun_SimpleString("del c_ptr");

		// decref and return
		Py_DECREF(module);
		Py_DECREF(newPyObject);
	}
}