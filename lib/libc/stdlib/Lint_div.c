/*	$NetBSD: Lint_div.c,v 1.1 1997/11/06 00:51:36 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <stdlib.h>

/*ARGSUSED*/
div_t
div(num, denom)
	int num, denom;
{
	div_t rv = { 0 };
	return (rv);
}
