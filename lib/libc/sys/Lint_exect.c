/*	$NetBSD: Lint_exect.c,v 1.1.2.2 1997/11/08 22:02:55 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
int
exect(path, argv, envp)
	const char *path;
	char * const * argv;
	char * const * envp;
{
	return (0);
}
