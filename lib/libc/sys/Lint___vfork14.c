/* $NetBSD: Lint___vfork14.c,v 1.4 2024/01/20 14:52:49 christos Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#define __LIBC12_SOURCE__
#include <unistd.h>
#include <compat/include/unistd.h>

/*ARGSUSED*/
pid_t
__vfork14(void)
{
	return (0);
}
