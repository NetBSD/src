/* $NetBSD: conjf.c,v 1.1 2007/08/20 16:01:35 drochner Exp $ */

/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include <complex.h>

float complex
conjf(float complex z)
{

	return ~z;
}
