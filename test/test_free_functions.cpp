#include "test.h"
#include <luabind/functor.hpp>
#include <iostream>

namespace
{

	LUABIND_ANONYMOUS_FIX int feedback = 0;

	luabind::functor<int> functor_test;
	
	void set_functor(luabind::functor<int> f)
	{
		functor_test = f;
	}
	
	struct base 
	{
		~base() { feedback = 99; }

		void f()
		{
			feedback = 5;
		}
	};

	void f(int x)
	{
		feedback = x;
	}

	void f(int x, int y)
	{
		feedback = x + y;
	}
	
	int g() { feedback = 3; return 4; }

	base* create_base()
	{
		return new base();
	}

	void test_functor(const luabind::functor<int>& fun)
	{
		feedback = fun(5);
	}

	void test_value_converter(const std::string str)
	{
		feedback = 9;
	}

	void test_pointer_converter(const char* const str)
	{
		feedback = 6;
	}

	struct copy_me
	{
	};

	void take_by_value(copy_me m)
	{
	}

	int function_should_never_be_called(lua_State*)
	{
		feedback = -1;
		return 0;
	}

} // anonymous namespace

namespace luabind { namespace converters
{
	yes_t is_user_defined(by_value<int>);

	int convert_lua_to_cpp(lua_State* L, by_value<int>, int index)
	{
		return static_cast<int>(lua_tonumber(L, index));
	}

	int match_lua_to_cpp(lua_State* L, by_value<int>, int index)
	{
		if (lua_isnumber(L, index)) return 0; else return -1;
	}

	void convert_cpp_to_lua(lua_State* L, const  int& v)
	{
		lua_pushnumber(L, v);
	}

}}

bool test_free_functions()
{
	using namespace luabind;
	{
		lua_State* L = lua_open();
		lua_closer c(L);
		int top = lua_gettop(L);

		open(L);

		lua_pushstring(L, "f");
		lua_pushcclosure(L, &function_should_never_be_called, 0);
		lua_settable(L, LUA_GLOBALSINDEX);

		if (dostring(L, "f()")) return false;
		if (feedback != -1) return false;

		module(L)
		[
			class_<copy_me>("copy_me")
				.def(constructor<>()),
		
			class_<base>("base")
				.def("f", &base::f),


			def("by_value", &take_by_value),
	
			def("f", (void(*)(int)) &f),
			def("f", (void(*)(int, int)) &f),
			def("g", &g),
			def("create", &create_base, adopt(return_value)),
			def("test_functor", &test_functor),
			def("set_functor", &set_functor)
				

#if !(BOOST_MSVC < 1300)
			,
			def("test_value_converter", &test_value_converter),
			def("test_pointer_converter", &test_pointer_converter)
#endif
				
		];

		if (dostring(L, "e = create()")) return false;
		if (dostring(L, "e:f()")) return false;
		if (feedback != 5) return false;

		if (dostring(L, "f(7)")) return false;
		if (feedback != 7) return false;

		if (dostring(L, "f(3, 9)")) return false;
		if (feedback != 12) return false;

		if (dostring(L, "a = g()")) return false;
		if (feedback != 3) return false;
		lua_pushstring(L, "a");
		lua_gettable(L, LUA_GLOBALSINDEX);
		if (lua_tonumber(L, -1) != 4) return false;
		lua_pop(L, 1);

		if (dostring(L, "test_functor(function(x) return x * 10 end)")) return false;
		if (feedback != 50) return false;

		functor<int> test_f(L, "g");
		int a = test_f();
		if (a != 4) return false;
		if (feedback != 3) return false;
		if (dostring(L, "set_functor(nil)")) return false;

		if (top != lua_gettop(L)) return false;

		if (dostring(L, "function lua_create() return create() end")) return false;

		base* ptr = call_function<base*>(L, "lua_create") [ adopt(result) ];

		delete ptr;

#if !(BOOST_MSVC < 1300)
		dostring(L, "test_value_converter('foobar')");
		if (feedback != 9) return false;
		dostring(L, "test_pointer_converter('foobar')");
		if (feedback != 6) return false;
#endif

		if (!dostring2(L, "f('incorrect', 'parameters')")) return false;
		std::cout << lua_tostring(L, -1) << "\n";
		lua_pop(L, 1);

		dostring(L, "function functor_test(a) glob = a\n return 'foobar'\nend");
		functor<std::string> functor_test = object_cast<functor<std::string> >(get_globals(L)["functor_test"]);
		
		std::string str = functor_test(6)[detail::null_type()];
		if (str != "foobar") return false;
		if (object_cast<int>(get_globals(L)["glob"]) != 6) return false;

		functor<std::string> functor_test2 = object_cast<functor<std::string> >(get_globals(L)["functor_test"]);

		if (functor_test != functor_test2) return false;		
	}

	if (feedback != 99) return false;

	return true;
}

