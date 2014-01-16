/*	$NetBSD: _warnc.c,v 1.1 2014/01/16 17:21:38 christos Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _warnc.c,v 1.1 2014/01/16 17:21:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference)
__indr_reference(_warnc, warnc)
#else

#include <stdarg.h>

void _vwarnc(int, const char *, va_list);

void
warnc(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vwarnc(code, fmt, ap);
	va_end(ap);
}
#endif
