/*	$NetBSD: Lint_sigpending.c,v 1.1.2.2 1997/11/08 22:02:50 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <signal.h>

/*ARGSUSED*/
int
sigpending(set)
	sigset_t *set;
{
	return (0);
}
