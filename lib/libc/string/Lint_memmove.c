/*	$NetBSD: Lint_memmove.c,v 1.1.2.2 1997/11/08 22:01:00 veego Exp $	*/

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
