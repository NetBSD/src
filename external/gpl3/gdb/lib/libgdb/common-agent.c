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

/*
 * According to the manual page canonicalize_file_name() is equivalent
 * realpath(3) so use that.
 */
extern "C" char *
canonicalize_file_name(const char *fn)
{
	return realpath(fn, NULL);
}
