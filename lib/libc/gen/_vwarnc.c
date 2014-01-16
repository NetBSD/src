/*	$NetBSD: _vwarnc.c,v 1.1 2014/01/16 17:21:38 christos Exp $	*/

/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _vwarnc.c,v 1.1 2014/01/16 17:21:38 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <stdarg.h>

#if defined(__indr_reference)
__indr_reference(_vwarnc, vwarnc)
#else

void _vwarnc(int code, const char *, va_list);

void
vwarnc(int code, const char *fmt, va_list ap)
{
	_vwarnc(code, fmt, ap);
}

#endif
