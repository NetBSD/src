/* $NetBSD: lround.c,v 1.1 2004/07/01 16:09:21 drochner Exp $ */

#include <math.h>
#include <sys/ieee754.h>
#include <machine/limits.h>
#include "math_private.h"

#ifndef LROUNDNAME
#define LROUNDNAME lround
#define RESTYPE long int
#define RESTYPE_MIN LONG_MIN
#define RESTYPE_MAX LONG_MAX
#endif

#define RESTYPE_BITS (sizeof(RESTYPE) * 8)

RESTYPE
LROUNDNAME(double x)
{
	u_int32_t i0, i1;
	int e, s, shift;
	RESTYPE res;

	GET_HIGH_WORD(i0, x);
	e = i0 >> 20;
	s = e >> DBL_EXPBITS;
	e = (e & 0x7ff) - DBL_EXP_BIAS;

	/* 1.0 x 2^-1 is the smallest number which can be rounded to 1 */
	if (e < -1)
		return (0);
	/* 1.0 x 2^31 (or 2^63) is already too large */
	if (e >= (int)RESTYPE_BITS - 1)
		return (s ? RESTYPE_MIN : RESTYPE_MAX); /* ??? unspecified */

	/* >= 2^52 is already an exact integer */
	if (e < DBL_FRACBITS) {
		/* add 0.5, extraction below will truncate */
		x += (s ? -0.5 : 0.5);
	}

	EXTRACT_WORDS(i0, i1, x);
	e = ((i0 >> 20) & 0x7ff) - DBL_EXP_BIAS;
	i0 &= 0xfffff;
	i0 |= (1 << 20);

	shift = e - DBL_FRACBITS;
	if (shift >=0)
		res = (shift < 32 ? (RESTYPE)i1 << shift : 0);
	else
		res = (shift > -32 ? i1 >> -shift : 0);
	shift += 32;
	if (shift >=0)
		res |= (shift < 32 ? (RESTYPE)i0 << shift : 0);
	else
		res |= (shift > -32 ? i0 >> -shift : 0);

	return (s ? -res : res);
}
