/* $NetBSD: subnormal.c,v 1.1 2006/02/21 16:36:57 drochner Exp $ */
#include <math.h>
#include <float.h>
#include <assert.h>

#ifdef USE_FLOAT
#define FPTYPE float
#define FPTYPE_MIN FLT_MIN
#define FPTYPE_MIN_EXP FLT_MIN_EXP
#define FPTYPE_MANT_DIG FLT_MANT_DIG
#define LDEXP(d,e) ldexpf(d,e)
#define MODF(d,ip) modff(d,ip)
#define FREXP(d,e) frexpf(d,e)
#else
#ifdef USE_LONGDOUBLE
#define FPTYPE long double
#define FPTYPE_MIN LDBL_MIN
#define FPTYPE_MIN_EXP LDBL_MIN_EXP
#define FPTYPE_MANT_DIG LDBL_MANT_DIG
#define LDEXP(d,e) ldexpl(d,e)
#define MODF(d,ip) modfl(d,ip)
#define FREXP(d,e) frexpl(d,e)
#else
#define FPTYPE double
#define FPTYPE_MIN DBL_MIN
#define FPTYPE_MIN_EXP DBL_MIN_EXP
#define FPTYPE_MANT_DIG DBL_MANT_DIG
#define LDEXP(d,e) ldexp(d,e)
#define MODF(d,ip) modf(d,ip)
#define FREXP(d,e) frexp(d,e)
#endif
#endif

int
main()
{
	FPTYPE d0, d1, d2, f, ip;
	int e, i;

	d0 = FPTYPE_MIN;
	assert(fpclassify(d0) == FP_NORMAL);
	f = FREXP(d0, &e);
	assert(e == FPTYPE_MIN_EXP);
	assert(f == 0.5);
	d1 = d0;

	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (i = 1; i < FPTYPE_MANT_DIG; i++) {
		d1 /= 2;
		assert(fpclassify(d1) == FP_SUBNORMAL);
		assert(d1 > 0 && d1 < d0);

		d2 = LDEXP(d0, -i);
		assert(d2 == d1);
		d2 = MODF(d1, &ip);
		assert(d2 == d1);
		assert(ip == 0);

		f = FREXP(d1, &e);
		assert(e == FPTYPE_MIN_EXP - i);
		assert(f == 0.5);
	}

	d1 /= 2;
	assert(fpclassify(d1) == FP_ZERO);
	f = FREXP(d1, &e);
	assert(e == 0);
	assert(f == 0);

	return (0);
}
