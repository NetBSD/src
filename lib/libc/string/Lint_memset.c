/*	$NetBSD: Lint_memset.c,v 1.1 1997/11/06 00:52:16 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <string.h>

/*ARGSUSED*/
void *
memset(b, c, len)
	void *b;
	int c;
	size_t len;
{
	return (0);
}
