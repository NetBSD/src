/*	$NetBSD: infinity.c,v 1.3 2003/10/25 22:38:19 kleink Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * Ben Harris, 2001
 */

/*
 * Check that HUGE_VAL{,FL} (alias __infinity{,fl}) really is infinite.
 * Alternatively, check that isinf() and _isinfl() minimally work,
 * and that floating type promotion respectively demotion works.
 */

#include "namespace.h"
#include <assert.h>
#include <math.h>

int
main(int argc, char **argv)
{

	/* HUGE_VAL is meant to be an infinity. */
	assert(isinf(HUGE_VAL));
	assert(_isinfl(HUGE_VAL));

	/* HUGE_VALF is the float analog of HUGE_VAL. */
	assert(isinf(HUGE_VALF));
	assert(_isinfl(HUGE_VALF));

	/* HUGE_VALL is the float analog of HUGE_VAL. */
	assert(isinf(HUGE_VALL));
	assert(_isinfl(HUGE_VALL));

	return 0;
}
