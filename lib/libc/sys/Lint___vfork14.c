/* $NetBSD: Lint___vfork14.c,v 1.1.8.1 2000/06/23 16:18:07 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
pid_t
__vfork14()
{
	return (0);
}
