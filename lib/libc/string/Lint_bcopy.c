/*	$NetBSD: Lint_bcopy.c,v 1.1 1997/11/06 00:51:51 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <string.h>

/*ARGSUSED*/
void
bcopy(src, dst, len)
	const void *src;
	void *dst;
	size_t len;
{
}
