#pragma once


#include <memory>

#include <Python.h>
#include <structmember.h>

#include "pyl_Convert.h"

namespace Python
{
	// Every python module function looks like this
	using PyFunc = std::function<PyObject *(PyObject *, PyObject *)>;

	// Class __init__
	// PyClsInitFunc = std::function<int(PyObject *, PyObject *, PyObject *)>;

	// () operator (__call__)
	using PyClsCallFunc = std::function<PyObject *(PyObject *, PyObject *, PyObject *)>;

	// We only need one instance of the above, shared by exposed objects
	//extern PyClsInitFunc PyClsInit;
	extern PyClsCallFunc PyClsCall;


	// Deleter that calls Py_XDECREF on the PyObject parameter.
	struct PyObjectDeleter {
		void operator()(PyObject *obj) {
			Py_XDECREF(obj);
		}
	};
	// unique_ptr that uses Py_XDECREF as the destructor function.
	typedef std::unique_ptr<PyObject, PyObjectDeleter> pyunique_ptr;

	// This feels gross
	using voidptr_t = void *;

	// We need to keep the method definition's
	// string name and docs stored somewhere,
	// where their references are good, since they're char *
	// Also note! std::vectors can move in memory,
	// so when once this is exposed to python it shouldn't
	// be modified

	// TODO template this
	struct MethodDefinitions
	{
		// Method defs must be contiguous
		std::vector<PyMethodDef> v_Defs;

		// These containers don't invalidate references
		std::list<std::string> MethodNames, MethodDocs;

		// By default add the "null terminator",
		// all other methods are inserted before it
		MethodDefinitions() :
			v_Defs(1, { 0 })
		{}

		// Add method definitions before the null terminator
		size_t AddMethod(std::string name, PyCFunction fnPtr, int flags, std::string docs = "");

		// pointer to the definitions (which can move!)
		PyMethodDef * ptr() { return v_Defs.data(); }
	};

	struct GenericPyClass
	{
		PyObject * capsule{ nullptr };
	};

	struct MemberDefinitions
	{
		// Method defs must be contiguous
		std::vector<PyMemberDef> v_Defs;

		// These containers don't invalidate references
		std::list<std::string> MemberNames, MemberDocs;

		// By default add the "null terminator",
		// all other methods are inserted before it
		MemberDefinitions();

		// Add method definitions before the null terminator
		size_t AddMember(std::string name, int type, int offset, int flags, std::string docs = "");

		// pointer to the definitions (which can move!)
		PyMemberDef * ptr() { return v_Defs.data(); }
	};

	// Defines an exposed class (which is not per instance)
	// as well as a list of exposed instances
	struct ExposedClass
	{
		// Instance struct
		// Contains pointer to object
		// and python var name
		struct Instance
		{
			voidptr_t c_ptr;
			std::string pyname;
		};
		// A list of exposed C++ object pointers
		std::list<Instance> Instances;

		// The name of the python class
		std::string PyClassName;
		// Each exposed class has a method definition
		MethodDefinitions m_MethodDef;
		// And members
		MemberDefinitions m_MemberDef;

		// We need to keep this where it won't move
		// (maps don't invalidate refs)
		PyTypeObject m_TypeObject;

		void Prepare();
		PyTypeObject& to() { return m_TypeObject; }


		// Add method definitions before the null terminator
		size_t AddMemberFn(std::string name, PyCFunction fnPtr, int flags, std::string docs = "") {
			return m_MethodDef.AddMethod(name, fnPtr, flags, docs);
		}
		size_t AddMember(std::string name, int type, int offset, int flags, std::string doc = "") {
			m_MemberDef.AddMember(name, type, offset, flags, doc);
		}

		ExposedClass(std::string n = " ", PyTypeObject = { PyVarObject_HEAD_INIT(NULL, 0) }, std::list<Instance> v = {});
	};

	// TODO more doxygen!
	// This is the original pywrapper::object... quite the beast
	/**
	* \class Object
	* \brief This class represents a python object.
	*/
	class Object {
	public:
		/**
		* \brief Constructs a default python object
		*/
		Object();

		/**
		* \brief Constructs a python object from a PyObject pointer.
		*
		* This Object takes ownership of the PyObject* argument. That
		* means no Py_INCREF is performed on it.
		* \param obj The pointer from which to construct this Object.
		*/
		Object(PyObject *obj);

		/**
		* \brief Calls the callable attribute "name" using the provided
		* arguments.
		*
		* This function might throw a std::runtime_error if there is
		* an error when calling the function.
		*
		* \param name The name of the attribute to be called.
		* \param args The arguments which will be used when calling the
		* attribute.
		* \return Python::Object containing the result of the function.
		*/
		template<typename... Args>
		Object call_function(const std::string &name, const Args&... args) {
			pyunique_ptr func(load_function(name));
			// Create the tuple argument
			pyunique_ptr tup(PyTuple_New(sizeof...(args)));
			add_tuple_vars(tup, args...);
			// Call our object
			PyObject *ret(PyObject_CallObject(func.get(), tup.get()));
			if (!ret)
				throw std::runtime_error("Failed to call function " + name);
			return{ ret };
		}

		/**
		* \brief Calls a callable attribute using no arguments.
		*
		* This function might throw a std::runtime_error if there is
		* an error when calling the function.
		*
		* \sa Python::Object::call_function.
		* \param name The name of the callable attribute to be executed.
		* \return Python::Object containing the result of the function.
		*/
		Object call_function(const std::string &name);

		/**
		* \brief Finds and returns the attribute named "name".
		*
		* This function might throw a std::runtime_error if an error
		* is encountered while fetching the attribute.
		*
		* \param name The name of the attribute to be returned.
		* \return Python::Object representing the attribute.
		*/
		Object get_attr(const std::string &name);

		/**
		* \brief Checks whether this object contains a certain attribute.
		*
		* \param name The name of the attribute to be searched.
		* \return bool indicating whether the attribute is defined.
		*/
		bool has_attr(const std::string &name);

		/**
		* \brief Returns the internal PyObject*.
		*
		* No reference increment is performed on the PyObject* before
		* returning it, so any DECREF applied to it without INCREF'ing
		* it will cause undefined behaviour.
		* \return The PyObject* which this Object is representing.
		*/
		PyObject *get() const { return py_obj.get(); }

		template<class T>
		bool convert(T &param) {
			return Python::convert(py_obj.get(), param);
		}

		/**
		* \brief Constructs a Python::Object from a script.
		*
		* The returned Object will be the representation of the loaded
		* script. If any errors are encountered while loading this
		* script, a std::runtime_error is thrown.
		*
		* \param script_path The path of the script to be loaded.
		* \return Object representing the loaded script.
		*/
		static Object from_script(const std::string &script_path);
	private:
		typedef std::shared_ptr<PyObject> pyshared_ptr;

		PyObject *load_function(const std::string &name);

		pyshared_ptr make_pyshared(PyObject *obj);

		// Variadic template method to add items to a tuple
		template<typename First, typename... Rest>
		void add_tuple_vars(pyunique_ptr &tup, const First &head, const Rest&... tail) {
			add_tuple_var(
				tup,
				PyTuple_Size(tup.get()) - sizeof...(tail)-1,
				head
				);
			add_tuple_vars(tup, tail...);
		}


		void add_tuple_vars(pyunique_ptr &tup, PyObject *arg) {
			add_tuple_var(tup, PyTuple_Size(tup.get()) - 1, arg);
		}

		// Base case for add_tuple_vars
		template<typename Arg>
		void add_tuple_vars(pyunique_ptr &tup, const Arg &arg) {
			add_tuple_var(tup,
				PyTuple_Size(tup.get()) - 1, alloc_pyobject(arg)
				);
		}

		// Adds a PyObject* to the tuple object
		void add_tuple_var(pyunique_ptr &tup, Py_ssize_t i, PyObject *pobj) {
			PyTuple_SetItem(tup.get(), i, pobj);
		}

		// Adds a PyObject* to the tuple object
		template<class T> void add_tuple_var(pyunique_ptr &tup, Py_ssize_t i,
			const T &data) {
			PyTuple_SetItem(tup.get(), i, alloc_pyobject(data));
		}

		pyshared_ptr py_obj;
	};
}