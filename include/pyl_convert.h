#pragma once

#include <vector>
#include <list>
#include <map>
#include <array>

#include <Python.h>

#include "pyl_overloads.h"

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
		if (!PyLong_Check(obj))
			return false;
		val = PyLong_AsLong(obj);
		return true;
	}

	// This gets invoked on calls to member functions, which require the instance ptr
    // It may be dangerous, since any pointer type will be interpreted
    // as a PyCObject, but so far it's been useful (do I need the reverse function?)
	template<typename T>
	bool convert(PyObject * obj, T *& val) {
		//if (!PyCapsule_CheckExact(obj))
		val = static_cast<T *>(PyCapsule_GetPointer(obj, NULL));
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
    
    // Convert a PyObject to a contiguous buffer (very unsafe, but hey)
    template<class T> bool convert_buf(PyObject *obj, T * arr, int N){
        if (!PyList_Check(obj))
            return false;
        Py_ssize_t len = PyList_Size(obj);
        if (len > N) len = N;
        for (Py_ssize_t i(0); i < len; ++i) {
            T& val = arr[i];
            if (!convert(PyList_GetItem(obj, i), val))
                return false;
        }
        return true;
    }
    
    // Convert a PyObject to a std::array, safe version of above
    template<class T, size_t N> bool convert_buf(PyObject *obj, std::array<T, N>& arr){
        return convert_buf<T>(obj, arr.data(), int(N));
    }

    // Convert a PyObject to a std::array
//    template<class T, size_t N> bool convert(PyObject *obj, std::array<T, N>& arr){
//        return convert_buf<T>(obj, arr.data(), int(N)>;
//    }

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

	// Creates a PyObject from any integral type(gets converted to PyLong)
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	PyObject *alloc_pyobject(T num) {
		return PyLong_FromLong(num);
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

    // I guess this is kind of a catch-all for pointer types
    template <typename T>
    PyObject * alloc_pyobject(T * ptr){
        return PyCapsule_New((voidptr_t)ptr, NULL, NULL);
    }
    
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
