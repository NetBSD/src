/*	$NetBSD: Lint_getcontext.c,v 1.1.2.1 2001/03/05 23:34:39 nathanw Exp $	*/

/*
 * This file placed in the public domain.
 * Klaus Klein, January 26, 1999.
 */

#include <ucontext.h>

/*ARGSUSED*/
int
getcontext(ucp)
	ucontext_t *ucp;
{

	return (0);
}
