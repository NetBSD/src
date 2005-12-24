/*	$NetBSD: fabs.c,v 1.3 2005/12/24 21:11:16 perry Exp $	*/

#include <math.h>

double
fabs(double x)
{
#ifdef _SOFT_FLOAT
	if (x < 0)
		x = -x;
#else
	__asm__ volatile("fabs %0,%1" : "=f"(x) : "f"(x));
#endif
	return (x);
}
