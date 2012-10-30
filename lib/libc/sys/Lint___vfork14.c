/* $NetBSD: Lint___vfork14.c,v 1.2.64.1 2012/10/30 18:59:02 yamt Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
pid_t
__vfork14(void)
{
	return (0);
}
