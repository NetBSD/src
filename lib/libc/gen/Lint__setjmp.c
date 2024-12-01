/* $NetBSD: Lint__setjmp.c,v 1.3 2024/12/01 16:16:56 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
_setjmp(jmp_buf env)
{
	return 0;
}

/*ARGSUSED*/
void
_longjmp(jmp_buf env, int val)
{
}
