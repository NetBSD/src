/*	$NetBSD: stdarg.h,v 1.4 2000/02/03 16:16:08 kleink Exp $	*/

#ifndef	_PPC_STDARG_H_
#define	_PPC_STDARG_H_

#include <machine/ansi.h>
#include <sys/featuretest.h>

#ifndef	_STDARG_H
#define	_STDARG_H
#endif
#include <powerpc/va-ppc.h>

typedef	__gnuc_va_list	va_list;

#if !defined(_ANSI_SOURCE) && \
    (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) || \
     defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L)
#define	va_copy		__va_copy
#endif

#endif /* ! _PPC_STDARG_H_ */
