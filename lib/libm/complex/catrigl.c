/*	$NetBSD: catrigl.c,v 1.3 2022/04/19 20:32:16 rillig Exp $	*/
/*-
 * Copyright (c) 2012 Stephen Montgomery-Smith <stephen@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * The algorithm is very close to that in "Implementing the complex arcsine
 * and arccosine functions using exception handling" by T. E. Hull, Thomas F.
 * Fairgrieve, and Ping Tak Peter Tang, published in ACM Transactions on
 * Mathematical Software, Volume 23 Issue 3, 1997, Pages 299-335,
 * http://dl.acm.org/citation.cfm?id=275324.
 *
 * The code for catrig.c contains complete comments.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: catrigl.c,v 1.3 2022/04/19 20:32:16 rillig Exp $");

#include "namespace.h"
#ifdef __weak_alias
__weak_alias(casinl, _casinl)
#endif
#ifdef __weak_alias
__weak_alias(catanl, _catanl)
#endif


#include <sys/param.h>
#include <complex.h>
#include <float.h>
#include <math.h>
#ifdef notyet // missing log1pl __HAVE_LONG_DOUBLE

#include "math_private.h"

#undef isinf
#define isinf(x)	(fabsl(x) == INFINITY)
#undef isnan
#define isnan(x)	((x) != (x))
#define	raise_inexact()	do { volatile float junk __unused = /*LINTED*/1 + tiny; } while (0)
#undef signbit
#define signbit(x)	(__builtin_signbitl(x)) 

#if __HAVE_LONG_DOUBLE + 0 == 128
// Ok
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
// XXX: Byte order
#define EXT_EXPBITS	15
struct ieee_ext {
	uint64_t ext_frac;
	uint16_t ext_exp:EXT_EXPBITS;
	uint16_t ext_sign:1;
	uint16_t ext_pad;
};
#define extu_exp	extu_ext.ext_exp
#define extu_sign	extu_ext.ext_sign
#define extu_frac	extu_ext.ext_frac
union ieee_ext_u {
	long double extu_ld;
	struct ieee_ext extu_ext;
};
#else
	#error "unsupported long double format"
#endif

#define GET_LDBL_EXPSIGN(r, s) \
    do { \
	    union ieee_ext_u u; \
	    u.extu_ld = s; \
	    r = u.extu_sign; \
	    r >>= EXT_EXPBITS - 1; \
    } while (0)
#define SET_LDBL_EXPSIGN(s, r) \
    do { \
	    union ieee_ext_u u; \
	    u.extu_ld = s; \
	    u.extu_exp &= __BITS(0, EXT_EXPBITS - 1); \
	    u.extu_exp |= (r) << (EXT_EXPBITS - 1); \
	    s = u.extu_ld; \
    } while (0)

static const long double
A_crossover =		10,
B_crossover =		0.6417,
FOUR_SQRT_MIN =		0x1p-8189L,
QUARTER_SQRT_MAX =	0x1p8189L,
RECIP_EPSILON =		1/LDBL_EPSILON,
SQRT_MIN =		0x1p-8191L;

static const long double
m_e =		2.71828182845904523536028747135266250e0L,	/* 0x15bf0a8b1457695355fb8ac404e7a.0p-111 */
m_ln2 =		6.93147180559945309417232121458176568e-1L,	/* 0x162e42fefa39ef35793c7673007e6.0p-113 */
pio2_hi =      1.5707963267948966192313216916397514L, /* pi/2 */
SQRT_3_EPSILON = 2.40370335797945490975336727199878124e-17L,	/*  0x1bb67ae8584caa73b25742d7078b8.0p-168 */
SQRT_6_EPSILON = 3.39934988877629587239082586223300391e-17L;	/*  0x13988e1409212e7d0321914321a55.0p-167 */

static const volatile double
pio2_lo =               6.1232339957367659e-17; /*  0x11a62633145c07.0p-106 */
static const volatile float
tiny =			0x1p-100;

static long double complex clog_for_large_values(long double complex z);

inline static long double
f(long double a, long double b, long double hypot_a_b)
{
	if (b < 0)
		return ((hypot_a_b - b) / 2);
	if (b == 0)
		return (a / 2);
	return (a * a / (hypot_a_b + b) / 2);
}

inline static void
do_hard_work(long double x, long double y, long double *rx, int *B_is_usable, long double *B, long double *sqrt_A2my2, long double *new_y)
{
	long double R, S, A;
	long double Am1, Amy;

	R = hypotl(x, y+1);
	S = hypotl(x, y-1);

	A = (R + S) / 2;
	if (A < 1)
		A = 1;

	if (A < A_crossover) {
		if (y == 1 && x < LDBL_EPSILON*LDBL_EPSILON/128) {
			*rx = sqrtl(x);
		} else if (x >= LDBL_EPSILON * fabsl(y-1)) {
			Am1 = f(x, 1+y, R) + f(x, 1-y, S);
			*rx = log1pl(Am1 + sqrtl(Am1*(A+1)));
		} else if (y < 1) {
			*rx = x/sqrtl((1-y)*(1+y));
		} else {
			*rx = log1pl((y-1) + sqrtl((y-1)*(y+1)));
		}
	} else
		*rx = logl(A + sqrtl(A*A-1));

	*new_y = y;

	if (y < FOUR_SQRT_MIN) {
		*B_is_usable = 0;
		*sqrt_A2my2 = A * (2 / LDBL_EPSILON);
		*new_y= y * (2 / LDBL_EPSILON);
		return;
	}

	*B = y/A;
	*B_is_usable = 1;

	if (*B > B_crossover) {
		*B_is_usable = 0;
		if (y == 1 && x < LDBL_EPSILON/128) {
			*sqrt_A2my2 = sqrtl(x)*sqrtl((A+y)/2);
		} else if (x >= LDBL_EPSILON * fabsl(y-1)) {
			Amy = f(x, y+1, R) + f(x, y-1, S);
			*sqrt_A2my2 = sqrtl(Amy*(A+y));
		} else if (y > 1) {
			*sqrt_A2my2 = x * (4/LDBL_EPSILON/LDBL_EPSILON) * y /
			    sqrtl((y+1)*(y-1));
			*new_y = y * (4/LDBL_EPSILON/LDBL_EPSILON);
		} else {
			*sqrt_A2my2 = sqrtl((1-y)*(1+y));
		}
	}
}

long double complex
casinhl(long double complex z)
{
	long double x, y, ax, ay, rx, ry, B, sqrt_A2my2, new_y;
	int B_is_usable;
	long double complex w;

	x = creall(z);
	y = cimagl(z);
	ax = fabsl(x);
	ay = fabsl(y);

	if (isnan(x) || isnan(y)) {
		if (isinf(x))
			return (CMPLXL(x, y+y));
		if (isinf(y))
			return (CMPLXL(y, x+x));
		if (y == 0) return (CMPLXL(x+x, y));
		return (CMPLXL(x+0.0L+(y+0), x+0.0L+(y+0)));
	}

	if (ax > RECIP_EPSILON || ay > RECIP_EPSILON) {
		if (signbit(x) == 0)
			w = clog_for_large_values(z) + m_ln2;
		else
			w = clog_for_large_values(-z) + m_ln2;
		return (CMPLXL(copysignl(creall(w), x), copysignl(cimagl(w), y)));
	}

	if (x == 0 && y == 0)
		return (z);

	raise_inexact();

	if (ax < SQRT_6_EPSILON/4 && ay < SQRT_6_EPSILON/4)
		return (z);

	do_hard_work(ax, ay, &rx, &B_is_usable, &B, &sqrt_A2my2, &new_y);
	if (B_is_usable)
		ry = asinl(B);
	else
		ry = atan2l(new_y, sqrt_A2my2);
	return (CMPLXL(copysignl(rx, x), copysignl(ry, y)));
}

long double complex
casinl(long double complex z)
{
	long double complex w = casinhl(CMPLXL(cimagl(z), creall(z)));
	return (CMPLXL(cimagl(w), creall(w)));
}

long double complex
cacosl(long double complex z)
{
	long double x, y, ax, ay, rx, ry, B, sqrt_A2mx2, new_x;
	int sx, sy;
	int B_is_usable;
	long double complex w;

	x = creall(z);
	y = cimagl(z);
	sx = signbit(x);
	sy = signbit(y);
	ax = fabsl(x);
	ay = fabsl(y);

	if (isnan(x) || isnan(y)) {
		if (isinf(x))
			return (CMPLXL(y+y, -INFINITY));
		if (isinf(y))
			return (CMPLXL(x+x, -y));
		if (x == 0) return (CMPLXL(pio2_hi + pio2_lo, y+y));
		return (CMPLXL(x+0.0L+(y+0), x+0.0L+(y+0)));
	}

	if (ax > RECIP_EPSILON || ay > RECIP_EPSILON) {
		w = clog_for_large_values(z);
		rx = fabsl(cimagl(w));
		ry = creall(w) + m_ln2;
		if (sy == 0)
			ry = -ry;
		return (CMPLXL(rx, ry));
	}

	if (x == 1 && y == 0)
		return (CMPLXL(0, -y));

	raise_inexact();

	if (ax < SQRT_6_EPSILON/4 && ay < SQRT_6_EPSILON/4)
		return (CMPLXL(pio2_hi - (x - pio2_lo), -y));

	do_hard_work(ay, ax, &ry, &B_is_usable, &B, &sqrt_A2mx2, &new_x);
	if (B_is_usable) {
		if (sx==0)
			rx = acosl(B);
		else
			rx = acosl(-B);
	} else {
		if (sx==0)
			rx = atan2l(sqrt_A2mx2, new_x);
		else
			rx = atan2l(sqrt_A2mx2, -new_x);
	}
	if (sy==0)
		ry = -ry;
	return (CMPLXL(rx, ry));
}

long double complex
cacoshl(long double complex z)
{
	long double complex w;
	long double rx, ry;

	w = cacosl(z);
	rx = creall(w);
	ry = cimagl(w);
	if (isnan(rx) && isnan(ry))
		return (CMPLXL(ry, rx));
	if (isnan(rx))
		return (CMPLXL(fabsl(ry), rx));
	if (isnan(ry))
		return (CMPLXL(ry, ry));
	return (CMPLXL(fabsl(ry), copysignl(rx, cimagl(z))));
}

static long double complex
clog_for_large_values(long double complex z)
{
	long double x, y;
	long double ax, ay, t;

	x = creall(z);
	y = cimagl(z);
	ax = fabsl(x);
	ay = fabsl(y);
	if (ax < ay) {
		t = ax;
		ax = ay;
		ay = t;
	}

	if (ax > LDBL_MAX / 2)
		return (CMPLXL(logl(hypotl(x / m_e, y / m_e)) + 1, atan2l(y, x)));

	if (ax > QUARTER_SQRT_MAX || ay < SQRT_MIN)
		return (CMPLXL(logl(hypotl(x, y)), atan2l(y, x)));

	return (CMPLXL(logl(ax*ax + ay*ay) / 2, atan2l(y, x)));
}

inline static long double
sum_squares(long double x, long double y)
{
	if (y < SQRT_MIN)
		return (x*x);

	return (x*x + y*y);
}

inline static long double
real_part_reciprocal(long double x, long double y)
{
	long double scale;
	uint16_t hx, hy;
	int16_t ix, iy;

	GET_LDBL_EXPSIGN(hx, x);
	ix = hx & 0x7fff;
	GET_LDBL_EXPSIGN(hy, y);
	iy = hy & 0x7fff;
#define	BIAS	(LDBL_MAX_EXP - 1)
#define	CUTOFF	(LDBL_MANT_DIG / 2 + 1)
	if (ix - iy >= CUTOFF || isinf(x))
		return (1/x);
	if (iy - ix >= CUTOFF)
		return (x/y/y);
	if (ix <= BIAS + LDBL_MAX_EXP / 2 - CUTOFF)
		return (x/(x*x + y*y));
	scale = 1;
	SET_LDBL_EXPSIGN(scale, 0x7fff - ix);
	x *= scale;
	y *= scale;
	return (x/(x*x + y*y) * scale);
}

long double complex
catanhl(long double complex z)
{
	long double x, y, ax, ay, rx, ry;

	x = creall(z);
	y = cimagl(z);
	ax = fabsl(x);
	ay = fabsl(y);

	if (y == 0 && ax <= 1)
		return (CMPLXL(atanhl(x), y)); 	/* XXX need atanhl() */

	if (x == 0)
		return (CMPLXL(x, atanl(y)));

	if (isnan(x) || isnan(y)) {
		if (isinf(x))
			return (CMPLXL(copysignl(0, x), y+y));
		if (isinf(y))
			return (CMPLXL(copysignl(0, x), copysignl(pio2_hi + pio2_lo, y)));
		return (CMPLXL(x+0.0L+(y+0), x+0.0L+(y+0)));
	}

	if (ax > RECIP_EPSILON || ay > RECIP_EPSILON)
		return (CMPLXL(real_part_reciprocal(x, y), copysignl(pio2_hi + pio2_lo, y)));

	if (ax < SQRT_3_EPSILON/2 && ay < SQRT_3_EPSILON/2) {
		raise_inexact();
		return (z);
	}

	if (ax == 1 && ay < LDBL_EPSILON) {
#if 0
		if (ay > 2*LDBL_MIN)
			rx = - logl(ay/2) / 2;
		else
#endif
			rx = - (logl(ay) - m_ln2) / 2;
	} else
		rx = log1pl(4*ax / sum_squares(ax-1, ay)) / 4;

	if (ax == 1)
		ry = atan2l(2, -ay) / 2;
	else if (ay < LDBL_EPSILON)
		ry = atan2l(2*ay, (1-ax)*(1+ax)) / 2;
	else
		ry = atan2l(2*ay, (1-ax)*(1+ax) - ay*ay) / 2;

	return (CMPLXL(copysignl(rx, x), copysignl(ry, y)));
}

long double complex
catanl(long double complex z)
{
	long double complex w = catanhl(CMPLXL(cimagl(z), creall(z)));
	return (CMPLXL(cimagl(w), creall(w)));
}

#else
__strong_alias(_casinl, casin)
__strong_alias(_catanl, catan)
__strong_alias(cacoshl, cacosh)
__strong_alias(cacosl, cacos)
__strong_alias(casinhl, casinh)
__strong_alias(catanhl, catanh)
#endif
