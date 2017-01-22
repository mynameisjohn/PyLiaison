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

#include <algorithm>
#include <fstream>

#include <Python.h>
#include <structmember.h>

#include "pyliaison.h"

namespace pyl
{
	Object::Object() : m_pPyObject( nullptr )
	{}

	Object::Object( PyObject *obj ) : m_pPyObject( obj )
	{
		Py_XINCREF( obj );
	}

	Object::Object( const std::string script_path )
	{
		// Get the directory path and file name
		std::string base_path( "." ), file_path;
		size_t last_slash( script_path.rfind( "/" ) );
		if ( last_slash != std::string::npos )
		{
			if ( last_slash >= script_path.size() - 2 )
				throw runtime_error( "Invalid script path" );
			base_path = script_path.substr( 0, last_slash );
			file_path = script_path.substr( last_slash + 1 );
		}
		else
			file_path = script_path;
		if ( file_path.rfind( ".py" ) == file_path.size() - 3 )
			file_path = file_path.substr( 0, file_path.size() - 3 );

		PyObject * pScript = PyImport_ImportModule( file_path.c_str() );
		if ( pScript )
		{
			m_pPyObject = pScript;
			Py_XINCREF( m_pPyObject );
		}

		// If we didn't get it, see if the dir path is in PyPath
		char arrPath[] = "path";
		pyl::Object path( PySys_GetObject( arrPath ) );
		std::set<std::string> curPath;

		pyl::convert( path.get(), curPath );
		// If it isn't add it to the path and try again
		if ( std::find( curPath.begin(), curPath.end(), base_path ) == curPath.end() )
		{
			Object pwd( PyUnicode_FromString( base_path.c_str() ) );
			PyList_Append( path.get(), pwd.get() );
		}

		pScript = PyImport_ImportModule( file_path.c_str() );
		if ( pScript )
		{
			m_pPyObject = pScript;
			Py_XINCREF( m_pPyObject );
		}

		print_error();
		throw runtime_error( "Error locating module" );
	}

	Object Object::_call_impl( const std::string strName, PyObject * pArgTup /*= nullptr*/ )
	{
		// Try to call, clean memory on error
		Object func;
		try
		{
			// Load function and arguments into unique ptrs
			func = get_attr( strName );

			// Call object, return result
			PyObject * pRet = PyObject_CallObject( func.get(), pArgTup );
			if ( pRet == nullptr )
				throw pyl::runtime_error( "Failed to call function " + strName );
			return { pRet };
		}
		// Release memory and pass along
		catch ( pyl::runtime_error e )
		{
			PyErr_Print();
			func.reset();
			throw e;
		}
		return { nullptr };
	}

	Object Object::call( const std::string strName )
	{
		return _call_impl( strName );
	}

	Object Object::get_attr( const std::string strName )
	{
		PyObject *obj( PyObject_GetAttrString( m_pPyObject, strName.c_str() ) );
		if ( !obj )
			throw std::runtime_error( "Unable to find attribute '" + strName + '\'' );
		return { obj };
	}

	bool Object::has_attr( const std::string strName )
	{
		try
		{
			get_attr( strName );
		}
		catch ( pyl::runtime_error& ) { return false; }

		return true;
	}

	void Object::reset()
	{
		if ( m_pPyObject )
		{
			Py_XDECREF( m_pPyObject );
			m_pPyObject = nullptr;
		}
	}

	PyObject * Object::get() const
	{
		return m_pPyObject;
	}

	void initialize()
	{
		// Finalize any previous stuff
		Py_Finalize();

		ModuleDef::InitAllModules();

		// Startup python
		Py_Initialize();
	}

	void finalize()
	{
		Py_Finalize();
	}

	void clear_error()
	{
		PyErr_Clear();
	}

	void print_error()
	{
		PyErr_Print();
	}

	void print_object( PyObject *obj )
	{
		PyObject_Print( obj, stdout, 0 );
	}

	// Allocation methods

	PyObject *alloc_pyobject( const std::string &str )
	{
		return PyBytes_FromString( str.c_str() );
	}

	PyObject *alloc_pyobject( const std::vector<char> &val, size_t sz )
	{
		return PyByteArray_FromStringAndSize( val.data(), sz );
	}

	PyObject *alloc_pyobject( const std::vector<char> &val )
	{
		return alloc_pyobject( val, val.size() );
	}

	PyObject *alloc_pyobject( const char *cstr )
	{
		return PyBytes_FromString( cstr );
	}

	PyObject *alloc_pyobject( bool value )
	{
		return PyBool_FromLong( value );
	}

	PyObject *alloc_pyobject( double num )
	{
		return PyFloat_FromDouble( num );
	}

	PyObject *alloc_pyobject( float num )
	{
		double d_num( num );
		return PyFloat_FromDouble( d_num );
	}

	bool is_py_int( PyObject *obj )
	{
		return PyLong_Check( obj );
	}

	bool is_py_float( PyObject *obj )
	{
		return PyFloat_Check( obj );
	}

	bool convert( PyObject *obj, std::string &val )
	{
		if ( PyBytes_Check( obj ) )
		{
			val = PyBytes_AsString( obj );
			return true;
		}
		else if ( PyUnicode_Check( obj ) )
		{
			val = PyUnicode_AsUTF8( obj );
			return true;
		}
		return false;
	}

	bool convert( PyObject *obj, std::vector<char> &val )
	{
		if ( !PyByteArray_Check( obj ) )
			return false;
		if ( val.size() < (size_t) PyByteArray_Size( obj ) )
			val.resize( PyByteArray_Size( obj ) );
		std::copy( PyByteArray_AsString( obj ),
				   PyByteArray_AsString( obj ) + PyByteArray_Size( obj ),
				   val.begin() );
		return true;
	}

	bool convert( PyObject *obj, bool &value )
	{
		if ( obj == Py_False )
			value = false;
		else if ( obj == Py_True )
			value = true;
		else
			return false;
		return true;
	}

	bool convert( PyObject *obj, double &val )
	{
		return _generic_convert<double>( obj, is_py_float, PyFloat_AsDouble, val );
	}

	// It's unforunate that this takes so long
	bool convert( PyObject *obj, float &val )
	{
		double d( 0 );
		if ( convert( obj, d ) )
		{
			val = (float) d;
			return true;
		}
		int i( 0 );
		if ( convert( obj, i ) )
		{
			val = (float) i;
			return true;
		}
		return false;
	}

	// If the client knows what to do, let 'em deal with it
	bool convert( PyObject * obj, pyl::Object& pyObj )
	{
		pyObj = pyl::Object( obj );
		// I noticed that the incref is needed... not sure why?
		if ( auto ptr = pyObj.get() )
		{
			Py_INCREF( ptr );
			return true;
		}
		return false;
	}

	// We only need one instance of the above, shared by exposed objects
	/*static*/ int PyClsInitFunc( PyObject * self, PyObject * args, PyObject * kwds )
	{
		// In the example the first arg isn't a PyObject *, but... idk man
		_GenericPyClass * realPtr = static_cast<_GenericPyClass *>( (void *) self );
		// The first argument is the capsule object
		PyObject * c = PyTuple_GetItem( args, 0 );

		if ( c )
		{// Or at least it better be
			if ( PyCapsule_CheckExact( c ) )
			{
				Py_INCREF( c );
				realPtr->capsule = c;

				return 0;
			}
		}

		return -1;
	};

	_ExposedClassDef::_ExposedClassDef()
	{
		// Take care of this now
		memset( &m_TypeObject, 0, sizeof( PyTypeObject ) );
		m_TypeObject.ob_base = PyVarObject_HEAD_INIT( NULL, 0 )
			m_TypeObject.tp_init = (initproc) PyClsInitFunc;
		m_TypeObject.tp_new = PyType_GenericNew;
		m_TypeObject.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
		m_TypeObject.tp_basicsize = sizeof( _GenericPyClass );
	}


	// Constructor for exposed classes, sets up type object
	_ExposedClassDef::_ExposedClassDef( std::string strClassName ) :
		_ExposedClassDef()
	{
		m_strClassName = strClassName;
	}

	// This has to happen at a time when these
	// definitions will no longer move
	void _ExposedClassDef::Prepare()
	{
		AddMember( "c_ptr", T_OBJECT_EX, offsetof( _GenericPyClass, capsule ), 0, "pointer to the underlying c object" );

		// Assing the pointers
		m_TypeObject.tp_name = m_strClassName.c_str();
		m_TypeObject.tp_members = (PyMemberDef *) m_ntMemberDefs.data();
		m_TypeObject.tp_methods = (PyMethodDef *) m_ntMethodDefs.data();
	}

	bool _ExposedClassDef::IsPrepared() const
	{
		return (bool) ( m_TypeObject.tp_name && m_TypeObject.tp_members && m_TypeObject.tp_methods );
	}

	bool _ExposedClassDef::AddMethod( std::string strMethodName, PyCFunction fnPtr, int flags, std::string docs )
	{
		if ( strMethodName.empty() )
		{
			throw std::runtime_error( "Error adding method " + strMethodName );
			return false;
		}

		auto paInsert = m_setUsedMethodNames.insert( strMethodName );
		if ( paInsert.second == false )
		{
			throw std::runtime_error( "Error: Attempting to overwrite exisiting exposed python function" );
			return false;
		}

		const char * pName = paInsert.first->c_str();
		const char * pDocs = docs.empty() ? nullptr : m_liMethodDocs.insert( m_liMethodDocs.end(), docs )->c_str();
		m_ntMethodDefs.push_back( { pName, fnPtr, flags, pDocs } );
		return true;
	}

	bool _ExposedClassDef::AddMember( std::string strMemberName, int type, int offset, int flags, std::string docs )
	{
		if ( strMemberName.empty() )
		{
			throw std::runtime_error( "Error adding member " + strMemberName );
			return false;
		}

		auto paInsert = m_setUsedMemberNames.insert( strMemberName );
		if ( paInsert.second == false )
		{
			throw std::runtime_error( "Error: Attempting to overwrite exisiting exposed python function" );
			return false;
		}

		char * pName = (char *) paInsert.first->c_str();
		char * pDocs = (char *) ( docs.empty() ? nullptr : m_liMemberDocs.insert( m_liMemberDocs.end(), docs )->c_str() );

		m_ntMemberDefs.push_back( { pName, type, offset, flags, pDocs } );
		return true;
	}

	PyTypeObject * _ExposedClassDef::GetTypeObject() const
	{
		return (PyTypeObject *) &m_TypeObject;
	}

	const char * _ExposedClassDef::GetName() const
	{
		return m_strClassName.c_str();
	}

	void _ExposedClassDef::SetName( std::string strName )
	{
		// We can't do this if we're been prepared
		if ( IsPrepared() )
			return;

		m_strClassName = strName;

	}

	int RunCmd( std::string cmd )
	{
		return PyRun_SimpleString( cmd.c_str() );
	}

	int RunFile( std::string file )
	{
		std::ifstream in( file );
		return RunCmd( { ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() } );
	}

	int get_total_ref_count()
	{
		PyObject* refCount = PyObject_CallObject( PySys_GetObject( ( char* )"gettotalrefcount" ), NULL );
		if ( !refCount ) return -1;
		int ret = _PyLong_AsInt( refCount );
		Py_DECREF( refCount );
		return ret;
	}

	// Static module map map declaration
	std::map<std::string, ModuleDef> ModuleDef::s_mapPyModules;

	// Name and doc constructor
	ModuleDef::ModuleDef( const std::string& moduleName, const std::string& moduleDocs ) :
		m_strModDocs( moduleDocs ),
		m_strModName( moduleName ),
		m_fnCustomInit( []( Object o ) {} )
	{}

	// This is implemented here just to avoid putting these STL calls in the header
	bool ModuleDef::registerClass_impl( const std::type_index T, const std::string& className )
	{
		return m_mapExposedClasses.emplace( T, className ).second;
	}

	// This is implemented here just to avoid putting these STL calls in the header
	bool ModuleDef::registerClass_impl( const std::type_index T, const std::type_index P, const std::string& strClassName, const ModuleDef * const pParentClassMod )
	{
		for ( auto& itClass : pParentClassMod->m_mapExposedClasses )
		{
			if ( itClass.first == P )
			{
				_ExposedClassDef clsDef = itClass.second;
				clsDef.SetName( strClassName );
				return m_mapExposedClasses.emplace( T, clsDef ).second;
			}
		}

		return m_mapExposedClasses.emplace( T, strClassName ).second;
	}

	void checkError( int ret, std::string strErrMsg )
	{
		if ( ret )
			throw pyl::runtime_error( strErrMsg );
	}

	// Implementation of expose object function that doesn't need to be in this header file
	int ModuleDef::exposeObject_impl( const std::type_index T, void * pInstance, const std::string& strName, PyObject * pModule )
	{
		// If we haven't declared the class, we can't expose it
		auto itExpCls = m_mapExposedClasses.find( T );
		if ( itExpCls == m_mapExposedClasses.end() )
			return -1;

		// Make ref to expose class object
		_ExposedClassDef& expCls = itExpCls->second;

		// If a module wasn't specified, just do main
		pModule = pModule ? pModule : PyImport_ImportModule( "__main__" );
		if ( pModule == nullptr )
			return -1;

		// Allocate a new object instance given the PyTypeObject
		PyObject* newPyObject = _PyObject_New( expCls.GetTypeObject() );

		// Make a PyCapsule from the void * to the instance (I'd give it a name, but why?
		PyObject* capsule = PyCapsule_New( pInstance, NULL, NULL );

		// Set the c_ptr member variable (which better exist) to the capsule
		static_cast<_GenericPyClass *>( (void *) newPyObject )->capsule = capsule;

		// Make a variable in the module out of the new py object
		checkError( PyObject_SetAttrString( pModule, strName.c_str(), newPyObject ), "Unable to create variable in module" );

		// decref and return
		Py_DECREF( pModule );
		Py_DECREF( newPyObject );
	}

	// Create the function object invoked when this module is imported
	void ModuleDef::createFnObject()
	{
		// Declare the init function, which gets called on import and returns a PyObject *
		// that represent the module itself (I hate capture this, but it felt necessary)
		m_fnModInit = [this]()
		{
			// The MethodDef contains all functions defined in C++ code,
			// including those called into by exposed classes
			m_pyModDef = PyModuleDef
			{
				PyModuleDef_HEAD_INIT,
				m_strModName.c_str(),
				m_strModDocs.c_str(),
				-1,
				(PyMethodDef *) m_ntMethodDefs.data()
			};

			// Create the module if possible
			if ( PyObject * mod = PyModule_Create( &m_pyModDef ) )
			{
				// Declare all exposed classes within the module
				for ( auto& itExposedClass : m_mapExposedClasses )
				{
					_ExposedClassDef& expCls = itExposedClass.second;

					// Get the classes itExposedClass
					PyTypeObject * pTypeObj = expCls.GetTypeObject();
					if ( expCls.IsPrepared() == false || PyType_Ready( pTypeObj ) < 0 )
						throw pyl::runtime_error( "Error! Exposing class def prematurely!" );

					// Add the type to the module, acting under the assumption that these pointers remain valid
					PyModule_AddObject( mod, expCls.GetName(), (PyObject *) pTypeObj );
				}

				// Call the init function once the module is created
				m_fnCustomInit( { mod } );

				// Return the created module
				return mod;
			}

			// If creating the module failed for whatever reason
			return ( PyObject * )nullptr;
		};
	}

	bool ModuleDef::addMethod_impl( std::string strMethodName, PyCFunction fnPtr, int flags, std::string docs )
	{
		if ( strMethodName.empty() )
		{
			throw std::runtime_error( "Error adding method " + strMethodName );
			return false;
		}

		auto paInsert = m_setUsedMethodNames.insert( strMethodName );
		if ( paInsert.second == false )
		{
			throw std::runtime_error( "Error: Attempting to overwrite exisiting exposed python function" );
			return false;
		}

		const char * pName = paInsert.first->c_str();
		const char * pDocs = docs.empty() ? nullptr : m_liMethodDocs.insert( m_liMethodDocs.end(), docs )->c_str();
		m_ntMethodDefs.push_back( { pName, fnPtr, flags, pDocs } );
		return true;
	}

	// This function locks down any exposed class definitions
	void ModuleDef::prepareClasses()
	{
		// Lock down any definitions
		for ( auto& e_Class : m_mapExposedClasses )
			e_Class.second.Prepare();
	}

	/*static*/ ModuleDef * ModuleDef::GetModuleDef( const std::string moduleName )
	{
		// Return nullptr if we don't have this module
		auto it = s_mapPyModules.find( moduleName );
		if ( it == s_mapPyModules.end() )
			return nullptr;

		// Otherwise return the address of the definition
		return &it->second;
	}

	Object ModuleDef::AsObject() const
	{
		auto it = s_mapPyModules.find( m_strModName );
		if ( it == s_mapPyModules.end() )
			return nullptr;

		PyObject * plMod = PyImport_ImportModule( m_strModName.c_str() );

		return plMod;
	}

	/*static*/ int ModuleDef::InitAllModules()
	{
		for ( auto& module : s_mapPyModules )
		{
			module.second.prepareClasses();
		}
		return 0;
	}

	const char * ModuleDef::getNameBuf() const
	{
		return m_strModName.c_str();
	}

	void ModuleDef::SetCustomModuleInit( std::function<void( Object )> fnCustomInit )
	{
		m_fnCustomInit = fnCustomInit;
	}

	Object GetModule( std::string modName )
	{
		PyObject * pModule = PyImport_ImportModule( modName.c_str() );
		if ( pModule )
			return { pModule };

		return { nullptr };
	}

	Object GetMainModule()
	{
		return GetModule( "__main__" );
	}

	void _PyObjectDeleter::operator()( PyObject * pObj )
	{
		Py_XDECREF( pObj );
	}

	std::string get_tabs( int n )
	{
		// Four spaces...
		const std::string py_tab = "    ";
		std::string ret;
		for ( int i = 0; i < n; i++ )
			ret += py_tab;
		return ret;
	}

	_PyFunc _getPyFunc_Case4( std::function<void()> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			fn();
			Py_INCREF( Py_None );
			return Py_None;
		};
		return pFn;
	}
}
