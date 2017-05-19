/*
 * cabs() wrapper for hypot().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: compat_cabs.c,v 1.2.62.1 2017/05/19 00:22:53 pgoyette Exp $");
#endif

#include "../src/namespace.h"
#include <math.h>

struct complex {
	double x;
	double y;
};

double cabs(struct complex);
__warn_references(cabs, "warning: reference to compatibility cabs()")

double
cabs(struct complex z)
{

	return hypot(z.x, z.y);
}
