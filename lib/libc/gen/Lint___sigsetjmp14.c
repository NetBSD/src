/* $NetBSD: Lint___sigsetjmp14.c,v 1.1.8.1 2000/06/23 16:17:20 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
__sigsetjmp14(env, savemask)
	sigjmp_buf env;
	int savemask;
{
	return (0);
}

/*ARGSUSED*/
void
__siglongjmp14(env, val)
	sigjmp_buf env;
	int val;
{
}
