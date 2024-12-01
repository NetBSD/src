/* $NetBSD: Lint_sbrk.c,v 1.4 2024/12/01 16:16:57 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
void *
sbrk(intptr_t incr)
{
	return NULL;
}
