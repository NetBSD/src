/*	$NetBSD: Lint___vfork14.c,v 1.1 1998/01/04 20:52:09 thorpej Exp $	*/

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
