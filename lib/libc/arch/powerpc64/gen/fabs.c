/*	$NetBSD: fabs.c,v 1.2.84.1 2018/11/26 01:52:11 pgoyette Exp $	*/

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
