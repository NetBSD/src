/*	$NetBSD: Lint_ldiv.c,v 1.1.2.2 1997/11/08 21:59:54 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <stdlib.h>

/*ARGSUSED*/
ldiv_t
ldiv(num, denom)
	long num, denom;
{
	ldiv_t rv = { 0 };
	return (rv);
}
