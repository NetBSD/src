/* $NetBSD: Lint___syscall.c,v 1.3.110.1 2025/08/02 05:54:42 perseant Exp $ */

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
