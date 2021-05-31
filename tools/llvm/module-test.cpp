// Test that NDEBUG works
#undef NDEBUG
#include <cassert>
#define NDEBUG
#include <cassert>
int
main()
{
	assert(this code is not compiled);
}

#ifndef _LIBCPP_VERSION
#error "Modules currently requires libc++"
#endif
