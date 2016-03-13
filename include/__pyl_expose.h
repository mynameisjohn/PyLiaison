#pragma once

#include <iostream>
#include <typeindex>
#include <string>

#include <Python.h>

#include "pyl_misc.h"
#include "pyl_funcs.h"

// I don't know how to declare all these without
// explicity specializing the templates... Unforunate

// TODO make a member function equivalent of this
#define Py_Add_Func(name, fn, docs)\
	Python::Register_Function<__LINE__>(std::string(name), Python::make_function(fn), docs)


namespace pyl
{



	class ModuleDef
	{
	private:
		ModuleDef( std::string modName, std::string modDocs );
		static std::map<std::string, ModuleDef> s_MapPyModules;
		static ModuleDef * getModule( std::string& modName );
	private:

		std::map<std::type_index, ExposedClass> m_mapExposedClasses;
		std::list<PyFunc> m_liExposedFunctions;
		MethodDefinitions m_vMethodDef;
		PyModuleDef m_pyModDef;
		std::string m_strModDocs;
		std::string m_strModName;
		std::function<PyObject *()> m_fnModInit;

		// Add a new method def fo the Method Definitions of the module
		template <typename tag>
		void addFunction(PyFunc pFn, std::string methodName, int methodFlags, std::string docs)
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
		void add_Member(std::string name, size_t offset, int flags, std::string docs) {
			const int type = T_INT; // What do I do for other types? Make a map?
			auto it = m_mapExposedClasses.find(typeid(C));
			if (it == m_mapExposedClasses.end())
				return;

			it->second.AddMember(name, type, offset, flags, docs);
		}

		template <typename tag, class C>
		void addMemFunction(std::string methodName, PyFunc pFn, int methodFlags, std::string docs)
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

		// Creates init function where it won't get invalidated
		void createFnObject();

		// Implementation of expose object function
		int exposeObject_impl( type_info T, voidptr_t instance, ExposedClass& expCls, const std::string& name, PyObject * mod);

		void registerClass_impl( type_info T, std::string& className );

	public:
		// Case 1: a straight up function that would look like : R fn( Args... ) { ... return R(); }
		template <typename tag, typename R, typename ... Args>
		void Register_Function(std::string methodName, std::function<R(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Case1(fn);

			addFunction<tag>(pFn, methodName, METH_VARARGS, docs);
		}

		// Case 2: like above, but void return : void fn( Args... ) { ... return; }
		template <typename tag, typename ... Args>
		void Register_Function(std::string methodName, std::function<void(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Case2(fn);

			addFunction<tag>(pFn, methodName, METH_VARARGS, docs);
		}

		// Case 3: returns R, no args : R fn() { ... return R(); }
		template <typename tag, typename R>
		void Register_Function(std::string methodName, std::function<R()> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Case3(fn);

			addFunction<tag>(pFn, methodName, METH_NOARGS, docs);
		}

		// Case 4: returns void, no args : void fn() { ... hello world; }
		template <typename tag>
		void Register_Function(std::string methodName, std::function<void()> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Case4(fn);

			addFunction<tag>(pFn, methodName, METH_NOARGS, docs);
		}

		// Case 1
		template <typename C, typename tag, typename R, typename ... Args,
			typename std::enable_if<sizeof...(Args) != 1, int>::type = 0>
			void Register_Mem_Function(std::string methodName, std::function<R(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Mem_Case1<C>(fn);
			addMemFunction<tag, C>(methodName, pFn, METH_VARARGS, docs);
		}

		// Case 2
		template <typename C, typename tag, typename ... Args>
		void Register_Mem_Function(std::string methodName, std::function<void(Args...)> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Mem_Case2<C>(fn);
			addMemFunction<tag, C>(methodName, pFn, METH_VARARGS, docs);
		}

		// Case 3
		template <typename C, typename tag, typename R, typename Callable>
		void Register_Mem_Function(std::string methodName, std::function<R(C *)> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Mem_Case3<C>(fn);
			addMemFunction<tag, C>(methodName, pFn, METH_NOARGS, docs);
		}

		// Case 4
		template <typename C, typename tag>
		void Register_Mem_Function(std::string methodName, std::function<void(C *)> fn, std::string docs = "")
		{
			PyFunc pFn = __getPyFunc_Mem_Case4<C>(fn);
			addMemFunction<tag, C>(methodName, pFn, METH_NOARGS, docs);
		}

		// This function generates a python class definition
		template <class C>
		void Register_Class(std::string className) {
			registerClass_impl( typeid(C) );
		}

		// This will expose a specific C++ object instance as a Python
		// object, giving it a pointer to the original object (which better stay live)
		// TODO this could leak if it fails; also classes should have a destructor
		template <class C>
		int Expose_Object(C * instance, const std::string& name, PyObject * mod = nullptr) {
			// Make sure it's a valid pointer
			if (!instance)
				return -1;

			return exposeObject_impl( typeid(C), static_cast<voidptr_t>(instance), it->second, name, mod );
		}

		// Does this reference invalidate if reassigned?
		template <typename tag>
		PyObject *(*__getFnPtr())() {
			createFnObject();
			return get_fn_ptr<tag>(m_fnModInit);
		}

		// Useful gets
		const char * __getNameBuf() const;
		std::string __getNameStr() const;
		Object AsObject() const;

		// Lock down references
		void __init();

		template <typename tag>
		static ModuleDef * Create( std::string modName, std::string modDocs )
		{
			if ( ModuleDef::getModule( modName ) == nullptr )
		}
	};

	// Module Storage, non reference invalidating
	extern std::map<std::string, ModuleDef> __g_MapPyModules;

	ModuleDef * __addModule_impl( std::string& modName, std::string& modDocs )

	// Add module to above, return pointer to new module
	template <typename tag>
	ModuleDef * AddModule(std::string modName, std::string modDocs = "No docs defined") {
		

		// Add to map
		ModuleDef& mod = __g_MapPyModules[modName] = ModuleDef(modName, modDocs);

		ModuleDef * pMod = __addModule_impl( modName, modDocs );

		// Make sure the init function gets called when imported (is fn ptr safe?)
		int success = PyImport_AppendInittab(mod.__getNameBuf(), mod.__getFnPtr<tag>());
		return &mod;
	}

	// Actually increment the module's ref count
	// and return the PyObject in order to
	// call functions and assign members
	Object GetModuleObj(std::string modName);

	// Just get a pointer to the stored module
	// use this for adding functions and members
	ModuleDef * GetModuleHandle(std::string modName);
}
