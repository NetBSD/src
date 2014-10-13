/* $NetBSD: cabsl.c,v 1.1.2.2 2014/10/13 19:34:58 martin Exp $ */

/*
 * Public domain.
 */

#include "../src/namespace.h"
#include <complex.h>
#include <math.h>

long double
cabsl(long double complex z)
{

	return hypotl(__real__ z, __imag__ z);
}
