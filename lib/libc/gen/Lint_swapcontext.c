/*	$NetBSD: Lint_swapcontext.c,v 1.2.110.1 2025/08/02 05:54:36 perseant Exp $	*/

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
