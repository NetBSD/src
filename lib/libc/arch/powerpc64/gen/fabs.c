/*	$NetBSD: fabs.c,v 1.2.86.1 2019/06/10 22:05:17 christos Exp $	*/

#include <math.h>

__strong_alias(fabsl, fabs)

double
fabs(double x)
{
#ifdef _SOFT_FLOAT
	if (x < 0)
		x = -x;
#else
	__asm volatile("fabs %0,%1" : "=f"(x) : "f"(x));
#endif
	return (x);
}
