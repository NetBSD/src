/*	$NetBSD: Lint_sigsetjmp.c,v 1.1.2.2 1997/11/08 21:58:15 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
sigsetjmp(env, savemask)
	sigjmp_buf env;
	int savemask;
{
	return (0);
}

/*ARGSUSED*/
void
siglongjmp(env, val)
	sigjmp_buf env;
	int val;
{
}
