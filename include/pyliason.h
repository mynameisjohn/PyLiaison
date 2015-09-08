#pragma once

// pyliason.h
// Basically a gateway to the other header files.
// Most of the functions used by clients live in
// pyl_expose.h

// This header contains the functions
// used to expose functions and objects
// to the python interpreter

#include <mutex>

#include "pyl_expose.h"

namespace Python
{
    // Mutex that gets locked when the interpreter runs simple commands
    // TODO there are plenty of commands other than PyRun_SimpleString
    // that would require a lock, so I'm not sure how you want to handle it
	extern std::mutex CmdMutex;
	
    // Initialize python interpreter, load module, define any registered classes
	void initialize();
    
    // Finalize python interpreter
	void finalize();
    
    // PyErr_Print/Clear, PyObject_Print
	void print_error();
	void clear_error();
	void print_object(PyObject *obj);

    // This gets called when the custom module is loaded
	PyMODINIT_FUNC _Mod_Init();

    // Thread safe way of running a python command
	inline int RunCmd(std::string cmd) {
		std::lock_guard<std::mutex> lg(CmdMutex);
		return PyRun_SimpleString(cmd.c_str());
	}
	
    // Invokes the above with a file from disk
    int RunFile(std::string fileName);
}