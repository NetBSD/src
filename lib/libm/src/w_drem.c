/*
 * drem() wrapper for remainder().
 * 
 * Written by J.T. Conklin, <jtc@wimsey.com>
 * Placed into the Public Domain, 1994.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: w_drem.c,v 1.2 1997/10/09 11:34:32 lukem Exp $");
#endif

#include <math.h>

double
drem(x, y)
	double x, y;
{
	return remainder(x, y);
}
