/*	$NetBSD: fabs.c,v 1.2 2005/12/02 09:33:48 skrll Exp $	*/

/*	$OpenBSD: fabs.c,v 1.3 2002/10/21 18:41:05 mickey Exp $	*/

/*
 * Written by Miodrag Vallat.  Public domain
 */

#include <math.h>

double
fabs(double val)
{

	__asm__ __volatile__("fabs,dbl %0,%0" : "+f" (val));
	return (val);
}
