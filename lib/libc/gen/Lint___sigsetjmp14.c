/* $NetBSD: Lint___sigsetjmp14.c,v 1.2.116.1 2025/08/02 05:54:35 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <setjmp.h>

/*ARGSUSED*/
int
__sigsetjmp14(sigjmp_buf env, int savemask)
{
	return 0;
}

/*ARGSUSED*/
void
__siglongjmp14(sigjmp_buf env, int val)
{
}
