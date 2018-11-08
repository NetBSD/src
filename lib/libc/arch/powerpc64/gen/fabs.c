/*	$NetBSD: fabs.c,v 1.3 2018/11/08 16:31:46 riastradh Exp $	*/

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
