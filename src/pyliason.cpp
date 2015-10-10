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

#include <algorithm>
#include <fstream>
#include <locale>
#include <codecvt>

#include "pyliason.h"

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
		// Get the directory path and file name
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

		// Try loading just the file name
		PyObject *py_ptr(PyImport_ImportModule(file_path.c_str()));
		if (py_ptr)
			return{ py_ptr };

		// If we didn't get it, see if the dir path is in PyPath
		char arr[] = "path";
		PyObject *path(PySys_GetObject(arr));
		std::vector<std::string> curPath;
		Python::convert(path, curPath);

		// If it isn't add it to the path and try again
		if (std::find(curPath.begin(), curPath.end(), base_path) == curPath.end()) {
			pyunique_ptr pwd(PyUnicode_FromString(base_path.c_str()));
			PyList_Append(path, pwd.get());
			return from_script(script_path);
		}

		// If it was in the path and we still couldn't load, there's a problem
		print_error();
		throw runtime_error("Failed to load script");
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

		// Set up relative path
		//if (argc && argv) {
		//	std::vector<std::wstring> wArgs;
		//	std::vector<wchar_t *> wBuf;
		//	for (int i = 0; i < argc; i++)
		//	{
		//		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		//		wArgs.push_back(converter.from_bytes(argv[i]));
		//		wBuf.push_back((wchar_t *)wArgs.back().c_str());
		//	}

		//	Py_SetProgramName(wBuf[0]);
		//	PySys_SetArgv(argc, wBuf.data());
		//}

		// Call the _Mod_Init function when my module is imported
		PyImport_AppendInittab(ModuleName.c_str(), _Mod_Init);

		// Startup python
		Py_Initialize();

		// Lock down any definitions
		for (auto& e_Class : ExposedClasses)
			e_Class.second.Prepare();

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



	// Allocation methods

	PyObject *alloc_pyobject(const std::string &str) {
		return PyBytes_FromString(str.c_str());
	}

	PyObject *alloc_pyobject(const std::vector<char> &val, size_t sz) {
		return PyByteArray_FromStringAndSize(val.data(), sz);
	}

	PyObject *alloc_pyobject(const std::vector<char> &val) {
		return alloc_pyobject(val, val.size());
	}

	PyObject *alloc_pyobject(const char *cstr) {
		return PyBytes_FromString(cstr);
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
		return PyLong_Check(obj);
	}

	bool is_py_float(PyObject *obj) {
		return PyFloat_Check(obj);
	}

	bool convert(PyObject *obj, std::string &val) {
		if (PyBytes_Check(obj)) {
			val = PyBytes_AsString(obj);
			return true;
		}
		else if (PyUnicode_Check(obj)) {
			val = PyUnicode_AsUTF8(obj);
			return true;
		}
		return false;
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
	
	// If the client knows what to do, let 'em deal with it
	bool convert(PyObject * obj, Python::Object& pyObj){
		pyObj = Python::Object(obj);
		// I noticed that the incref is needed... not sure why?
		if (auto ptr = pyObj.get()) {
			Py_INCREF(ptr);
			return true;
		}
		return false;
	}

	std::map<std::type_index, ExposedClass> ExposedClasses;
	std::string ClassesDef; // Do I still need this?
	std::list<PyFunc> ExposedFunctions;
	MethodDefinitions MethodDef;
	std::mutex CmdMutex;
	PyObject * Py_ErrorObj;

	// These are the black sheep for now
	PyModuleDef ModDef;
	std::string ModDocs;

	// TODO 
	// Have the generic PyTypeObject as a global
	// and inherit from it

	// We only need one instance of the above, shared by exposed objects
	static int PyClsInit (PyObject * self, PyObject * args, PyObject * kwds) 
	{
		// In the example the first arg isn't a PyObject *, but... idk man
		GenericPyClass * realPtr = static_cast<GenericPyClass *>((void *)self);
		// The first argument is the capsule object
		PyObject * c = PyTuple_GetItem(args, 0), *tmp(nullptr);
		
		if (c)
		{// Or at least it better be
			if (PyCapsule_CheckExact(c))
			{
				// These ref counts are all messed up
				tmp = realPtr->capsule;

				Py_INCREF(c);
				realPtr->capsule = c;

				return 0;
			}
		}

		return -1;
	};
	
	// The () operator just returns the capsule object
	static PyObject * PyClsCall(PyObject * co, PyObject * args, PyObject * kw)
	{
		auto realPtr = static_cast<GenericPyClass *>((voidptr_t)co);
		Py_INCREF(realPtr->capsule);
		return realPtr->capsule;
	}

	// Constructor for exposed classes, sets up type object
	ExposedClass::ExposedClass(std::string n) : PyClassName(n)
	{
		// Take care of this now
        memset(&m_TypeObject, 0, sizeof(PyTypeObject));
		m_TypeObject.ob_base = PyVarObject_HEAD_INIT(NULL, 0)
		m_TypeObject.tp_name = PyClassName.c_str();
		m_TypeObject.tp_init = (initproc)PyClsInit;
		m_TypeObject.tp_call = (ternaryfunc)PyClsCall;
		m_TypeObject.tp_new = PyType_GenericNew;
		m_TypeObject.tp_flags = Py_TPFLAGS_DEFAULT;
		m_TypeObject.tp_basicsize = sizeof(GenericPyClass);
	}

	// This has to happen at a time when these
	// definitions will no longer move
	void ExposedClass::Prepare()
	{
		m_TypeObject.tp_members = m_MemberDef.Ptr();
		m_TypeObject.tp_methods = m_MethodDef.Ptr();
	}

	// Add a method, preserving the null terminator and storing strings where they won't be destroyed
	void MethodDefinitions::AddMethod(std::string name, PyCFunction fnPtr, int flags, std::string docs)
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

		PyMethodDef method{ 0 };

		if (docs.empty())
			method = { namePtr, fnPtr, flags, NULL };
		else {
			MethodDocs.push_back(std::string(docs));
			method = { namePtr, fnPtr, flags, MethodDocs.back().c_str() };
		}

		_insert(method);
	}

	// These by default get the c_ptr capsule object
	MemberDefinitions::MemberDefinitions()
		: _NullTermBuf()
	{
		MemberNames.push_back("c_ptr");
		MemberDocs.push_back("pointer to a c object");
		PyMemberDef d = { (char *)MemberNames.back().c_str(), T_OBJECT_EX, offsetof(GenericPyClass, capsule), 0, (char *)MemberDocs.back().c_str() };
		_insert(d);
	}

	// Add a member, preserving the null terminator and storing strings where they won't be destroyed
	void MemberDefinitions::AddMember(std::string name, int type, int offset, int flags, std::string docs)
	{
		// If a member with this name has already been declared, throw an error
		if (std::find(MemberNames.begin(), MemberNames.end(), name) != MemberNames.end())
		{
			// Alternatively, this could actually overwrite the pre-existing member. 
			throw runtime_error("Error: Attempting to overwrite exisiting exposed python class member");
		}

		// We need the names in a list so their references stay valid
		MemberNames.push_back(name);
		char * namePtr = (char *)MemberNames.back().c_str();

		PyMemberDef member{ 0 };
		
		if (docs.empty())
			member = { namePtr, type, offset, flags, NULL };
		else {
			MemberDocs.push_back(std::string(docs));
			member = { namePtr, type, offset, flags, (char *)MemberDocs.back().c_str() };
		}

		_insert(member);
	}

	PyMODINIT_FUNC _Mod_Init()
	{
		// Is it fair to assume the method def is ready?
		// The MethodDef contains all functions defined in C++ code,
		// including those called into by exposed classes

		ModDef = PyModuleDef
		{
			PyModuleDef_HEAD_INIT,
			ModuleName.c_str(),
			ModDocs.c_str(),
			-1,
			MethodDef.Ptr()
		};

		PyObject * mod = PyModule_Create(&ModDef);
		//if (mod == nullptr) ...

		std::string errName = ModuleName + ".error";
		Py_ErrorObj = PyErr_NewException((char *)errName.c_str(), 0, 0);
		Py_XINCREF(Py_ErrorObj);
		PyModule_AddObject(mod, "error", Py_ErrorObj);

		// Is now the time to declare all classes?
		for (auto& exp_class : ExposedClasses) {
			if (PyType_Ready(&(exp_class.second.m_TypeObject)) < 0)
				assert(false);

			PyModule_AddObject(mod, exp_class.second.PyClassName.c_str(), (PyObject *)&exp_class.second.m_TypeObject);
		}

		return mod;
	}

	int RunFile(std::string file)
	{
		std::ifstream in(file);
		return RunCmd({ (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>() });
	}

	int GetTotalRefCount()
	{
		PyObject* refCount = PyObject_CallObject(PySys_GetObject((char*)"gettotalrefcount"), NULL);
		if (!refCount) return -1;
		int ret = _PyLong_AsInt(refCount);
		Py_DECREF(refCount);
		return ret;
	}

	// Check out the docs for PyImport and see if there's anything cool
	Object GetPyLiaisonModule()
	{
		PyObject * plMod = PyImport_ImportModule(ModuleName.c_str());

		return Object(plMod);
	}
}
