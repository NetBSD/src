/*	$NetBSD: Lint_sigprocmask.c,v 1.1.2.2 1997/11/08 22:02:54 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <signal.h>

/*ARGSUSED*/
int
sigprocmask(how, set, oset)
	int how;
	const sigset_t *set;
	sigset_t *oset;
{
	return (0);
}
