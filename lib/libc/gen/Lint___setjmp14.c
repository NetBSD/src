/* $NetBSD: Lint___setjmp14.c,v 1.2 2000/06/14 06:49:05 cgd Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
__setjmp14(env)
	jmp_buf env;
{
	return (0);
}

/*ARGSUSED*/
void
__longjmp14(env, val)
	jmp_buf env;
	int val;
{
}
