/*	$NetBSD: infinity.c,v 1.4 2004/03/05 01:00:53 kleink Exp $	*/

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

	/* HUGE_VALL is the float analog of HUGE_VAL. */
	assert(isinf(HUGE_VALL));

	return 0;
}
