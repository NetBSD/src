/* $NetBSD: Lint___syscall.c,v 1.2 2002/05/26 16:53:30 wiz Exp $ */

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
