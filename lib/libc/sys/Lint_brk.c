/*	$NetBSD: Lint_brk.c,v 1.2 1999/03/10 09:52:54 kleink Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
int
brk(addr)
	void *addr;
{
	return (0);
}
