/*	$NetBSD: Lint_modf.c,v 1.1 1997/11/06 00:51:04 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <math.h>

/*ARGSUSED*/
double
modf(value, iptr)
	double value, *iptr;
{
	return (0.0);
}
