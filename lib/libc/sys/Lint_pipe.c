/*	$NetBSD: Lint_pipe.c,v 1.1.2.2 1997/11/08 22:02:49 veego Exp $	*/

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
