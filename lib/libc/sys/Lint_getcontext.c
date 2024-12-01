/*	$NetBSD: Lint_getcontext.c,v 1.3 2024/12/01 16:16:57 rillig Exp $	*/

/*
 * This file placed in the public domain.
 * Klaus Klein, January 26, 1999.
 */

#include <ucontext.h>

/*ARGSUSED*/
int
getcontext(ucontext_t *ucp)
{
	return 0;
}
