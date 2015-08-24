/*      This program is free software; you can redistribute it and/or modify
*      it under the terms of the GNU General Public License as published by
*      the Free Software Foundation; either version 3 of the License, or
*      (at your option) any later version.
*
*      This program is distributed in the hope that it will be useful,
*      but WITHOUT ANY WARRANTY; without even the implied warranty of
*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*      GNU General Public License for more details.
*
*      You should have received a copy of the GNU General Public License
*      along with this program; if not, write to the Free Software
*      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*      MA 02110-1301, USA.
*
*      Author:
*      Matias Fontanini
*
*/

#ifndef PYLIASON_H
#define PYLIASON_H

#include <functional>
#include <string>
#include <stdexcept>
#include <utility>
#include <memory>
#include <map>
#include <vector>
#include <list>
#include <tuple>
#include <Python.h>
#include <typeindex>
#include <mutex>

namespace Python {
	// Deleter that calls Py_XDECREF on the PyObject parameter.
	struct PyObjectDeleter {
		void operator()(PyObject *obj) {
			Py_XDECREF(obj);
		}
	};
	// unique_ptr that uses Py_XDECREF as the destructor function.
	typedef std::unique_ptr<PyObject, PyObjectDeleter> pyunique_ptr;

	// ------------ Conversion functions ------------

	// Convert a PyObject to a std::string.
	bool convert(PyObject *obj, std::string &val);
	// Convert a PyObject to a std::vector<char>.
	bool convert(PyObject *obj, std::vector<char> &val);
	// Convert a PyObject to a bool value.
	bool convert(PyObject *obj, bool &value);
	// Convert a PyObject to any integral type.
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	bool convert(PyObject *obj, T &val) {
		if (!PyInt_Check(obj))
			return false;
		val = PyInt_AsLong(obj);
		return true;
	}

	// I need pointer conversion
	template<typename T>
	bool convert(PyObject * obj, T *& val) {
		val = static_cast<T *>(PyCObject_AsVoidPtr(obj));
		return true;
	}

	// Convert a PyObject to an float.
	bool convert(PyObject *obj, double &val);
	bool convert(PyObject *obj, float &val);

	template<size_t n, class... Args>
	typename std::enable_if<n == 0, bool>::type
		add_to_tuple(PyObject *obj, std::tuple<Args...> &tup) {
		return convert(PyTuple_GetItem(obj, n), std::get<n>(tup));
	}

	template<size_t n, class... Args>
	typename std::enable_if<n != 0, bool>::type
		add_to_tuple(PyObject *obj, std::tuple<Args...> &tup) {
		add_to_tuple<n - 1, Args...>(obj, tup);
		return convert(PyTuple_GetItem(obj, n), std::get<n>(tup));
	}

	template<class... Args>
	bool convert(PyObject *obj, std::tuple<Args...> &tup) {
		if (!PyTuple_Check(obj) ||
			PyTuple_Size(obj) != sizeof...(Args))
			return false;
		return add_to_tuple<sizeof...(Args)-1, Args...>(obj, tup);
	}
	// Convert a PyObject to a std::map
	template<class K, class V>
	bool convert(PyObject *obj, std::map<K, V> &mp) {
		if (!PyDict_Check(obj))
			return false;
		PyObject *py_key, *py_val;
		Py_ssize_t pos(0);
		while (PyDict_Next(obj, &pos, &py_key, &py_val)) {
			K key;
			if (!convert(py_key, key))
				return false;
			V val;
			if (!convert(py_val, val))
				return false;
			mp.insert(std::make_pair(key, val));
		}
		return true;
	}
	// Convert a PyObject to a generic container.
	template<class T, class C>
	bool convert_list(PyObject *obj, C &container) {
		if (!PyList_Check(obj))
			return false;
		for (Py_ssize_t i(0); i < PyList_Size(obj); ++i) {
			T val;
			if (!convert(PyList_GetItem(obj, i), val))
				return false;
			container.push_back(std::move(val));
		}
		return true;
	}
	// Convert a PyObject to a std::list.
	template<class T> bool convert(PyObject *obj, std::list<T> &lst) {
		return convert_list<T, std::list<T>>(obj, lst);
	}
	// Convert a PyObject to a std::vector.
	template<class T> bool convert(PyObject *obj, std::vector<T> &vec) {
		return convert_list<T, std::vector<T>>(obj, vec);
	}

	template<class T> bool generic_convert(PyObject *obj,
		const std::function<bool(PyObject*)> &is_obj,
		const std::function<T(PyObject*)> &converter,
		T &val) {
		if (!is_obj(obj))
			return false;
		val = converter(obj);
		return true;
	}

	// -------------- PyObject allocators ----------------

	// Generic python list allocation
	template<class T> static PyObject *alloc_list(const T &container) {
		PyObject *lst(PyList_New(container.size()));

		Py_ssize_t i(0);
		for (auto it(container.begin()); it != container.end(); ++it)
			PyList_SetItem(lst, i++, alloc_pyobject(*it));

		return lst;
	}
	// Creates a PyObject from a std::string
	PyObject *alloc_pyobject(const std::string &str);
	// Creates a PyObject from a std::vector<char>
	PyObject *alloc_pyobject(const std::vector<char> &val, size_t sz);
	// Creates a PyObject from a std::vector<char>
	PyObject *alloc_pyobject(const std::vector<char> &val);
	// Creates a PyObject from a const char*
	PyObject *alloc_pyobject(const char *cstr);
	// Creates a PyObject from any integral type(gets converted to PyInt)
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	PyObject *alloc_pyobject(T num) {
		return PyInt_FromLong(num);
	}
	// Creates a PyObject from a bool
	PyObject *alloc_pyobject(bool value);
	// Creates a PyObject from a double
	PyObject *alloc_pyobject(double num);
	// Creates a PyObject from a std::vector
	template<class T> PyObject *alloc_pyobject(const std::vector<T> &container) {
		return alloc_list(container);
	}
	// Creates a PyObject from a std::list
	template<class T> PyObject *alloc_pyobject(const std::list<T> &container) {
		return alloc_list(container);
	}
	// Creates a PyObject from a std::map
	template<class T, class K> PyObject *alloc_pyobject(
		const std::map<T, K> &container) {
		PyObject *dict(PyDict_New());

		for (auto it(container.begin()); it != container.end(); ++it)
			PyDict_SetItem(dict,
				alloc_pyobject(it->first),
				alloc_pyobject(it->second)
				);

		return dict;
	}

	void initialize();
	void finalize();
	void print_error();
	void clear_error();
	void print_object(PyObject *obj);

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



	///////////////////////////////////////////////////////////////////////

	// implementation details, users never invoke these directly
	namespace detail
	{
		template <typename R, typename F, typename Tuple, bool Done, int Total, int... N>
		struct call_impl
		{
			static R call(F f, Tuple && t)
			{
				return call_impl<R, F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
			}
		};

		template <typename R, typename F, typename Tuple, int Total, int... N>
		struct call_impl<R, F, Tuple, true, Total, N...>
		{
			static R call(F f, Tuple && t)
			{
				return f(std::get<N>(std::forward<Tuple>(t))...);
			}
		};
	}

	// user invokes this
	template <typename R, typename F, typename Tuple>
	R call(F f, Tuple && t)
	{
		typedef typename std::decay<Tuple>::type ttype;
		return detail::call_impl<R, F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
	}

	template <const size_t _UniqueId, typename _Res, typename... _ArgTypes>
	struct fun_ptr_helper
	{
	public:
		typedef std::function<_Res(_ArgTypes...)> function_type;

		static void bind(function_type&& f)
		{
			instance().fn_.swap(f);
		}

		static void bind(const function_type& f)
		{
			instance().fn_ = f;
		}

		static _Res invoke(_ArgTypes... args)
		{
			return instance().fn_(args...);
		}

		typedef decltype(&fun_ptr_helper::invoke) pointer_type;
		static pointer_type ptr()
		{
			return &invoke;
		}

	private:
		static fun_ptr_helper& instance()
		{
			static fun_ptr_helper inst_;
			return inst_;
		}

		fun_ptr_helper() {}

		function_type fn_;
	};

	template <const size_t _UniqueId, typename _Res, typename... _ArgTypes>
	typename fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::pointer_type
		get_fn_ptr(const std::function<_Res(_ArgTypes...)>& f)
	{
		fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::bind(f);
		return fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::ptr();
	}

	template<typename T>
	std::function<typename std::enable_if<std::is_function<T>::value, T>::type>
		make_function(T *t)
	{
		return{ t };
	}

	using PyFunc = std::function<PyObject *(PyObject *, PyObject *)>;

	struct ExposedClass
	{
		std::vector<PyObject *> instances;
		std::string pyname;
		std::string classDef;
		ExposedClass(std::string n = " ", std::string d = "", std::vector<PyObject *> v = {});
	};

	// We need to keep the method definition's
	// string name and docs stored somewhere,
	// where their references are good, since they're char *
	// Also note! std::vectors can move in memory,
	// so when once this is exposed to python it shouldn't
	// be modified
	struct MethodDefinitions
	{
		// Method defs must be contiguous
		std::vector<PyMethodDef> v_Defs;

		// These containers don't invalidate references
		std::list<std::string> MethodNames, MethodDocs;

		// By default add the "null terminator",
		// all other methods are inserted before it
		MethodDefinitions() :
			v_Defs(1, { NULL, NULL, 0, })
		{}

		// Add method definitions before the null terminator
		size_t AddMethod(std::string name, PyCFunction fnPtr, int flags, std::string docs = "");

		// pointer to the definitions (which can move!)
		PyMethodDef * ptr() { return v_Defs.data(); }
	};

	extern std::map<std::type_index, ExposedClass> ExposedClasses;
	extern std::string ClassesDef; // Do I still need this?
	extern std::list<PyFunc> ExposedFunctions;
	extern MethodDefinitions MethodDef;
	extern std::mutex CmdMutex;
	extern PyObject * Py_ErrorObj;

	static int RunCmd(std::string cmd) {
		std::lock_guard<std::mutex> lg(CmdMutex);
		return PyRun_SimpleString(cmd.c_str());
	}

	//int Runfile(std::string fileName) {
	//	// load this to a string and run it
	//}

	// The idea is to make a module where all your functions live
	const static std::string ModuleName = "spam";

	// This gets called when the module is imported
	PyMODINIT_FUNC _Mod_Init();

	// This function generates a python class definition
	template <class C>
	static void Register_Class(std::string className) {
		if (ExposedClasses.find(typeid(C)) != ExposedClasses.end())
			return;

		std::string classDef;
		classDef.append("class ").append(className).append(":\n\
			\tdef __init__( self, ecsPtr ):\n\
			\t\tself._self = ecsPtr\n\
			\tdef __call__( self ):\n\
			\t\treturn self._self\n\n");

		ExposedClasses[std::type_index(typeid(C))] = ExposedClass(className, classDef, std::vector<PyObject *>());

		// Run the class def to declare the class; we shouldn't do this
		// until all class definitionsa are declared
		//PyRun_SimpleString(classDef.c_str());
	}

	// This is the function that should initialize a class
	// after it's declaration is finalized
	template <class C>
	static void Init_Class() {
		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		PyRun_SimpleString(it->second.classDef.c_str());
	}

	// This will expose a specific C++ object instance as a Python
	// object, giving it a pointer to the original object (which better stay live)
	template <class C>
	static void Expose_Object(C * instance, std::string name) {
		if (!instance)
			return;

		auto it = ExposedClasses.find(typeid(C));
		if (it == ExposedClasses.end())
			return;

		PyObject * pyObject = static_cast<PyObject *>((void *)instance);

		std::string pythonCall;
		PyObject* module = NULL;
		
		PyObject* newPyObject = PyCObject_FromVoidPtr((void *)instance, nullptr);//Py_BuildValue("k", pyObject);

		PyRun_SimpleString("ecsPtr = 0");

		module = PyImport_ImportModule("__main__");
		PyObject_SetAttrString(module, "ecsPtr", newPyObject);

		pythonCall.append(name).append(" = ").append(it->second.pyname).append("(ecsPtr)");
		PyRun_SimpleString(pythonCall.c_str());

		PyRun_SimpleString("del ecsPtr");

		Py_DECREF(module);
		Py_DECREF(newPyObject);

		it->second.instances.push_back(pyObject);
	}

	// TODO
	template <typename T>
	inline PyObject * objToPyObj(const T& Value) {
		return nullptr;
	}
	template <>
	inline PyObject * objToPyObj(const int& Value) {
		return PyLong_FromLong(Value);
	}

	// TODO void functions?
	// This function declares a (global) python function that calls into an existing (global, static?) C++ function
	// Invoke with the macro below, which automatically sets idx from the line number (best I could do...)
	template <size_t idx, typename R, typename ... Args>
	static void _add_Func(std::string methodName, std::function<R(Args...)> fn, int methodFlags, std::string docs = "")
	{
		PyFunc pfn = [fn](PyObject * s, PyObject * a)
		{
			PyObject * ret = nullptr;

			Python::Object args(a);
			std::tuple<Args...> tup;
			args.convert(tup);
			R rVal = call<R>(fn, tup);
			ret = objToPyObj<R>(rVal);

			// The pywrapper deleter
			// will do a decref on a...
			// will this leak?
			Py_XINCREF(a);

			return ret;
		};
		ExposedFunctions.push_back(pfn);

		// now make the function pointer (TODO figure out these ids)
		PyCFunction fnPtr = get_fn_ptr<idx>(ExposedFunctions.back());

		// You can key the methodName string to a std::function
		MethodDef.AddMethod(methodName, fnPtr, methodFlags, docs);// , doc.empty() ? NULL : doc.c_str() );
	}

	// Invokes the above function with the correct
	// unique id, and promotes fn to a std::function
	// However I can't get make_function to work with
	// member functions, hopefully that's doable
#define Py_Add_Func(name, fn, docs)\
	Python::_add_Func<__LINE__>(std::string(name), Python::make_function(fn), METH_VARARGS, docs)

};

#endif // PYLIASON_H
