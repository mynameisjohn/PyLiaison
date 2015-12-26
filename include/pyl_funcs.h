#pragma once

#include <Python.h>

#include "pyl_classes.h"

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

}