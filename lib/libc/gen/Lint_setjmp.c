/*	$NetBSD: Lint_setjmp.c,v 1.1.2.2 1997/11/08 21:58:23 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
setjmp(env)
	jmp_buf env;
{
	return (0);
}

/*ARGSUSED*/
void
longjmp(env, val)
	jmp_buf env;
	int val;
{
}
