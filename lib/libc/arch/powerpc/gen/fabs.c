/*	$NetBSD: fabs.c,v 1.1 2001/05/25 12:17:45 simonb Exp $	*/

#include <math.h>

double
fabs(double x)
{
#ifdef _SOFT_FLOAT
	if (x < 0)
		x = -x;
#else
	__asm__ __volatile("fabs %1,%0" : "=f" (x) : "f" (x));
#endif
	return (x);
}
