/*	$NetBSD: _errc.c,v 1.1 2014/01/16 17:21:38 christos Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _errc.c,v 1.1 2014/01/16 17:21:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference)
__indr_reference(_errc, errc)
#else

#include <stdarg.h>

__dead void _verrc(int eval, int code, const char *, va_list);

__dead void
errc(int eval, int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_verr(eval, code, fmt, ap);
	va_end(ap);
}
#endif
