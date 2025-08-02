/* $NetBSD: Lint__setjmp.c,v 1.2.116.1 2025/08/02 05:54:35 perseant Exp $ */

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
