/*	$NetBSD: nan.c,v 1.1 2001/10/28 10:41:56 simonb Exp $	*/

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

#include <err.h>
#include <math.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{

	/* NAN is meant to be an infinity. */
	if (!isnan(NAN))
		errx(1, "NAN is a number");
	return 0;
}
