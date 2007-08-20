/* $NetBSD: creal.c,v 1.1 2007/08/20 16:01:36 drochner Exp $ */

/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include <complex.h>

double
creal(double complex z)
{

	return __real__ z;
}
