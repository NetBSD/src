/*	$NetBSD: infinity.c,v 1.2 2003/10/24 16:35:08 kleink Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * Ben Harris, 2001
 */

/*
 * Check that HUGE_VAL (alias __infinity) really is infinite.
 * Alternatively, check that isinf() and _isinfl() minimally work.
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
	return 0;
}
