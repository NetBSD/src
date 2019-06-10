/*	$NetBSD: fabs.c,v 1.4.86.1 2019/06/10 22:05:16 christos Exp $	*/

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
