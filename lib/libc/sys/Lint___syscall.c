/* $NetBSD: Lint___syscall.c,v 1.1.2.1 2002/06/21 18:18:24 nathanw Exp $ */

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
	return (0);
}
