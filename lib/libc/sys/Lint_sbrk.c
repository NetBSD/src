/* $NetBSD: Lint_sbrk.c,v 1.2.6.1 2000/06/23 16:18:08 minoura Exp $ */

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
