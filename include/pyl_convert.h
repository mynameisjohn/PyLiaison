#pragma once

#include <vector>
#include <list>
#include <map>

#include <Python.h>

namespace Python
{
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

	// Generic convert function used by others
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

	// Creates a PyObject from any integral type(gets converted to PyInt)
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	PyObject *alloc_pyobject(T num) {
		return PyInt_FromLong(num);
	}

	// Generic python list allocation
	template<class T> static PyObject *alloc_list(const T &container) {
		PyObject *lst(PyList_New(container.size()));

		Py_ssize_t i(0);
		for (auto it(container.begin()); it != container.end(); ++it)
			PyList_SetItem(lst, i++, alloc_pyobject(*it));

		return lst;
	}

	// Creates a PyString from a std::string
	PyObject *alloc_pyobject(const std::string &str);

	// Creates a PyByteArray from a std::vector<char>
	PyObject *alloc_pyobject(const std::vector<char> &val, size_t sz);

	// Creates a PyByteArray from a std::vector<char>
	PyObject *alloc_pyobject(const std::vector<char> &val);

	// Creates a PyString from a const char*
	PyObject *alloc_pyobject(const char *cstr);

	// Creates a PyBool from a bool
	PyObject *alloc_pyobject(bool value);

	// Creates a PyFloat from a double
	PyObject *alloc_pyobject(double num);

	// Creates a PyFloat from a float
	PyObject *alloc_pyobject(float num);

	// Creates a PyList from a std::vector
	template<class T> PyObject *alloc_pyobject(const std::vector<T> &container) {
		return alloc_list(container);
	}

	// Creates a PyList from a std::list
	template<class T> PyObject *alloc_pyobject(const std::list<T> &container) {
		return alloc_list(container);
	}

	// Creates a PyDict from a std::map
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

	// TODO std::set? And unordered map/set?
}