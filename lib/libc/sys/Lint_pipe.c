/* $NetBSD: Lint_pipe.c,v 1.1.10.1 2000/06/23 16:18:07 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
int
pipe(filedes)
	int filedes[2];
{
	return (0);
}
