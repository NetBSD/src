/*
 * drem() wrapper for remainder().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: w_drem.c,v 1.3 1999/07/02 15:37:44 simonb Exp $");
#endif

#include <math.h>

double
drem(x, y)
	double x, y;
{
	return remainder(x, y);
}
