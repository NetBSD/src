/*	$NetBSD: Lint__setjmp.c,v 1.1.2.2 1997/11/08 21:58:12 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
_setjmp(env)
	jmp_buf env;
{
	return (0);
}

/*ARGSUSED*/
void
_longjmp(env, val)
	jmp_buf env;
	int val;
{
}
