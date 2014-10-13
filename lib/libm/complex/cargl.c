/* $NetBSD: cargl.c,v 1.1.2.2 2014/10/13 19:34:58 martin Exp $ */

/*
 * Public domain.
 */

#include "../src/namespace.h"
#include <complex.h>
#include <math.h>

long double
cargl(long double complex z)
{

	return atan2l(__imag__ z, __real__ z);
}
