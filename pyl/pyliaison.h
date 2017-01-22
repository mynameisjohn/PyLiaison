#include <vector>
#include <set>
#include <list>
#include <map>
#include <string>
#include <functional>
#include <memory>
#include <utility>
#include <typeindex>

#include <Python.h>

/********************************************//*!
\namespace pyl
\brief The namespace in which Pyliaison functionality lives

The Pyliaison (pyl) namespace contains all of the classes and functions
used when facilitating communication between C++ and Python
***********************************************/
namespace pyl
{
	// ----------------- Engine -----------------

	void initialize();
	void finalize();
	bool isInitialized();

	// ----------------- Utility -----------------

	// Generic python ret(args, kwargs) function
	using _PyFunc = std::function<PyObject *( PyObject *, PyObject * )>;

	// Unique pointer with deleter to decrement PyObject ref count
	struct _PyObjectDeleter { void operator()( PyObject *obj ); };
	using unique_ptr = std::unique_ptr<PyObject, _PyObjectDeleter>;

	// Null terminated buffers (kind of a lazy hack...)
	// For method and member definitions in modules/classes
	using _MethodDefs = std::basic_string<PyMethodDef>;
	using _MemberDefs = std::basic_string<PyMemberDef>;

	// Overridden runtime_error class, doesn't do much
	class runtime_error : public std::runtime_error
	{
	public:
		runtime_error( std::string strMessage ) : std::runtime_error( strMessage ) {}
	};

	// Used to get tabs for python code strings
	std::string get_tabs( int n );

	// Invoke some callable object with a std::tuple
	template<typename Func, typename Tup, std::size_t... index>
	decltype( auto ) _invoke_helper( Func&& func, Tup&& tup, std::index_sequence<index...> )
	{
		return func( std::get<index>( std::forward<Tup>( tup ) )... );
	}
	template<typename Func, typename Tup>
	decltype( auto ) _invoke( Func&& func, Tup&& tup )
	{
		constexpr auto Size = std::tuple_size<typename std::decay<Tup>::type>::value;
		return
			_invoke_helper( std::forward<Func>( func ), std::forward<Tup>( tup ), std::make_index_sequence<Size>{} );
	}

	// This was also stolen from stack overflow
	// but I'm hoping to phase it out. It allows me to expose
	// std::functions as function pointers, which python
	// wants for its PyMethodDef buffer (among other things)
	template <typename _UniqueTag, typename _Res, typename... _ArgTypes>
	struct _fun_ptr_helper
	{
	public:
		typedef std::function<_Res( _ArgTypes... )> function_type;

		static void bind( function_type&& f )
		{
			instance().fn_.swap( f );
		}

		static void bind( const function_type& f )
		{
			instance().fn_ = f;
		}

		static _Res invoke( _ArgTypes... args )
		{
			return instance().fn_( args... );
		}

		typedef decltype( &fun_ptr_helper::invoke ) pointer_type;
		static pointer_type ptr()
		{
			return &invoke;
		}

	private:
		static _fun_ptr_helper& instance()
		{
			static _fun_ptr_helper inst_;
			return inst_;
		}

		_fun_ptr_helper() {}

		function_type fn_;
	};

	template <typename _UniqueTag, typename _Res, typename... _ArgTypes>
	typename _fun_ptr_helper<_UniqueTag, _Res, _ArgTypes...>::pointer_type
		_get_fn_ptr( const std::function<_Res( _ArgTypes... )>& f )
	{
		_fun_ptr_helper<_UniqueTag, _Res, _ArgTypes...>::bind( f );
		return _fun_ptr_helper<_UniqueTag, _Res, _ArgTypes...>::ptr();
	}

	template<typename T>
	std::function<typename std::enable_if<std::is_function<T>::value, T>::type>
		_make_function( T *t )
	{
		return { t };
	}

	// TODO
	//rewrite make_function for class member functions

	// Returns the interpreter's total ref count
	int get_total_ref_count();

	// Print the current error set in the interpreter
	void print_error();

	// Clear whatever error the interpreter has set
	void clear_error();

	// Invoke the standard print function on a PyObject
	void print_object( PyObject *obj );

	// ------------ Conversion functions ------------

	/*! convert \brief Convert a PyObject to a string
	Works if the object is a bytes or unicode string*/
	bool convert( PyObject *obj, std::string &val );
	
	/*! convert \brief Convert a PyObject to a char vector
	Works if the object is a bytearray*/
	bool convert( PyObject *obj, std::vector<char> &val );

	/*! convert \brief Convert a PyObject to a bool
	Could be more lenient - only accepts True or False for now*/
	bool convert( PyObject *obj, bool &value );

	/*! convert \brief Convert a PyObject to a double
	Works if the object is a PyFloat or PyLong*/
	bool convert( PyObject *obj, double &val );

	/*! convert \brief Convert a PyObject to a float
	Works if the object is a PyFloat or PyLong*/
	bool convert( PyObject *obj, float &val );

	/*! convert \brief Convert a PyObject to some integral type
	Works if the object is a PyLong*/
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	bool convert( PyObject *obj, T &val )
	{
		if ( !PyLong_Check( obj ) )
			return false;
		val = PyLong_AsLong( obj );
		return true;
	}

	/*! convert_list \brief Convert a PyObject to a generic container type
	The Container type must have a push_back method*/
	template<class T, class C> bool convert_list( PyObject *obj, C &container );

	/*! convert \brief Convert a PyObject to a std::list of type T
	Works if input is a python list and every entry is convertable to T*/
	template<class T> bool convert( PyObject *obj, std::list<T> &lst );

	/*! convert \brief Convert a PyObject to a std::vector of type T
	Works if input is a python list and every entry is convertable to T*/
	template<class T> bool convert( PyObject *obj, std::vector<T> &vec );

	/*! convert \brief Convert a PyObject to an array of type T and size N
	Works if input is a python list and every entry is convertable to T*/
	template<class T> bool convert( PyObject *obj, T * arr, int N );

	/*! convert \brief Convert a PyObject to a std::array of type T and size N
	Works if input is a python list and every entry is convertable to T*/
	template<class T, size_t N> bool convert( PyObject *obj, std::array<T, N>& arr );

	/*! convert \brief Convert a PyObject to a std::map<K, V>
	Works if the input is a dict where each key/value is convertable to K/V*/
	template<class K, class V> bool convert( PyObject *obj, std::map<K, V> &mp );

	/*! convert \brief Convert a PyObject to a std::set<T>
	Works if input is a python set and each element is convertable to T*/
	template<class C> bool convert( PyObject *obj, std::set<C>& s );

	// Used internally
	template<class T> bool _generic_convert( PyObject *obj,
											const std::function<bool( PyObject* )> &is_obj,
											const std::function<T( PyObject* )> &converter,
											T &val );

	/*! convert \brief Convert a PyObject to a pyl::Object
	This allows the use of an opaque python object in C++ code*/
	class Object;
	bool convert( PyObject * obj, pyl::Object& pyObj );

	/*! convert \brief Convert a PyObject to a pointer of type T
	Converts either a capsule or address (integer) to a pointer to some T
	It's up to you to make sure the address coming from python is valid*/
	template<typename T> bool convert( PyObject * obj, T *& val );

	// Add to Tuple functions
	// These recurse to an arbitrary base b
	// and convert objects in a PyTuple to objects in a
	// std::tuple. I added the b parameter because I wanted
	// to set it to 1 and leave the first element alone.

	// Forward declarations
	template<typename First, typename... Rest> void _add_tuple_vars( unique_ptr &tup, const First &head, const Rest&... tail );
	void _add_tuple_vars( unique_ptr &tup, PyObject *arg );
	template<typename Arg> void _add_tuple_vars( unique_ptr &tup, const Arg &arg );
	void _add_tuple_var( unique_ptr &tup, Py_ssize_t i, PyObject *pobj );

	template<typename First, typename... Rest>
	void _add_tuple_vars( unique_ptr &tup, const First &head, const Rest&... tail )
	{
		_add_tuple_var(
			tup,
			PyTuple_Size( tup.get() ) - sizeof...(tail) -1,
			head
		);

		// 
		_add_tuple_vars( tup, tail... );
	}

	void _add_tuple_vars( unique_ptr &tup, PyObject *arg )
	{
		_add_tuple_var( tup, PyTuple_Size( tup.get() ) - 1, arg );
	}

	// Base case for add_tuple_vars
	template<typename Arg>
	void _add_tuple_vars( unique_ptr &tup, const Arg &arg )
	{
		_add_tuple_var( tup,
					   PyTuple_Size( tup.get() ) - 1, alloc_pyobject( arg )
		);
	}

	// Adds a PyObject* to the tuple object
	void _add_tuple_var( unique_ptr &tup, Py_ssize_t i, PyObject *pobj )
	{
		PyTuple_SetItem( tup.get(), i, pobj );
	}

	// Adds a PyObject* to the tuple object
	template<class T> void _add_tuple_var( unique_ptr &tup, Py_ssize_t i, const T &data )
	{
		PyTuple_SetItem( tup.get(), i, alloc_pyobject( data ) );
	}

	// Base case, when n==b, just convert and return
	template<size_t n, size_t b, class... Args>
	typename std::enable_if<n == b, bool>::type
		_add_to_tuple( PyObject *obj, std::tuple<Args...> &tup )
	{
		return convert( PyTuple_GetItem( obj, n - b ), std::get<n>( tup ) );
	}

	// Recurse down to b; note that this can't compile
	// if n <= b because you'll overstep the bounds
	// of the tuple, which is a compile time thing
	template<size_t n, size_t b, class... Args>
	typename std::enable_if<n != b, bool>::type
		_add_to_tuple( PyObject *obj, std::tuple<Args...> &tup )
	{
		_add_to_tuple<n - 1, b, Args...>( obj, tup );
		return convert( PyTuple_GetItem( obj, n - b ), std::get<n>( tup ) );
	}

	// Convert to std::tuple of specified type
	template<class... Args>
	bool convert( PyObject *obj, std::tuple<Args...> &tup )
	{
		if ( !PyTuple_Check( obj ) ||
			 PyTuple_Size( obj ) != sizeof...( Args ) )
			return false;
		return add_to_tuple<sizeof...(Args) -1, 0, Args...>( obj, tup );
	}

	// Convert a PyObject to a std::map
	template<class K, class V>
	bool convert( PyObject *obj, std::map<K, V> &mp )
	{
		if ( !PyDict_Check( obj ) )
			return false;

		// Use this until we succeed
		std::map<K, V> mapRet;

		// Iterate through key/value, convert all
		PyObject *py_key, *py_val;
		Py_ssize_t pos( 0 );
		while ( PyDict_Next( obj, &pos, &py_key, &py_val ) )
		{
			K key;
			if ( !convert( py_key, key ) )
				return false;
			V val;
			if ( !convert( py_val, val ) )
				return false;
			mapRet.emplace( key, val );
		}

		// Assign and return
		mp = std::move( mapRet );
		return true;
	}

	// Convert a PyObject to a std::set
	template<class C>
	bool convert( PyObject *obj, std::set<C>& s )
	{
		if ( !PySet_Check( obj ) )
			return false;

		// Use this until we succeed
		std::set<C> setRet;

		PyObject *iter = PyObject_GetIter( obj );
		PyObject *item = PyIter_Next( iter );
		while ( item )
		{
			C val;
			if ( !convert( item, val ) )
				return false;
			setRet.insert( val );
			item = PyIter_Next( iter );
		}

		s = std::move( setRet );
		return true;
	}

	// Convert a PyObject to a generic container.
	template<class T, class C>
	bool convert_list( PyObject *obj, C &container )
	{
		// Must be a list type
		if ( !PyList_Check( obj ) )
			return false;

		C cRet;
		for ( Py_ssize_t i( 0 ); i < PyList_Size( obj ); ++i )
		{
			T val;
			if ( !convert( PyList_GetItem( obj, i ), val ) )
				return false;
			cRet.push_back( std::move( val ) );
		}

		container = std::move( cRet );
		return true;
	}
	// Convert a PyObject to a std::list.
	template<class T> bool convert( PyObject *obj, std::list<T> &lst )
	{
		return convert_list<T, std::list<T>>( obj, lst );
	}
	// Convert a PyObject to a std::vector.
	template<class T> bool convert( PyObject *obj, std::vector<T> &vec )
	{
		return convert_list<T, std::vector<T>>( obj, vec );
	}

	// Convert a PyObject to a contiguous buffer (very unsafe, but hey)
	template<class T> bool convert( PyObject *obj, T * arr, int N )
	{
		if ( !PyList_Check( obj ) )
			return false;

		// I can't really afford to allocate temporary
		// space here... think of something else
		Py_ssize_t len = PyList_Size( obj );
		if ( len > N ) len = N;
		for ( Py_ssize_t i( 0 ); i < len; ++i )
		{
			T& val = arr[i];
			PyObject * pItem = PyList_GetItem( obj, i );
			if ( !convert( pItem, val ) )
				return false;
		}
		return true;
	}

	// Convert a PyObject to a std::array, safe version of above
	template<class T, size_t N> bool convert( PyObject *obj, std::array<T, N>& arr )
	{
		return convert<T>( obj, arr.data(), int( N ) );
	}

	// Generic convert function used by others
	template<class T> bool _generic_convert( PyObject *obj,
											const std::function<bool( PyObject* )> &is_obj,
											const std::function<T( PyObject* )> &converter,
											T &val )
	{
		if ( !is_obj( obj ) )
			return false;
		val = converter( obj );
		return true;
	}

	// Convert to a pyl::Object; useful if function can unpack it
	class Object;
	bool convert( PyObject * obj, pyl::Object& pyObj );

	// This gets invoked on calls to member functions, which require the instance ptr
	// It may be dangerous, since any pointer type will be interpreted
	// as a PyCObject, but so far it's been useful. To protect yourself from collisions,
	// try and specialize any type that you don't want getting caught in this conversion
	template<typename T>
	bool convert( PyObject * obj, T *& val )
	{
		// Try getting the pointer from the capsule
		T * pRet = static_cast<T *>( PyCapsule_GetPointer( obj, NULL ) );
		if ( pRet )
		{
			val = pRet;
			return true;
		}

		// If that doesn't work, try converting from a size_t
		return convert<size_t>( obj, (size_t&) val );
	}

	// -------------- PyObject allocators ----------------

	/*! alloc_pyobject \brief Convert some integral type T to a PyLong
	This will work for any integer based type (int, char, short, size_t, etc.)*/
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	PyObject *alloc_pyobject( T num );

	/*! alloc_pyobject \brief Converts a string to a (bytes) string
	I may have this convert to unicode instead...*/
	PyObject *alloc_pyobject( const std::string &str );

	/*! alloc_pyobject \brief Creates a PyByteArray from a std::vector<char>*/
	PyObject *alloc_pyobject( const std::vector<char> &val, size_t sz );

	/*! alloc_pyobject \brief Creates a PyByteArray from a std::vector<char>*/
	PyObject *alloc_pyobject( const std::vector<char> &val );

	/*! alloc_pyobject \brief Creates (wide) Pystring from a const char**/
	PyObject *alloc_pyobject( const char *cstr );

	/*! alloc_pyobject \brief Creates a PyBool from a bool*/
	PyObject *alloc_pyobject( bool value );

	/*! alloc_pyobject \brief Creates a PyFloat from a double*/
	PyObject *alloc_pyobject( double num );

	/*! alloc_pyobject \brief Creates a PyFloat from a float*/
	PyObject *alloc_pyobject( float num );

	/*! alloc_pyobject \brief Creates a PyCapsule for unspecified pointer types*/
	template <typename T> PyObject * alloc_pyobject( T * ptr );

	/*! alloc_pyobject \brief Creates a PyList from a std::vector<T>*/
	template<class T> PyObject *alloc_pyobject( const std::vector<T> &container );

	/*! alloc_pyobject \brief Creates a a PyList from a std::list<T>*/
	template<class T> PyObject *alloc_pyobject( const std::list<T> &container );

	/*! alloc_pyobject \brief Creates a a PyDict from a std::map<K, V>*/
	template<class K, class V> PyObject *alloc_pyobject( const std::map<K, V> &container );

	/*! alloc_pyobject \brief Creates a PySet from a std::set<C>*/
	template<class C> PyObject *alloc_pyobject( const std::set<C>& s );

	// Creates a PyObject from any integral type (gets converted to PyLong)
	template<class T, typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
	PyObject *alloc_pyobject( T num )
	{
		return PyLong_FromLong( num );
	}

	// Creates a PyCapsule for unspecified pointer types
	template <typename T>
	PyObject * alloc_pyobject( T * ptr )
	{
		return PyCapsule_New( (voidptr_t) ptr, NULL, NULL );
	}

	// Generic python list allocation
	template<class T> static PyObject *alloc_list( const T &container )
	{
		PyObject *lst( PyList_New( container.size() ) );

		Py_ssize_t i( 0 );
		for ( auto it( container.begin() ); it != container.end(); ++it )
			PyList_SetItem( lst, i++, alloc_pyobject( *it ) );

		return lst;
	}

	// Creates a PyList from a std::vector
	template<class T> PyObject *alloc_pyobject( const std::vector<T> &container )
	{
		return alloc_list( container );
	}

	// Creates a PyList from a std::list
	template<class T> PyObject *alloc_pyobject( const std::list<T> &container )
	{
		return alloc_list( container );
	}

	// Creates a PyDict from a std::map
	template<class T, class K> PyObject *alloc_pyobject( const std::map<T, K> &container )
	{
		PyObject *dict( PyDict_New() );

		for ( auto it( container.begin() ); it != container.end(); ++it )
			PyDict_SetItem( dict,
							alloc_pyobject( it->first ),
							alloc_pyobject( it->second )
			);

		return dict;
	}

	// Creates a PySet from a std::set
	template<class C> PyObject *alloc_pyobject( const std::set<C>& s )
	{
		PyObject * pSet( PySet_New( NULL ) );
		for ( auto& i : s )
		{
			PySet_Add( pSet, alloc_pyobject( i ) );
		}
		return pSet;
	}

	// TODO Unordered sets/maps

	// Defines an exposed class (which is not per instance)
	class _ExposedClassDef
	{
	private:
		std::string m_strClassName;                 /*!< Name of class*/
		_MethodDefs m_ntMethodDefs;                 /*!< Method Def buffer*/
		std::set<std::string> m_setUsedMethodNames; /*!< Set of used method names*/
		std::list<std::string> m_liMethodDocs;      /*!< List of method doc strings*/

		_MemberDefs m_ntMemberDefs;                 /*!< Member Def  buffer*/
		std::set<std::string> m_setUsedMemberNames; /*!< Set of used member names*/
		std::list<std::string> m_liMemberDocs;      /*!< List of member doc strings*/

		PyTypeObject m_TypeObject;                  /*!< Python type object*/

	public:
		// Add a method to a class definition
		bool AddMethod( std::string strMethodName, PyCFunction fnPtr, int flags, std::string docs = "" );

		// Add a member to a class definition
		bool AddMember( std::string strMemberName, int type, int offset, int flags, std::string doc = "" );

		// The PyTypeObject struct has pointer members,
		// and we need to assign them before the class
		// is declared to the interpeter with this function
		void Prepare();
		bool IsPrepared() const;

		// Getters/Setters
		PyTypeObject * GetTypeObject() const;
		const char * GetName() const;
		void SetName( std::string strName );

		_ExposedClassDef();
		_ExposedClassDef( std::string strClassName );
	};

	// Exposed classes need a python constructor, backed by this function
	// Note that this does not construct a C++ class, but rather exposes
	// it in the interpreter as a new python object
	static int PyClsInitFunc( PyObject * self, PyObject * args, PyObject * kwargs );

	// All exposed objects inherit from this python type, which has a capsule
	// member holding a pointer to the original object
	struct _GenericPyClass { PyObject_HEAD PyObject * capsule { nullptr }; };

	// TODO more doxygen!
	// This is the original pywrapper::object... quite the beast
	/**
	* \class Object
	* \brief This class represents a python object.
	*/
	class Object
	{
		PyObject * m_pPyObject;

	public:
		/*!
		\brief Constructs a default python object */
		Object();

		/*!
		\brief Constructs a python object from a PyObject pointer.

		This Object takes ownership of the PyObject* argument. That
		means no Py_INCREF is performed on it.
		\param obj The pointer from which to construct this Object.*/

		/*!
		\brief Copy construct from another pyl Object

		This will incrememnt the reference counter of the
		input object, making it similar to an assignment*/
		Object( PyObject *obj );

		/*!
		\brief Construct from a script file
		Will import a script file into the interpreter and
		construct an object containing it. This, like any object,
		can have its members and functions accessed*/
		Object( std::string strScript );

		Object _call_impl( const std::string strName, PyObject * pArgTup = nullptr );

		/*! call
		\brief Invokes the "__call__" operator of object.name

		This will invoke the "__call__" operator of the object's
		"name" attribute, if one exists, else throws a runtime error*/
		template<typename... Args>
		Object call( const std::string strName, const Args... args )
		{
			// Try to call, clean memory on error
			Object tup;
			try
			{
				// Create the tuple argument
				tup = PyTuple_New( sizeof...( args ) );
				add_tuple_vars( tup, args... );
				return _call_impl( strName, tup.get() );
			}
			// Release memory and pass along
			catch ( pyl::runtime_error e )
			{
				print_error();
				tup.reset();
				throw e;
			}
			return { nullptr };
		}

		/*! call
		\brief Invokes the "__call__" operator of object.name

		This will invoke the "__call__" operator of the object's
		"name" attribute, if one exists, else throws a runtime error*/
		Object call( const std::string strName );

		/*! get_attr
		\brief Returns the attr at strName as a pyl Object

		Returns a reference to the object's attribute with
		the name strName, if one exists, else throws a runtime_error*/
		Object get_attr( const std::string strName );

		/*! get_attr
		\brief Returns the attr at strName as a type T

		Gets the attribute at strName and converts it to a type T
		throws error if not found, returns true of conversion successful*/
		template<typename T>
		bool get_attr( const std::string strName, T& obj )
		{
			Object o;
			try { o = get_attr( strName ); }
			catch ( runtime_error ) { return false; }
			if ( o.get() != nullptr )
				return o.convert( obj );
			return false;
		}

		/*! get_attr
		\brief Returns the attr at strName as a type T

		\param name The name of the attribute
		\return bool indicating whether the attribute exists
		*/
		bool has_attr( const std::string strName );

		/*! set_attr
		\brief Sets the object's member from T

		\param name The name of the attribute
		\return bool indicating that the attribute was assigned successfully
		*/
		template<typename T>
		bool set_attr( const std::string strName, T obj )
		{
			unique_ptr pyObj = alloc_pyobject( obj );
			int success = PyObject_SetAttrString( this->get(), name.c_str(), pyObj );
			return ( success == 0 );
		}

		/*!
		\brief Returns the internal PyObject *

		No reference inc/dec is performed
		\return The PyObject * this Object represents, nullptr if None*/
		PyObject * get() const;// { return m_pPyObject; }

							   /*! convert
							   \brief Attempts to convert this object to a Type T, stored in param
							   \return True or false depending on success of conversion*/
		template<class T>
		bool convert( T &param ) { return convert( this->get(), param ); }

		/*! reset
		\brief Decerements our reference of the PyObject
		This is what happens if a python object goes out of scope*/
		void reset();
	};

	// Pretty ridiculous
	template <typename C>
	static C * _getCapsulePtr( PyObject * pObject )
	{
		if ( pObject )
		{
			_GenericPyClass * pGPC = (_GenericPyClass *) pObject;
			PyObject * pCapsule = pGPC->capsule;
			if ( pCapsule && PyCapsule_CheckExact( pCapsule ) )
			{
				return static_cast<C *>( PyCapsule_GetPointer( pCapsule, NULL ) );
			}
		}

		return nullptr;
	}

	template <typename R, typename ... Args>
	_PyFunc _getPyFunc_Case1( std::function<R( Args... )> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			std::tuple<Args...> tup;
			convert( a, tup );
			R rVal = invoke( fn, tup );

			return alloc_pyobject( rVal );
		};
		return pFn;
	}

	template <typename ... Args>
	_PyFunc _getPyFunc_Case2( std::function<void( Args... )> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			std::tuple<Args...> tup;
			convert( a, tup );
			invoke( fn, tup );

			Py_INCREF( Py_None );
			return Py_None;
		};
		return pFn;
	}

	template <typename R>
	_PyFunc _getPyFunc_Case3( std::function<R()> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			R rVal = fn();
			return alloc_pyobject( rVal );
		};
		return pFn;
	}

	_PyFunc _getPyFunc_Case4( std::function<void()> fn );

	template <typename C, typename R, typename ... Args>
	_PyFunc _getPyFunc_Mem_Case1( std::function<R( Args... )> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			// the first arg is the instance pointer, contained in s
			std::tuple<Args...> tup;
			std::get<0>( tup ) = _getCapsulePtr<C>( s );

			// recurse till the first element, getting args from a
			add_to_tuple<sizeof...(Args) -1, 1, Args...>( a, tup );

			// Invoke function, get retVal
			R rVal = invoke( fn, tup );

			// convert rVal to PyObject, return
			return alloc_pyobject( rVal );
		};
		return pFn;
	}

	template <typename C, typename ... Args>
	_PyFunc _getPyFunc_Mem_Case2( std::function<void( Args... )> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			// the first arg is the instance pointer, contained in s
			std::tuple<Args...> tup;
			std::get<0>( tup ) = _getCapsulePtr<C>( s );

			// recurse till the first element, getting args from a
			add_to_tuple<sizeof...(Args) -1, 1, Args...>( a, tup );

			// invoke function
			invoke( fn, tup );

			// Return None
			Py_INCREF( Py_None );
			return Py_None;
		};
		return pFn;
	}

	template <typename C, typename R>
	_PyFunc _getPyFunc_Mem_Case3( std::function<R( C * )> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			// Nothing special here
			R rVal = fn( _getCapsulePtr<C>( s ) );

			return alloc_pyobject( rVal );
		};
		return pFn;
	}

	template<typename C>
	_PyFunc _getPyFunc_Mem_Case4( std::function<void( C * )> fn )
	{
		_PyFunc pFn = [fn]( PyObject * s, PyObject * a )
		{
			// Nothing special here
			fn( _getCapsulePtr<C>( s ) );

			Py_INCREF( Py_None );
			return Py_None;
		};
		return pFn;
	}

	/********************************************//*!
	pyl::ModuleDef
	\brief A Python module definition, encapsulated in a C++ class

	This class stores and manages a Python module that can be accessed
	from within the python interpreter. The constructor is private,
	and all modules must be added via the public static pyl::Module::Create method
	***********************************************/
	class ModuleDef
	{
	private:
		static std::map<std::string, ModuleDef> s_mapPyModules;

		/*!
		\brief Construct module from name and docs

		The Module constructor is private because any modules that aren't constructed
		and properly registered within the internal module map will not be available to
		the python interpreter when it is initialized
		*/
		ModuleDef( const std::string& moduleName, const std::string& moduleDocs );

		// Private members

		std::map<std::type_index, _ExposedClassDef> m_mapExposedClasses;	/*!< A map of exposable C++ class types */
		std::list<_PyFunc> m_liExposedFunctions;							/*!< A list of exposed c++ functions */

		_MethodDefs m_ntMethodDefs;									/*!< A null terminated MethodDef buffer */
		std::list<std::string> m_liMethodDocs;
		std::set<std::string> m_setUsedMethodNames;

		PyModuleDef m_pyModDef;											/*!< The actual Python module def */
		std::string m_strModDocs;										/*!< The string containing module docs */
		std::string m_strModName;										/*!< The string containing the module name */
		std::function<PyObject *( )> m_fnModInit;						/*!< Function called on import that creates the module*/
		std::function<void( Object )> m_fnCustomInit;

		// Sets up m_fnModInit
		void createFnObject();

		// Implementation of expose object function that doesn't need to be in this header file
		int exposeObject_impl( const std::type_index T, void * pInstance, const std::string& strName, PyObject * pModule );

		// Implementation of RegisterClass that doesn't need to be in this header file
		bool registerClass_impl( const std::type_index T, const  std::string& strClassName );
		bool registerClass_impl( const std::type_index T, const std::type_index P, const std::string& strClassName, const ModuleDef * const pParentMod );

		// Adds a method to the null terminated method def buf
		bool addMethod_impl( std::string strMethodName, PyCFunction fnPtr, int flags, std::string docs );

		// Calls the prepare function on all of our exposed classes
		void prepareClasses();

		// When pyl::ModuleDef::CreateModuleDef is called, the module created is added to the list of builtin modules via PyImport_AppendInittab.
		// The PyImport_AppendInittab function relies on a live char * to the module's name provided by this interface.
		// Doing it any other way, or doing it in such a way that the char * will not remain valid, will prevent your module from
		// being imported (which is why they're stored in a map, where references are not invalidated.)
		const char * getNameBuf() const;

		// Add a new method def fo the Method Definitions of the module
		template <typename tag>
		bool addFunction( const _PyFunc pFn, const std::string methodName, const int methodFlags, const std::string docs )
		{
			// We need to store these where they won't move
			m_liExposedFunctions.push_back( pFn );

			// now make the function pointer (TODO figure out these ids, or do something else)
			PyCFunction fnPtr = get_fn_ptr<tag>( m_liExposedFunctions.back() );

			// You can key the methodName string to a std::function
			if ( addMethod_impl( methodName, fnPtr, methodFlags, docs ) )
				return true;

			m_liExposedFunctions.pop_back();
			return false;
		}

		// Like the above but for member functions of exposed C++ clases
		template <typename tag, class C>
		bool addMemFunction( const std::string methodName, const _PyFunc pFn, const int methodFlags, const std::string docs )
		{
			auto it = m_mapExposedClasses.find( typeid( C ) );
			if ( it == m_mapExposedClasses.end() )
				return false;

			// We need to store these where they won't move
			m_liExposedFunctions.push_back( pFn );

			// now make the function pointer (TODO figure out these ids, or do something else)
			PyCFunction fnPtr = get_fn_ptr<tag>( m_liExposedFunctions.back() );

			// Add function
			if ( it->second.AddMethod( methodName, fnPtr, methodFlags, docs ) )
				return true;

			m_liExposedFunctions.pop_back();
			return false;
		}

		// The public expose APIs
	public:

		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Functions for registering non-member functions
		////////////////////////////////////////////////////////////////////////////////////////////////////

		template <typename tag, typename R, typename ... Args>
		bool RegisterFunction( std::string methodName, std::function<R( Args... )> fn, std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Case1( fn );

			return addFunction<tag>( pFn, methodName, METH_VARARGS, docs );
		}

		template <typename tag, typename ... Args>
		bool RegisterFunction( const std::string methodName, const std::function<void( Args... )> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Case2( fn );

			return addFunction<tag>( pFn, methodName, METH_VARARGS, docs );
		}

		template <typename tag, typename R>
		bool RegisterFunction( std::string methodName, const std::function<R()> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Case3( fn );

			return addFunction<tag>( pFn, methodName, METH_NOARGS, docs );
		}

		template <typename tag>
		bool RegisterFunction( const std::string methodName, const std::function<void()> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Case4( fn );

			return addFunction<tag>( pFn, methodName, METH_NOARGS, docs );
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Functions for registering C++ class member functions
		////////////////////////////////////////////////////////////////////////////////////////////////////

		template <typename C, typename tag, typename R, typename ... Args,
			typename std::enable_if<sizeof...( Args ) != 1, int>::type = 0>
			bool RegisterMemFunction( const std::string methodName, const std::function<R( Args... )> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Mem_Case1<C>( fn );
			return addMemFunction<tag, C>( methodName, pFn, METH_VARARGS, docs );
		}

		template <typename C, typename tag, typename ... Args>
		bool RegisterMemFunction( const std::string methodName, std::function<void( Args... )> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Mem_Case2<C>( fn );
			return addMemFunction<tag, C>( methodName, pFn, METH_VARARGS, docs );
		}

		template <typename C, typename tag, typename R>
		bool RegisterMemFunction( const std::string methodName, std::function<R( C * )> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Mem_Case3<C>( fn );
			return addMemFunction<tag, C>( methodName, pFn, METH_NOARGS, docs );
		}

		template <typename C, typename tag>
		bool RegisterMemFunction( const std::string methodName, const std::function<void( C * )> fn, const std::string docs = "" )
		{
			_PyFunc pFn = _getPyFunc_Mem_Case4<C>( fn );
			return addMemFunction<tag, C>( methodName, pFn, METH_NOARGS, docs );
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Functions for registering C++ types with the module
		////////////////////////////////////////////////////////////////////////////////////////////////////

		template <class C>
		bool RegisterClass( std::string className )
		{
			return registerClass_impl( typeid( C ), className );
		}

		template <class C, class P>
		bool RegisterClass( std::string className, const ModuleDef * const pParentClassMod )
		{
			return registerClass_impl( typeid( C ), typeid( P ), className, pParentClassMod );
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Functions for exposing existing C++ class instances whose types are declared to in the module
		////////////////////////////////////////////////////////////////////////////////////////////////////

		template <class C>
		int Expose_Object( C * instance, const std::string name, PyObject * mod = nullptr )
		{
			// Make sure it's a valid pointer
			if ( !instance )
				return -1;

			// Call the implementation function, which makes sure it's in our type map and creates the PyObject
			return exposeObject_impl( typeid( C ), static_cast<voidptr_t>( instance ), name, mod );
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////
		// Innocent functions
		////////////////////////////////////////////////////////////////////////////////////////////////////

		static ModuleDef * GetModuleDef( const std::string moduleName );

		template <typename tag>
		static ModuleDef * Create( const std::string moduleName, const std::string moduleDocs = "" )
		{
			if ( ModuleDef * pExistingDef = GetModuleDef( moduleName ) )
				return pExistingDef;

			// Add to map
			ModuleDef& mod = s_mapPyModules[moduleName] = ModuleDef( moduleName, moduleDocs );

			// Create an initialize m_fnModInit
			mod.createFnObject();

			// Add this module to the list of builtin modules, and ensure m_fnModInit gets called on import
			int success = PyImport_AppendInittab( mod.getNameBuf(), get_fn_ptr<tag>( mod.m_fnModInit ) );
			if ( success != 0 )
			{
				throw pyl::runtime_error( "Error creating module " + moduleName );
				return nullptr;
			}

			return &mod;
		}

		Object AsObject() const;

		static int InitAllModules();

		void SetCustomModuleInit( std::function<void( Object )> fnInit );

		// Don't ever call this... it isn't even implemented, but some STL containers demand that it exists
		ModuleDef() = default;
	};

	Object GetMainModule();
	Object GetModule( std::string modName );

#define S1(x) #x
#define S2(x) S1(x)

#define CreateMod(strModName)\
	pyl::ModuleDef::Create<struct __st_##strModName>(#strModName)

#define CreateModWithDocs(strModName, strModDocs)\
	pyl::ModuleDef::Create<struct __st_#strModName>(strModName, strModDocs)

#define AddFnToMod(M, F)\
	M->RegisterFunction<struct __st_fn##F>(#F, pyl::_make_function(F))

#define AddMemFnToMod(M, C, F, R, ...)\
	std::function<R(C *, ##__VA_ARGS__)> fn##F = &C::F;\
	M->RegisterMemFunction<C, struct __st_fn##C##F>(#F, fn##F)
}
