/*	$NetBSD: infinity.c,v 1.5 2006/02/20 17:14:33 drochner Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * Ben Harris, 2001
 */

/*
 * Check that HUGE_VAL{,FL} (alias __infinity{,fl}) really is infinite.
 * Alternatively, check that isinf() minimally works.
 */

#include <assert.h>
#include <math.h>

int
main(int argc, char **argv)
{

	/* HUGE_VAL is meant to be an infinity. */
	assert(isinf(HUGE_VAL));

	/* HUGE_VALF is the float analog of HUGE_VAL. */
	assert(isinf(HUGE_VALF));

	/* HUGE_VALL is the long double analog of HUGE_VAL. */
	assert(isinf(HUGE_VALL));

	return 0;
}
