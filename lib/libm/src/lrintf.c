/* $NetBSD: lrintf.c,v 1.2 2004/07/01 16:06:30 drochner Exp $ */

#include <math.h>
#include <sys/ieee754.h>
#include <machine/limits.h>
#include "math_private.h"

#ifndef LRINTNAME
#define LRINTNAME lrintf
#define RESTYPE long int
#define RESTYPE_MIN LONG_MIN
#define RESTYPE_MAX LONG_MAX
#endif

#define RESTYPE_BITS (sizeof(RESTYPE) * 8)

static const float
TWO23[2]={
  8.3886080000e+06, /* 0x4b000000 */
 -8.3886080000e+06, /* 0xcb000000 */
};

RESTYPE
LRINTNAME(float x)
{
	u_int32_t i0;
	int e, s, shift;
	RESTYPE res;

	GET_FLOAT_WORD(i0, x);
	e = i0 >> SNG_FRACBITS;
	s = e >> SNG_EXPBITS;
	e = (e & 0xff) - SNG_EXP_BIAS;

	/* 1.0 x 2^-1 is the smallest number which can be rounded to 1 */
	if (e < -1)
		return (0);
	/* 1.0 x 2^31 (or 2^63) is already too large */
	if (e >= (int)RESTYPE_BITS - 1)
		return (s ? RESTYPE_MIN : RESTYPE_MAX); /* ??? unspecified */

	/* >= 2^23 is already an exact integer */
	if (e < SNG_FRACBITS) {
		/* round, using current direction */
		x += TWO23[s];
		x -= TWO23[s];
	}

	GET_FLOAT_WORD(i0, x);
	e = ((i0 >> SNG_FRACBITS) & 0xff) - SNG_EXP_BIAS;
	i0 &= 0x7fffff;
	i0 |= (1 << SNG_FRACBITS);

	shift = e - SNG_FRACBITS;
	if (shift >=0)
		res = (shift < 32 ? (RESTYPE)i0 << shift : 0);
	else
		res = (shift > -32 ? i0 >> -shift : 0);

	return (s ? -res : res);
}
