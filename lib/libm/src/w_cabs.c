/*
 * cabs() wrapper for hypot().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: w_cabs.c,v 1.3 1999/07/02 15:37:44 simonb Exp $");
#endif

#include <math.h>

struct complex {
	double x;
	double y;
};

double cabs __P((struct complex));

double
cabs(z)
	struct complex z;
{
	return hypot(z.x, z.y);
}
