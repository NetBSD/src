/* $NetBSD: Lint_syscall.c,v 1.4.110.1 2025/08/02 05:54:42 perseant Exp $ */

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
	return 0;
}
