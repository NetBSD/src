/*	$NetBSD: Lint_fork.c,v 1.1.2.2 1997/11/08 22:02:57 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
pid_t
fork()
{
	return (0);
}
