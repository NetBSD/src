/*	$NetBSD: Lint_alloca.c,v 1.1 1997/11/06 00:50:39 cgd Exp $	*/

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
