/* $NetBSD: Lint__setjmp.c,v 1.1.10.1 2000/06/23 16:17:21 minoura Exp $ */

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
