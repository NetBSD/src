/*	$NetBSD: nan.c,v 1.3 2003/10/24 15:54:46 kleink Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * Blatently copied from the infinity test by Ben Harris.
 *
 * Simon Burge, 2001
 */

/*
 * Check that NAN (alias __nanf) really is not-a-number.
 * Alternatively, check that isnan() and _isnanl() minimally work.
 */

#include "namespace.h"
#include <assert.h>
#include <math.h>

int
main(int argc, char **argv)
{

	/* NAN is meant to be a NaN. */
	assert(isnan(NAN));
	assert(_isnanl(NAN));
	return 0;
}
