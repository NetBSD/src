/* $NetBSD: Lint_alloca.c,v 1.1.10.1 2000/06/23 16:17:21 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <stdlib.h>

/*ARGSUSED*/
void *
alloca(size)
	size_t size;
{
	return (0);
}
