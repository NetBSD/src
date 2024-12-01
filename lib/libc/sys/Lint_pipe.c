/* $NetBSD: Lint_pipe.c,v 1.3 2024/12/01 16:16:57 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
int
pipe(int filedes[2])
{
	return 0;
}
