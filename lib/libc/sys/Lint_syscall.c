/* $NetBSD: Lint_syscall.c,v 1.2.4.1 2002/06/21 18:18:24 nathanw Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <stdarg.h>
#include <unistd.h>

/*ARGSUSED*/
int
syscall(int arg1, ...)
{
	return (0);
}
