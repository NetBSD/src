/*	$NetBSD: Lint_modf.c,v 1.1.2.2 1997/11/08 21:58:20 veego Exp $	*/

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
