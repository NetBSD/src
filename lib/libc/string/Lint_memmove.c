/*	$NetBSD: Lint_memmove.c,v 1.1 1997/11/06 00:52:12 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <string.h>

/*ARGSUSED*/
void *
memmove(dst, src, len)
	void *dst;
	const void *src;
	size_t len;
{
	return (0);
}
