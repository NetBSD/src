/*	$NetBSD: Lint_strcpy.c,v 1.1 1997/11/06 00:52:28 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <string.h>

/*ARGSUSED*/
char *
strcpy(dst, src)
	char *dst;
	const char *src;
{
	return (0);
}
