/* $NetBSD: Lint___syscall.c,v 1.1 2000/12/10 21:27:38 mycroft Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*ARGSUSED*/
quad_t
#ifdef __STDC__
__syscall(quad_t arg1, ...)
#else
__syscall(arg1, va_alist)
        quad_t arg1;
        va_dcl
#endif
{
	return (0);
}
