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
	Python::Register_Function<__LINE__>(std::string(name), Python::make_function(fn), docs)


namespace Python
{
	// Pretty ridiculous
	template <typename C>
	static C * _getCapsulePtr(PyObject * obj)
	{
		assert(obj);
		auto gpcPtr = static_cast<GenericPyClass *>((voidptr_t)obj);
		assert(gpcPtr);
		PyObject * capsule = gpcPtr->capsule;
		assert(PyCapsule_CheckExact(capsule));
		return static_cast<C *>(PyCapsule_GetPointer(capsule, NULL));
	}


	template <typename R, typename ... Args>
	PyFunc GetPyFunc_Case1(std::function<R(Args...)> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			std::tuple<Args...> tup;
			convert(a, tup);
			R rVal = invoke(fn, tup);

			return alloc_pyobject(rVal);
		};
		return pFn;
	}

	template <typename ... Args>
	PyFunc GetPyFunc_Case2(std::function<void(Args...)> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			std::tuple<Args...> tup;
			convert(a, tup);
			invoke(fn, tup);

			Py_INCREF(Py_None);
			return Py_None;
		};
		return pFn;
	}

	template <typename R>
	PyFunc GetPyFunc_Case3(std::function<R()> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a)
		{
			R rVal = fn();
			return alloc_pyobject(rVal);
		};
		return pFn;
	}

	PyFunc GetPyFunc_Case4(std::function<void()> fn);


	template <typename C, typename R, typename ... Args>
	PyFunc GetPyFunc_Mem_Case1(std::function<R(Args...)> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			// the first arg is the instance pointer, contained in s
			std::tuple<Args...> tup;
			std::get<0>(tup) = _getCapsulePtr<C>(s);

			// recurse till the first element, getting args from a
			add_to_tuple<sizeof...(Args)-1, 1, Args...>(a, tup);

			// Invoke function, get retVal	
			R rVal = invoke(fn, tup);

			// convert rVal to PyObject, return
			return alloc_pyobject(rVal);
		};
		return pFn;
	}

	template <typename C, typename ... Args>
	PyFunc GetPyFunc_Mem_Case2(std::function<void(Args...)> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			// the first arg is the instance pointer, contained in s
			std::tuple<Args...> tup;
			std::get<0>(tup) = _getCapsulePtr<C>(s);

			// recurse till the first element, getting args from a
			add_to_tuple<sizeof...(Args)-1, 1, Args...>(a, tup);

			// invoke function
			invoke(fn, tup);

			// Return None
			Py_INCREF(Py_None);
			return Py_None;
		};
		return pFn;
	}

	template <typename C, typename R>
	PyFunc GetPyFunc_Mem_Case3(std::function<R(C *)> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			// Nothing special here
			R rVal = fn(_getCapsulePtr<C>(s));

			return alloc_pyobject(rVal);
		};
		return pFn;
	}
	
	template<typename C>
	PyFunc GetPyFunc_Mem_Case4(std::function<void(C *)> fn) {
		PyFunc pFn = [fn](PyObject * s, PyObject * a) {
			// Nothing special here
			fn(_getCapsulePtr<C>(s));

			Py_INCREF(Py_None);
			return Py_None;
		};
		return pFn;
	}

	class Module {// : public Object {
	private:
		std::map<std::type_index, ExposedClass> m_mapExposedClasses;
		std::list<PyFunc> m_liExposedFunctions;
		MethodDefinitions m_vMethodDef;
		PyModuleDef m_pyModDef;
		std::string m_strModDocs;
		std::string m_strModName;
		std::function<PyObject *()> m_fnModInit;


		//};

		//// The idea is to make a module where all your functions live
		//const std::string ModuleName = "PyLiaison";

		//// Externs
		//extern std::map<std::type_index, ExposedClass> ExposedClasses;
		//extern std::string ClassesDef; // Do I still need this?
		//extern std::list<PyFunc> ExposedFunctions;
		//extern MethodDefinitions MethodDef;
		//extern PyObject * Py_ErrorObj;

		//// These don't really need encapsulation
		//extern PyModuleDef ModDef;
		//extern std::string ModDocs;

		// Add a new method def fo the Method Definitions of the module
		template <typename tag>
		void _add_Method_Def(PyFunc pFn, std::string methodName, int methodFlags, std::string docs)
		{
			// We need to store these where they won't move
			m_liExposedFunctions.push_back(pFn);

			// now make the function pointer (TODO figure out these ids, or do something else)
			PyCFunction fnPtr = get_fn_ptr<tag>(m_liExposedFunctions.back());

			// You can key the methodName string to a std::function
			m_vMethodDef.AddMethod(methodName, fnPtr, methodFlags, docs);// , doc.empty() ? NULL : doc.c_str() );
		}

		// This isn't really being used at the moment, since the intention is unclear
		template<class C>
		void _add_Member(std::string name, size_t offset, int flags, std::string docs) {
			const int type = T_INT; // What do I do for other types? Make a map?
			auto it = m_mapExposedClasses.find(typeid(C));
			if (it == m_mapExposedClasses.end())
				return;

			it->second.AddMember(name, type, offset, flags, docs);
		}

		template <typename tag, class C>
		void _add_Mem_Fn_Def(std::string methodName, PyFunc pFn, int methodFlags, std::string docs)
		{
			auto it = m_mapExposedClasses.find(typeid(C));
			if (it == m_mapExposedClasses.end())
				return;

			// We need to store these where they won't move
			m_liExposedFunctions.push_back(pFn);

			// now make the function pointer (TODO figure out these ids, or do something else)
			PyCFunction fnPtr = get_fn_ptr<tag>(m_liExposedFunctions.back());

			// Add function
			it->second.AddMemberFn(methodName, fnPtr, methodFlags, docs);
		}

	public:
		Module();
		Module(std::string modName, std::string modDocs);


		// Case 1: a straight up function that would look like : R fn( Args... ) { ... return R(); }
		template <typename tag, typename R, typename ... Args>
		void Register_Function(std::string methodName, std::function<R(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Case1(fn);

			_add_Method_Def<tag>(pFn, methodName, METH_VARARGS, docs);
		}

		// Case 2: like above, but void return : void fn( Args... ) { ... return; }
		template <typename tag, typename ... Args>
		void Register_Function(std::string methodName, std::function<void(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Case2(fn);

			_add_Method_Def<tag>(pFn, methodName, METH_VARARGS, docs);
		}

		// Case 3: returns R, no args : R fn() { ... return R(); }
		template <typename tag, typename R>
		void Register_Function(std::string methodName, std::function<R()> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Case3();

			_add_Method_Def<tag>(pFn, methodName, METH_NOARGS, docs);
		}

		// Case 4: returns void, no args : void fn() { ... hello world; }
		template <typename tag>
		void Register_Function(std::string methodName, std::function<void()> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Case4();

			_add_Method_Def<tag>(pFn, methodName, METH_NOARGS, docs);
		}



		// Case 1
		template <typename C, typename tag, typename R, typename ... Args,
			typename std::enable_if<sizeof...(Args) != 1, int>::type = 0>
			void Register_Mem_Function(std::string methodName, std::function<R(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Mem_Case1<C>(fn);
			_add_Mem_Fn_Def<tag, C>(methodName, pFn, METH_VARARGS, docs);
		}

		// Case 2
		template <typename C, typename tag, typename ... Args>
		void Register_Mem_Function(std::string methodName, std::function<void(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Mem_Case2<C>(fn);
			_add_Mem_Fn_Def<tag, C>(methodName, pFn, METH_VARARGS, docs);
		}

		// Case 3
		template <typename C, typename tag, typename R>
		void Register_Mem_Function(std::string methodName, std::function<R(C *)> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Mem_Case3<C>(fn);
			_add_Mem_Fn_Def<tag, C>(methodName, pFn, METH_NOARGS, docs);
		}

		// Case 4
		template <typename C, typename tag>
		void Register_Mem_Function(std::string methodName, std::function<void(C *)> fn, std::string docs = "")
		{
			PyFunc pFn = GetPyFunc_Mem_Case4<C>(fn);
			_add_Mem_Fn_Def<tag, C>(methodName, pFn, METH_NOARGS, docs);
		}

		// This function generates a python class definition
		template <class C>
		void Register_Class(std::string className) {
			auto it = m_mapExposedClasses.find(typeid(C));
			if (it != m_mapExposedClasses.end())
				return;
			m_mapExposedClasses.emplace(typeid(C), className);
		}

		int _ExposeObjectImpl(voidptr_t instance, ExposedClass& expCls, const std::string& name, PyObject * mod);

		// This will expose a specific C++ object instance as a Python
		// object, giving it a pointer to the original object (which better stay live)
		// TODO this could leak if it fails; also classes should have a destructor
		template <class C>
		void Expose_Object(C * instance, const std::string& name, PyObject * mod = nullptr) {
			// Make sure it's a valid pointer
			if (!instance)
				return;

			// If we haven't declared the class, we can't expose it
			auto it = m_mapExposedClasses.find(typeid(C));
			if (it == m_mapExposedClasses.end())
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

			int success = _ExposeObjectImpl(obj, it->second, name, mod);
		}

		// make private
		void createFnObject();

		// Does this reference invalidate if reassigned?
		template <typename tag>
		PyObject *(*getFnPtr())() {
			createFnObject();
			return get_fn_ptr<tag>(m_fnModInit);
		}

		// Useful gets
		const char * GetNameBuf() const;
		std::string GetNameStr() const;
		Object AsObject() const;

		// Lock down references
		void Init();

		
	};

	// Module Storage, non reference invalidating
	extern std::map<std::string, Module> __g_MapPyModules;

	// Add module to above, return pointer to new module
	template <typename tag>
	Module * AddModule(std::string modName, std::string modDocs = "No docs defined") {
		// If we've already added this, return a pointer to it
		auto it = __g_MapPyModules.find(modName);
		if (it != __g_MapPyModules.end())
			return &it->second;

		// Add to map
		Module& mod = __g_MapPyModules[modName] = Module(modName, modDocs);

		// Make sure the init function gets called when imported (is fn ptr safe?)
		int success = PyImport_AppendInittab(mod.GetNameBuf(), mod.getFnPtr<tag>());
		return &mod;
	}

	// Actually increment the module's ref count
	// and return the PyObject in order to
	// call functions and assign members
	Object GetModuleObj(std::string modName);

	// Just get a pointer to the stored module
	// use this for adding functions and members
	Module * GetModuleHandle(std::string modName);
}
