/* $NetBSD: Lint_syscall.c,v 1.5 2024/12/01 16:16:57 rillig Exp $ */

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
