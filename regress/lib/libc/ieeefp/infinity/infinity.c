/*	$NetBSD: infinity.c,v 1.1 2001/10/27 23:36:32 bjh21 Exp $	*/

/*
 * This file is in the Public Domain.
 *
 * Ben Harris, 2001
 */

/*
 * Check that HUGE_VAL (alias __infinity) really is infinite.
 * Alternatively, check that isinf() minimally works.
 */

#include <err.h>
#include <math.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{

	/* HUGE_VAL is meant to be an infinity. */
	if (!isinf(HUGE_VAL))
		errx(1, "Infinity isn't infinite");
	return 0;
}
