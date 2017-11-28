#include "common/agent.c"

/* XXX
 * Gdb wants to use its internal fnmatch that does wide characters overriding
 * the one from libiberty. This is madness, let's use ours!
 */
extern "C" int
gnu_fnmatch(const char *pattern, const char *string, int flags)
{
	return fnmatch(pattern, string, flags);
}
