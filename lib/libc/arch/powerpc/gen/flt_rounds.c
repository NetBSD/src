/*	$NetBSD: flt_rounds.c,v 1.3 1998/08/09 12:43:33 tsubai Exp $	*/

#include <float.h>

static const int map[] = {
	1,	/* round to nearest */
	0,	/* round to zero */
	2,	/* round to positive infinity */
	3	/* round to negative infinity */
};

int
__flt_rounds()
{
	double tmp;
	int x;

	__asm("mffs %0; stfiwx %0,0,%1" : "=f"(tmp): "b"(&x));
	return map[x & 0x03];
}
