/* $NetBSD: Lint_brk.c,v 1.3.118.1 2025/08/02 05:54:42 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
int
brk(void *addr)
{
	return 0;
}
