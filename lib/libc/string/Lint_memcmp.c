/* $NetBSD: Lint_memcmp.c,v 1.1.10.1 2000/06/23 16:59:13 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <string.h>

/*ARGSUSED*/
int
memcmp(b1, b2, len)
	const void *b1, *b2;
	size_t len;
{
	return (0);
}
