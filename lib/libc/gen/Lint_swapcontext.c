/*	$NetBSD: Lint_swapcontext.c,v 1.3 2024/12/01 16:16:56 rillig Exp $	*/

/*
 * This file placed in the public domain.
 * Klaus Klein, November 29, 1998.
 */

#include <ucontext.h>

/*ARGSUSED*/
int
swapcontext(ucontext_t *oucp, const ucontext_t *ucp)
{
	return 0;
}
