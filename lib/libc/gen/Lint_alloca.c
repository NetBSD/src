/*	$NetBSD: Lint_alloca.c,v 1.1.2.2 1997/11/08 21:58:16 veego Exp $	*/

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
