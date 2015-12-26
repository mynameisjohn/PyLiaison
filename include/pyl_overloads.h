#pragma once

#include "pyliason.h"

// This shows an example of how to create a custom
// conversion to a C++ type from a PyType
// Problems: If you've registered a type and
// would like to convert a pointer of that type, you're
// shit out of luck. This should be reserved for POD
// types that you'd like to pass back and forth
// between the interpreter and host code

// Forward your type here
struct Vector3;

namespace Python
{
    // Declare the conversion
	// (by reference, not pointer)
	bool convert(PyObject * o, Vector3& v);

	// Declare allocation
	PyObject * alloc_pyobject(Vector3 v);
}

//#include <glm/fwd.hpp>
//
//namespace Python
//{
//	bool convert(PyObject * o, glm::vec2& v);
//	bool convert(PyObject * o, glm::vec3& v);
//	bool convert(PyObject * o, glm::vec4& v);
//	bool convert(PyObject * o, glm::fquat& v);
//}
//
//struct KeyState;
//namespace Python
//{
//	PyObject * alloc_pyobject(const KeyState& v);
//}