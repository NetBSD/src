/*
 * cabs() wrapper for hypot().
 * 
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: w_cabs.c,v 1.2 1997/10/09 11:34:19 lukem Exp $");
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
