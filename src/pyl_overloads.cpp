// Overloading things for a project specific case
// just use glm as an example


#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "pyl_overloads.h"

namespace Python
{
	bool convert(PyObject * o, glm::vec2& v){
		return convert_buf(o, &v[0], sizeof(v)/sizeof(float));
	}
	bool convert(PyObject * o, glm::vec3& v){
		return convert_buf(o, &v[0], sizeof(v)/sizeof(float));
	}
	bool convert(PyObject * o, glm::vec4& v){
		return convert_buf(o, &v[0], sizeof(v)/sizeof(float));
	}
	bool convert(PyObject * o, glm::fquat& v){
		return convert_buf(o, &v[0], sizeof(v)/sizeof(float));
    }
}
