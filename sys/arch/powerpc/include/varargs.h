/*	$NetBSD: varargs.h,v 1.4 2000/02/03 16:16:09 kleink Exp $	*/

#ifndef	_PPC_VARARGS_H_
#define	_PPC_VARARGS_H_

#include <machine/ansi.h>
#include <sys/featuretest.h>

#ifndef	_VARARGS_H
#define	_VARARGS_H
#endif
#include <powerpc/va-ppc.h>

typedef	__gnuc_va_list	va_list;

#undef	_BSD_VA_LIST

#if !defined(_ANSI_SOURCE) && \
    (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) || \
     defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) >= 199901L)
#define	va_copy		__va_copy
#endif

#endif /* ! _PPC_VARARGS_H_ */
