/*	$NetBSD: Lint_brk.c,v 1.1.2.2 1997/11/08 22:02:52 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
char *
brk(addr)
	const char *addr;
{
	return (0);
}
