/* $NetBSD: Lint_modf.c,v 1.1.10.1 2000/06/23 16:17:24 minoura Exp $ */

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
