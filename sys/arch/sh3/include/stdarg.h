/* $NetBSD: stdarg.h,v 1.5 2002/04/28 17:10:37 uch Exp $ */

#ifndef _SH3_STDARG_H_
#define	_SH3_STDARG_H_

#ifdef __lint__

#include <machine/ansi.h>

typedef	_BSD_VA_LIST_		va_list;	/* XXX */

#define	va_start(a, l)		((a) = ((l) ? 0 : 0))
#define	va_arg(a, t)		((a) ? (t)0 : (t)0)
#define	va_end(a)		/* nothing */
#define	__va_copy(d, s)		((d) = (s))

#else /* ! __lint__ */

#ifndef _VARARGS_H
#define	_STDARG_H
#endif

#include <sh3/va-sh.h>
#include <sys/featuretest.h>

typedef __gnuc_va_list va_list;

#endif /* __lint__ */

#if !defined(_ANSI_SOURCE) && \
    (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) || \
     defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L)
#define	va_copy		__va_copy
#endif

#endif /* _SH3_STDARG_H_ */
