/*	$NetBSD: Lint_sigsuspend.c,v 1.1.2.2 1997/11/08 22:02:59 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <signal.h>

/*ARGSUSED*/
int
sigsuspend(sigmask)
	const sigset_t *sigmask;
{
	return (0);
}
