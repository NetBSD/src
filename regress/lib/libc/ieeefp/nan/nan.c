/*	$NetBSD: nan.c,v 1.4 2004/03/05 01:00:53 kleink Exp $	*/

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

	/* NAN is meant to be a NaN. */
	assert(isnan(NAN));
	return 0;
}
