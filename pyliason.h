#pragma once

#include "pyl_expose.h"
#include "pyl_classes.h"
#include "pyl_convert.h"
#include "pyl_misc.h"

#include <mutex>

namespace Python
{
	extern std::mutex CmdMutex;
	
	void initialize();
	void finalize();
	void print_error();
	void clear_error();
	void print_object(PyObject *obj);

	PyMODINIT_FUNC _Mod_Init();

	static int RunCmd(std::string cmd) {
		std::lock_guard<std::mutex> lg(CmdMutex);
		return PyRun_SimpleString(cmd.c_str());
	}
	
	//int Runfile(std::string fileName) {
	//	// load this to a string and run it
	//}
}