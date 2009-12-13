/*	$NetBSD: emalloc.c,v 1.1.1.1 2009/12/13 16:55:02 kardel Exp $	*/

/*
 * emalloc - return new memory obtained from the system.  Belch if none.
 */
#include "ntp_types.h"
#include "ntp_malloc.h"
#include "ntp_syslog.h"
#include "ntp_stdlib.h"

extern char *progname;

#if !defined(_MSC_VER) || !defined(_DEBUG)


void *
erealloc(
	void *	prev,
	size_t	size
	)
{
	void *	mem;

	mem = realloc(prev, size ? size : 1);

	if (NULL == mem) {
		msyslog(LOG_ERR,
			"fatal out of memory (%u bytes)", (u_int)size);
		fprintf(stderr,
			"%s: fatal out of memory (%u bytes)", progname,
			(u_int)size);
		exit(1);
	}

	return mem;
}


void *
emalloc(
	size_t	size
	)
{
	return erealloc(NULL, size);
}


char *
estrdup(
	const char *	str
	)
{
	char *	copy;

	copy = strdup(str);

	if (NULL == copy) {
		msyslog(LOG_ERR, 
			"fatal out of memory duplicating %u bytes",
			(u_int)strlen(str) + 1);
		fprintf(stderr, 
			"%s: fatal out of memory duplicating %u bytes",
			progname, (u_int)strlen(str) + 1);
		exit(1);
	}

	return copy;
}

#else /* below is _MSC_VER && _DEBUG */

/*
 * When using the debug MS CRT allocator, each allocation stores the
 * callsite __FILE__ and __LINE__, which is then displayed at process
 * termination, to track down leaks.  We don't want all of our
 * allocations to show up as coming from emalloc.c, so we preserve the
 * original callsite's source file and line using macros which pass
 * __FILE__ and __LINE__ as parameters to these routines.
 */

void *
debug_erealloc(
	void *		prev,
	size_t		size,
	const char *	file,		/* __FILE__ */
	int		line		/* __LINE__ */
	)
{
	void *	mem;

	mem = _realloc_dbg(prev, size ? size : 1,
			   _NORMAL_BLOCK, file, line);

	if (NULL == mem) {
		msyslog(LOG_ERR,
			"fatal: out of memory in %s line %d size %u", 
			file, line, (u_int)size);
		fprintf(stderr,
			"%s: fatal: out of memory in %s line %d size %u", 
			progname, file, line, (u_int)size);
		exit(1);
	}

	return mem;
}

char *
debug_estrdup(
	const char *	str,
	const char *	file,		/* __FILE__ */
	int		line		/* __LINE__ */
	)
{
	char *	copy;
	size_t	bytes;

	bytes = strlen(str) + 1;
	copy = debug_erealloc(NULL, bytes, file, line);
	memcpy(copy, str, bytes);

	return copy;
}

#endif /* _MSC_VER && _DEBUG */
