/*	$NetBSD: Lint_sigsetjmp.c,v 1.1 1997/11/06 00:51:10 cgd Exp $	*/

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
