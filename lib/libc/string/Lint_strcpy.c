/*	$NetBSD: Lint_strcpy.c,v 1.1.2.2 1997/11/08 22:01:10 veego Exp $	*/

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
