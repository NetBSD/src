/*	$NetBSD: Lint_sbrk.c,v 1.2 1999/07/12 21:55:19 kleink Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
void *
sbrk(incr)
	intptr_t incr;
{
	return (0);
}
