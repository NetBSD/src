/*
 * cabsf() wrapper for hypotf().
 *
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: w_cabsf.c,v 1.3 1999/07/02 15:37:44 simonb Exp $");
#endif

#include <math.h>

struct complex {
	float x;
	float y;
};

float cabsf __P((struct complex));

float
cabsf(z)
	struct complex z;
{
	return hypotf(z.x, z.y);
}
