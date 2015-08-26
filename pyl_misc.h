#pragma once

#include <functional>

namespace Python
{
	const std::string py_tab = "    ";

	inline std::string getTabs(int n) {
		std::string ret;
		for (int i = 0; i < n; i++)
			ret += py_tab;
		return ret;
	}

	// implementation details, users never invoke these directly
	namespace detail
	{
		template <typename R, typename F, typename Tuple, bool Done, int Total, int... N>
		struct call_impl
		{
			static R call(F f, Tuple && t)
			{
				return call_impl<R, F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
			}
		};

		template <typename R, typename F, typename Tuple, int Total, int... N>
		struct call_impl<R, F, Tuple, true, Total, N...>
		{
			static R call(F f, Tuple && t)
			{
				return f(std::get<N>(std::forward<Tuple>(t))...);
			}
		};
	}

	// user invokes this
	template <typename R, typename F, typename Tuple>
	R call(F f, Tuple && t)
	{
		typedef typename std::decay<Tuple>::type ttype;
		return detail::call_impl<R, F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
	}






	template <const size_t _UniqueId, typename _Res, typename... _ArgTypes>
	struct fun_ptr_helper
	{
	public:
		typedef std::function<_Res(_ArgTypes...)> function_type;

		static void bind(function_type&& f)
		{
			instance().fn_.swap(f);
		}

		static void bind(const function_type& f)
		{
			instance().fn_ = f;
		}

		static _Res invoke(_ArgTypes... args)
		{
			return instance().fn_(args...);
		}

		typedef decltype(&fun_ptr_helper::invoke) pointer_type;
		static pointer_type ptr()
		{
			return &invoke;
		}

	private:
		static fun_ptr_helper& instance()
		{
			static fun_ptr_helper inst_;
			return inst_;
		}

		fun_ptr_helper() {}

		function_type fn_;
	};

	template <const size_t _UniqueId, typename _Res, typename... _ArgTypes>
	typename fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::pointer_type
		get_fn_ptr(const std::function<_Res(_ArgTypes...)>& f)
	{
		fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::bind(f);
		return fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::ptr();
	}

	template<typename T>
	std::function<typename std::enable_if<std::is_function<T>::value, T>::type>
		make_function(T *t)
	{
		return{ t };
	}

	// TODO
	//rewrite the above for class member functions
}