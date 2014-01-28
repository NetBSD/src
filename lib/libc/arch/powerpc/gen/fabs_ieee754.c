/*	$NetBSD: fabs_ieee754.c,v 1.1 2014/01/28 13:47:04 macallan Exp $	*/

#include <math.h>

double
fabs(double x)
{
	if (x < 0)
		x = -x;
	return (x);
}
