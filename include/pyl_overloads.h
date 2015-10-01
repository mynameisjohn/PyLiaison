#pragma once

#include "pyliason.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Python
{
	bool convert(PyObject * o, glm::vec2& v);
	bool convert(PyObject * o, glm::vec3& v);
	bool convert(PyObject * o, glm::vec4& v);
	bool convert(PyObject * o, glm::fquat& v);
}
