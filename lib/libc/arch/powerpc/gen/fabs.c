/*	$NetBSD: fabs.c,v 1.2.2.2 2001/10/08 20:17:54 nathanw Exp $	*/

#include <math.h>

double
fabs(double x)
{
#ifdef _SOFT_FLOAT
	if (x < 0)
		x = -x;
#else
	__asm__ __volatile("fabs %0,%1" : "=f"(x) : "f"(x));
#endif
	return (x);
}
