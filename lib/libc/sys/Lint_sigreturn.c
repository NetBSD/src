/*	$NetBSD: Lint_sigreturn.c,v 1.1.2.2 1997/11/08 22:02:57 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <signal.h>

/*ARGSUSED*/
int
sigreturn(scp)
	struct sigcontext *scp;
{
	return (0);
}
