/*
 * cabsf() wrapper for hypotf().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: compat_cabsf.c,v 1.3 2017/05/13 02:58:03 maya Exp $");
#endif

#include "../src/namespace.h"
#include <math.h>

struct complex {
	float x;
	float y;
};

float cabsf __P((struct complex));
__warn_references(cabsf, "warning: reference to compatibility cabsf()")

float
cabsf(struct complex z)
{

	return hypotf(z.x, z.y);
}
