/* $NetBSD: Lint___syscall.c,v 1.4 2024/12/01 16:16:57 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <stdarg.h>
#include <unistd.h>

/*ARGSUSED*/
quad_t
__syscall(quad_t arg1, ...)
{
	return 0;
}
