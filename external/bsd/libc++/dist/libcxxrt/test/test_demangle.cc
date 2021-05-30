#include "test.h"
#include <cxxabi.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>

template <typename T> void test(const char* expected, int line) {
	const char *mangled = typeid(T).name();
	int status = 0;
	using abi::__cxa_demangle;
	char* demangled = __cxa_demangle(mangled, 0, 0, &status);
	printf("mangled='%s' demangled='%s', status=%d\n", mangled, demangled,
	    status);
	free(demangled);
	TEST_LOC(status == 0, "should be able to demangle", __FILE__, line);
	TEST_LOC(demangled != 0, "should be able to demangle", __FILE__, line);
	if (!demangled) {
		/* Don't dereference NULL in strcmp() */
		return;
	}
	TEST_LOC(strcmp(expected, demangled) == 0, "should be able to demangle",
	    __FILE__, line);
	TEST_LOC(strcmp(mangled, demangled) != 0, "should be able to demangle",
	    __FILE__, line);
}


namespace N {
template<typename T, int U>
class Templated {
	virtual ~Templated() {};
};
}

void test_demangle(void)
{
	using namespace N;
	test<int>("int", __LINE__);
	test<char[4]>("char [4]", __LINE__);
	test<char[]>("char []", __LINE__);
	test<Templated<Templated<long, 7>, 8> >(
	    "N::Templated<N::Templated<long, 7>, 8>", __LINE__);
	test<Templated<void(long), -1> >(
	    "N::Templated<void (long), -1>", __LINE__);
}
