/* $NetBSD: Lint_brk.c,v 1.2.8.1 2000/06/23 16:18:07 minoura Exp $ */

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
