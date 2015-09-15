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
	Python::Register_Function<__LINE__>(std::string(name), Python::make_function(fn), METH_VARARGS, docs)

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

	// These don't really need encapsulation
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

    // This isn't really being used at the moment, since the intention is unclear
	template<class C>
	void _add_Member(std::string name, size_t offset, int flags, std::string docs) {
		const int type = T_INT; // What do I do for other types? Make a map?
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		it->second.AddMember(name, type, offset, flags, docs);
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
	}

	// Case 1: a straight up function that would look like : R fn( Args... ) { ... return R(); }
	template <size_t idx, typename R, typename ... Args>
	static void Register_Function(std::string methodName, std::function<R(Args...)> fn, int methodFlags, std::string docs = "")
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
	static void Register_Function(std::string methodName, std::function<void(Args...)> fn, int methodFlags, std::string docs = "")
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
	static void Register_Function(std::string methodName, std::function<R()> fn, int methodFlags, std::string docs = "")
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

	// Pretty ridiculous
	template <typename C>
	C * _getCapsulePtr(PyObject * obj) 
	{
		assert(obj);
		auto gpcPtr = static_cast<GenericPyClass *>((voidptr_t)obj);
		assert(gpcPtr);
		PyObject * capsule = gpcPtr->capsule;
		assert(PyCapsule_CheckExact(capsule));
		return static_cast<C *>(PyCapsule_GetPointer(capsule, NULL));
	}

	// Case 1
	template <size_t idx, class C, typename R, typename ... Args>
	static void Register_Function(std::string methodName, std::function<R(C *, Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			std::tuple<Args...> tup;
			convert(a, tup); // Can I prepend the pointer to this tuple?
			R rVal = call_C<C, R>(fn, _getCapsulePtr<C>(s), tup);

			return alloc_pyobject(rVal);
		};
		Python::_add_Mem_Fn_Def<idx, C>(methodName, pFn, methodFlags, docs);
	}

	// Case 2
	template <size_t idx, class C, typename ... Args>
	static void Register_Function(std::string methodName, std::function<void(C *, Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			std::tuple<Args...> tup;
			convert(a, tup); // Can I prepend the pointer to this tuple?
			callv_C<C>(fn, _getCapsulePtr<C>(s), tup);

			Py_INCREF(Py_None);
			return Py_None;
		};
		Python::_add_Mem_Fn_Def<idx, C>(methodName, pFn, methodFlags, docs);
	}

	// Case 3
	template <size_t idx, class C, typename R>
	static void Register_Function(std::string methodName, std::function<R(C *)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
            R rVal = fn(_getCapsulePtr<C>(s));

			return alloc_pyobject(rVal);
		};
		Python::_add_Mem_Fn_Def<idx, C>(methodName, pFn, methodFlags, docs);
	}

	// Case 4
	template <size_t idx, class C>
	static void Register_Function(std::string methodName, std::function<void(C *)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
            fn(_getCapsulePtr<C>(s));

			Py_INCREF(Py_None);
			return Py_None;
		};
		Python::_add_Mem_Fn_Def<idx, C>(methodName, pFn, methodFlags, docs);
	}
	
	// This function generates a python class definition
	template <class C, size_t idx>
	static void Register_Class(std::string className) {
		auto it = ExposedClasses.find(typeid(C));
		if (it != ExposedClasses.end())
			return;
		ExposedClasses.emplace(typeid(C), className);
	}

	// This will expose a specific C++ object instance as a Python
	// object, giving it a pointer to the original object (which better stay live)
	// TODO this could leak if it fails; also classes should have a destructor
	template <class C>
	static void Expose_Object(C * instance, const std::string& name, PyObject * mod = nullptr) {
		// Make sure it's a valid pointer
		if (!instance)
			return;

		// If we haven't declared the class, we can't expose it
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		// If a module wasn't specified, just do main
		if (!mod) {
			// If there isn't a specific module,
			// then we can either add it to __main__
			// or to the PyLiaison module
			mod = PyImport_ImportModule("__main__");
		}

		// Make a void pointer out of it
		voidptr_t obj = static_cast<voidptr_t>(instance);

		// Get the type object pointer
		PyTypeObject * tObjPtr = &it->second.m_TypeObject;

		// Allocate a new object instance given the PyTypeObject
		PyObject* newPyObject = _PyObject_New(tObjPtr);

		// Make a PyCapsule from the void * to the instance (I'd give it a name, but why?
		PyObject* capsule = PyCapsule_New(obj, NULL, NULL);

		// Set the c_ptr member variable (which better exist) to the capsule
        static_cast<GenericPyClass *>((voidptr_t)newPyObject)->capsule = capsule;

		// Make a variable in the module out of the new py object
		int success = PyObject_SetAttrString(mod, name.c_str(), newPyObject);

		// Right now I don't know why we should keep them
		it->second.Instances.push_back({ obj, name });

		// decref and return
		Py_DECREF(mod);
		Py_DECREF(newPyObject);
	}
}