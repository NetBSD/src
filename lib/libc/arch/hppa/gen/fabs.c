/*	$NetBSD: fabs.c,v 1.6 2018/11/08 16:29:50 riastradh Exp $	*/

/*	$OpenBSD: fabs.c,v 1.3 2002/10/21 18:41:05 mickey Exp $	*/

/*
 * Written by Miodrag Vallat.  Public domain
 */

#include <sys/cdefs.h>

#include <math.h>

#ifndef __HAVE_LONG_DOUBLE
__strong_alias(fabsl, fabs)
#endif

double
fabs(double val)
{

	__asm volatile("fabs,dbl %0,%0" : "+f" (val));
	return (val);
}
