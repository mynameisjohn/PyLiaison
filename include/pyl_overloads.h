#pragma once

#include "pyliason.h"

//#include <glm/glm.hpp>
//#include <glm/gtc/quaternion.hpp>

// I think glm is the exception here
#include <glm/fwd.hpp>

// I've implemented the convert function for Bar
// in main.cpp, meaning as long as pyl_convert.h
// can see the template specialization declaration,
// a forwarded class conversion is fine. GLM is special
// and requires I includes its 'forward' header file,
// rather than forward the classes myself, but that's
// not a big deal
class Bar;

namespace Python
{
	bool convert(PyObject * o, glm::vec2& v);
	bool convert(PyObject * o, glm::vec3& v);
	bool convert(PyObject * o, glm::vec4& v);
	bool convert(PyObject * o, glm::fquat& v);
    
    bool convert(PyObject * o, Bar& v);
}
