/* $NetBSD: cimag.c,v 1.1 2007/08/20 16:01:34 drochner Exp $ */

/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include <complex.h>

double
cimag(double complex z)
{

	return __imag__ z;
}
