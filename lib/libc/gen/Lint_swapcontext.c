/*	$NetBSD: Lint_swapcontext.c,v 1.1.2.1 2001/03/05 23:34:38 nathanw Exp $	*/

/*
 * This file placed in the public domain.
 * Klaus Klein, November 29, 1998.
 */

#include <ucontext.h>

/*ARGSUSED*/
int
swapcontext(oucp, ucp)
	ucontext_t *oucp;
	const ucontext_t *ucp;
{

	return (0);
}
