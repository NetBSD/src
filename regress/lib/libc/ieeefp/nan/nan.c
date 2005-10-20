/*	$NetBSD: nan.c,v 1.5 2005/10/20 18:02:52 drochner Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * Blatently copied from the infinity test by Ben Harris.
 *
 * Simon Burge, 2001
 */

/*
 * Check that NAN (alias __nanf) really is not-a-number.
 * Alternatively, check that isnan() minimally works.
 */

#include <assert.h>
#include <math.h>

int
main(int argc, char **argv)
{

	/* NAN is meant to be a (float) NaN. */
	assert(isnan(NAN));
	assert(isnan((double)NAN));
	return 0;
}
