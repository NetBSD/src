/* $NetBSD: Lint_syscall.c,v 1.2 2000/06/14 06:49:10 cgd Exp $ */

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
int
#ifdef __STDC__
syscall(int arg1, ...)
#else
syscall(arg1, va_alist)
        int arg1;
        va_dcl
#endif
{
	return (0);
}
