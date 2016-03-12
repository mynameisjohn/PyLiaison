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
*      John Joseph
*
*/

#pragma once

// pyliason.h
// Basically a gateway to the other header files.
// Most of the functions used by clients live in
// pyl_expose.h

// This header contains the functions
// used to expose functions and objects
// to the python interpreter

#include <mutex>

#include "pyl_misc.h"
#include "pyl_module.h"

namespace pyl
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

	// Get the PyLiaison Module object
	// TODO Make an arbitrary number of modules
	// and have them gettable by clients
	//Object GetPyLiaisonModule();
}
