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

#include "pyliason.h"

#include <algorithm>
#include <fstream>

namespace Python {
	using std::runtime_error;
	using std::string;

	Object::Object() {

	}

	Object::Object(PyObject *obj) : py_obj(make_pyshared(obj)) {

	}

	Python::Object::pyshared_ptr Object::make_pyshared(PyObject *obj) {
		return pyshared_ptr(obj, [](PyObject *obj) { Py_XDECREF(obj); });
	}

	Object Object::from_script(const string &script_path) {
		char arr[] = "path";
		PyObject *path(PySys_GetObject(arr));
		string base_path("."), file_path;
		size_t last_slash(script_path.rfind("/"));
		if (last_slash != string::npos) {
			if (last_slash >= script_path.size() - 2)
				throw runtime_error("Invalid script path");
			base_path = script_path.substr(0, last_slash);
			file_path = script_path.substr(last_slash + 1);
		}
		else
			file_path = script_path;
		if (file_path.rfind(".py") == file_path.size() - 3)
			file_path = file_path.substr(0, file_path.size() - 3);
		pyunique_ptr pwd(PyString_FromString(base_path.c_str()));

		PyList_Append(path, pwd.get());
		/* We don't need that string value anymore, so deref it */
		PyObject *py_ptr(PyImport_ImportModule(file_path.c_str()));
		if (!py_ptr) {
			print_error();
			throw runtime_error("Failed to load script");
		}
		return{ py_ptr };
	}

	PyObject *Object::load_function(const std::string &name) {
		PyObject *obj(PyObject_GetAttrString(py_obj.get(), name.c_str()));
		if (!obj)
			throw std::runtime_error("Failed to find function");
		return obj;
	}

	Object Object::call_function(const std::string &name) {
		pyunique_ptr func(load_function(name));
		PyObject *ret(PyObject_CallObject(func.get(), 0));
		if (!ret)
			throw std::runtime_error("Failed to call function");
		return{ ret };
	}

	Object Object::get_attr(const std::string &name) {
		PyObject *obj(PyObject_GetAttrString(py_obj.get(), name.c_str()));
		if (!obj)
			throw std::runtime_error("Unable to find attribute '" + name + '\'');
		return{ obj };
	}

	bool Object::has_attr(const std::string &name) {
		try {
			get_attr(name);
			return true;
		}
		catch (std::runtime_error&) {
			return false;
		}
	}

	void initialize() {
        // Finalize any previous stuff
        Py_Finalize();
        
        // Call the _Mod_Init function when my module is imported
        PyImport_AppendInittab(ModuleName.c_str(), _Mod_Init);
        
        // Startup python
			Py_Initialize();
        

        // Import our module (this makes it a bit easier)
			std::string importString = "from " + ModuleName + " import *";
        
        // It seems like this is a bad time to lock a mutex
        // so just run it the old fashioned way
        PyRun_SimpleString(importString.c_str());
	}

	void finalize() {
		Py_Finalize();
	}

	void clear_error() {
		PyErr_Clear();
	}

	void print_error() {
		PyErr_Print();
	}

	void print_object(PyObject *obj) {
		PyObject_Print(obj, stdout, 0);
	}
    
    // TODO
    // What's the difference between this and pywrapper::from_script
    int RunFile(std::string fileName){
        std::ifstream in(fileName);
        std::string cmd((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return RunCmd(cmd);
    }

	// Allocation methods

	PyObject *alloc_pyobject(const std::string &str) {
		return PyString_FromString(str.c_str());
	}

	PyObject *alloc_pyobject(const std::vector<char> &val, size_t sz) {
		return PyByteArray_FromStringAndSize(val.data(), sz);
	}

	PyObject *alloc_pyobject(const std::vector<char> &val) {
		return alloc_pyobject(val, val.size());
	}

	PyObject *alloc_pyobject(const char *cstr) {
		return PyString_FromString(cstr);
	}

	PyObject *alloc_pyobject(bool value) {
		return PyBool_FromLong(value);
	}

	PyObject *alloc_pyobject(double num) {
		return PyFloat_FromDouble(num);
	}

	PyObject *alloc_pyobject(float num) {
		double d_num(num);
		return PyFloat_FromDouble(d_num);
	}

	bool is_py_int(PyObject *obj) {
		return PyInt_Check(obj);
	}

	bool is_py_float(PyObject *obj) {
		return PyFloat_Check(obj);
	}

	bool convert(PyObject *obj, std::string &val) {
		if (!PyString_Check(obj))
			return false;
		val = PyString_AsString(obj);
		return true;
	}

	bool convert(PyObject *obj, std::vector<char> &val) {
		if (!PyByteArray_Check(obj))
			return false;
		if (val.size() < (size_t)PyByteArray_Size(obj))
			val.resize(PyByteArray_Size(obj));
		std::copy(PyByteArray_AsString(obj),
			PyByteArray_AsString(obj) + PyByteArray_Size(obj),
			val.begin());
		return true;
	}

	/*bool convert(PyObject *obj, Py_ssize_t &val) {
	return generic_convert<Py_ssize_t>(obj, is_py_int, PyInt_AsSsize_t, val);
	}*/
	bool convert(PyObject *obj, bool &value) {
		if (obj == Py_False)
			value = false;
		else if (obj == Py_True)
			value = true;
		else
			return false;
		return true;
	}

	bool convert(PyObject *obj, double &val) {
		return generic_convert<double>(obj, is_py_float, PyFloat_AsDouble, val);
	}

	// It's unforunate that this takes so long
	bool convert(PyObject *obj, float &val) {
		double d(0);
		bool ret = generic_convert<double>(obj, is_py_float, PyFloat_AsDouble, d);
		val = float(d);
		return ret;
	}

	std::map<std::type_index, ExposedClass> ExposedClasses;
	std::string ClassesDef; // Do I still need this?
	std::list<PyFunc> ExposedFunctions;
	MethodDefinitions MethodDef;
	std::mutex CmdMutex;
	PyObject * Py_ErrorObj;

	ExposedClass::ExposedClass(std::string n , std::string d, std::list<Instance> v) :
		PyClassName(n),
		ClassDef(d),
		Instances(v)
	{}

	size_t MethodDefinitions::AddMethod(std::string name, PyCFunction fnPtr, int flags, std::string docs)
	{
		// If a method with this name has already been declared, throw an error
		if (std::find(MethodNames.begin(), MethodNames.end(), name) != MethodNames.end())
		{
			// Alternatively, this could actually overwrite the pre-existing method. 
			throw runtime_error("Error: Attempting to overwrite exisiting exposed python function");
		}
			
		// We need the names in a list so their references stay valid
		MethodNames.push_back(name);
		const char * namePtr = MethodNames.back().c_str();

		PyMethodDef method;

		if (docs.empty())
			method = { MethodNames.back().c_str(), fnPtr, flags, NULL };
		else {
			MethodDocs.push_back(std::string(docs));
			method = { MethodNames.back().c_str(), fnPtr, flags, MethodDocs.back().c_str() };
		}

		v_Defs.insert(v_Defs.end() - 1, method);
		return v_Defs.size();
	}

	PyMODINIT_FUNC _Mod_Init()
	{
		// Is it fair to assume the method def is ready?
		// The MethodDef contains all functions defined in C++ code,
		// including those called into by exposed classes
		PyObject * mod = Py_InitModule(ModuleName.c_str(), MethodDef.ptr());
        
		//if (mod == nullptr) ...
        std::string errName = ModuleName + ".error";
		Py_ErrorObj = PyErr_NewException((char *)errName.c_str(), 0, 0);
		Py_XINCREF(Py_ErrorObj);
		PyModule_AddObject(mod, "error", Py_ErrorObj);

		// Is now the time to declare all classes?
		for (auto& exp_class : ExposedClasses)
		{
			const std::string& classDef = exp_class.second.ClassDef;
			RunCmd(classDef.c_str());
		}
	}
}
