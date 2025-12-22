/* $NetBSD: t_fpclassify.c,v 1.12 2025/12/22 23:06:09 thorpej Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_fpclassify.c,v 1.12 2025/12/22 23:06:09 thorpej Exp $");

#include <sys/endian.h>

#include <atf-c.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* XXX copied from include/fenv.h -- factor me out, please! */
#if \
        (defined(__arm__) && defined(__SOFTFP__)) || \
        (defined(__m68k__) && !defined(__HAVE_68881__)) || \
        defined(__mips_soft_float) || \
        (defined(__powerpc__) && defined(_SOFT_FLOAT)) || \
        (defined(__sh__) && !defined(__SH_FPU_ANY__)) || \
        0
#define	SOFTFLOAT
#endif

/*
 * Some ports use softfloat for long double even if they use hardfloat
 * for other floating-point operations, often if the long double ABI is
 * 128-bit for which essentially no hardware actually exists.
 *
 * XXX Not copied from anywhere -- find a place for me, please!
 */
#if defined SOFTFLOAT || defined __aarch64__ || defined __sparc64__
#define	SOFTFLOAT_LONG_DOUBLE
#endif

#ifdef _FLOAT_IEEE754

#include <machine/ieee.h>

#  if defined __hppa__ || defined __mips__	/* just, why? */
#    define	QNANBIT	0
#    define	SNANBIT	1
#  else
#    define	QNANBIT	1
#    define	SNANBIT	0
#  endif

#  define	FLT_QNANBIT	(QNANBIT*__BIT(FLT_MANT_DIG - 2))
#  define	FLT_SNANBIT	(SNANBIT*__BIT(FLT_MANT_DIG - 2))

#  define	DBL_QNANBIT	(QNANBIT*__BIT(DBL_MANT_DIG - 2))
#  define	DBL_SNANBIT	(SNANBIT*__BIT(DBL_MANT_DIG - 2))

#  define	LDBL_QNANBITH						      \
	(QNANBIT*__BIT(LDBL_MANT_DIG - 2 - EXT_FRACLBITS))
#  define	LDBL_SNANBITH						      \
	(SNANBIT*__BIT(LDBL_MANT_DIG - 2 - EXT_FRACLBITS))

static const char *
formatbitsf(float f)
{
	static char buf[2*sizeof(f) + 1];
	union { float f; uint32_t i; } u = { .f = f };

	__CTASSERT(sizeof(f) <= sizeof(u));
	snprintf(buf, sizeof(buf), "%08"PRIx32, u.i);

	return buf;
}

static const char *
formatbits(double f)
{
	static char buf[2*sizeof(f) + 1];
	union { double f; uint64_t i; } u = { .f = f };

	__CTASSERT(sizeof(f) <= sizeof(u));
	snprintf(buf, sizeof(buf), "%016"PRIx64, u.i);

	return buf;
}

#ifdef __HAVE_LONG_DOUBLE
static const char *
formatbitsl(long double f)
{
	static char buf[2*sizeof(f) + 1];
	union {
		long double f;
		uint64_t i[sizeof(long double)/sizeof(double)];
	} u = { .f = f };
	size_t i, j, n = __arraycount(u.i);

	__CTASSERT(sizeof(f) <= sizeof(u));
#if _BYTE_ORDER == _BIG_ENDIAN
	for (i = j = 0; j < n; i++, j++)
#elif _BYTE_ORDER == _LITTLE_ENDIAN
	for (i = 0, j = n; j --> 0; i++)
#else
#  error Unknown byte order
#endif
		snprintf(buf + 16*i, 16 + 1, "%016"PRIx64, u.i[j]);

	return buf;
}
#endif /* __HAVE_LONG_DOUBLE */

#endif /* _FLOAT_IEEE754 */

#ifdef __HAVE_FENV
#  define	CLEAREXCEPT()	feclearexcept(FE_ALL_EXCEPT)
#  define	CHECKEXCEPT()	do					      \
{									      \
	int _except = fetestexcept(FE_ALL_EXCEPT);			      \
	ATF_CHECK_MSG(_except == 0,					      \
	    "expected no exceptions, got 0x%x", _except);		      \
} while (0)
#else
#  define	CLEAREXCEPT()	__nothing
#  define	CHECKEXCEPT()	__nothing
#endif

#if __STDC_VERSION__ - 0 >= 202311L

#  define	HAVE_ISSIGNALLING

#else  /* __STDC_VERSION__ - 0 < 202311L */

#  ifndef issubnormal
#    define	issubnormal(x)	(fpclassify(x) == FP_SUBNORMAL)
#  endif

#  ifndef iszero
#    define	iszero(x)	(fpclassify(x) == FP_ZERO)
#  endif

#  if !defined issignalling && defined _FLOAT_IEEE754

#    define	HAVE_ISSIGNALLING
#    define	issignalling(x)	__fpmacro_unary_floating(issignalling, x)

static bool
__issignallingf(float f)
{
	union { float f; uint32_t i; } u = { .f = f };

	return isnan(f) && (u.i & (FLT_SNANBIT|FLT_QNANBIT)) == FLT_SNANBIT;
}

static bool
__issignallingd(double f)
{
	union { double f; uint64_t i; } u = { .f = f };

	return isnan(f) && (u.i & (DBL_SNANBIT|DBL_QNANBIT)) == DBL_SNANBIT;
}

#    ifdef __HAVE_LONG_DOUBLE
static bool
__issignallingl(long double f)
{
	union ieee_ext_u u = { .extu_ld = f };

	return isnan(f) &&
	    ((u.extu_frach & (LDBL_SNANBITH|LDBL_QNANBITH)) == LDBL_SNANBITH);
}
#    endif

#  endif	/* !defined issignalling && defined _FLOAT_IEEE754 */

#endif	/* __STDC_VERSION__ < 202311L */

ATF_TC(fpclassify_float);
ATF_TC_HEAD(fpclassify_float, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test float operations");
}

ATF_TC_BODY(fpclassify_float, tc)
{
	volatile float d0, d1, f;
	int e;

	d0 = FLT_MIN;
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnan(d0), "d0=%a", d0);
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(d0), "d0=%a", d0);
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnormal(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(d0), FP_NORMAL,
	    "fpclassify(%a)=%d FP_NORMAL=%d",
	    d0, fpclassify(d0), FP_NORMAL);
	CHECKEXCEPT();
	f = frexpf(d0, &e);
	ATF_CHECK_EQ_MSG(e, FLT_MIN_EXP,
	    "frexpf(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d0, f, e, 0.5, FLT_MIN_EXP);
	ATF_CHECK_EQ_MSG(f, 0.5,
	    "frexpf(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d0, f, e, 0.5, FLT_MIN_EXP);
	d1 = d0;

#if __FLT_HAS_DENORM__
	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (int i = 1; i < FLT_MANT_DIG; i++) {
		float d2, ip;

		d1 /= 2;
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(d1), "d1=%a", d1);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(d1), "d1=%a", d1);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isinf(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(issubnormal(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(d1), FP_SUBNORMAL,
		    "[%d] fpclassify(%a)=%d FP_SUBNORMAL=%d",
		    i, d1, fpclassify(d1), FP_SUBNORMAL);
		CHECKEXCEPT();
		ATF_CHECK_MSG(d1 > 0 && d1 < d0,
		    "[%d] d1=%a d0=%a", i, d1, d0);

		d2 = ldexpf(d0, -i);
		ATF_CHECK_EQ_MSG(d2, d1, "[%d] ldexpf(%a, -%d)=%a != %a",
		    i, d0, i, d2, d1);

		d2 = modff(d1, &ip);
		ATF_CHECK_EQ_MSG(d2, d1,
		    "[%d] modff(%a) returned int %a, frac %a;"
		    " expected int %a, frac %a",
		    i, d1, ip, d2, 0., d1);
		ATF_CHECK_EQ_MSG(ip, 0,
		    "[%d] modff(%a) returned int %a, frac %a;"
		    " expected int %a, frac %a",
		    i, d1, ip, d2, 0., d1);

		f = frexpf(d1, &e);
		ATF_CHECK_EQ_MSG(e, FLT_MIN_EXP - i,
		    "[%d] frexpf(%a) returned normalized %a, exponent %d;"
		    " expected normalized %a, exponent %d",
		    i, d1, f, e, 0.5, FLT_MIN_EXP - i);
		ATF_CHECK_EQ_MSG(f, 0.5,
		    "[%d] frexpf(%a) returned normalized %a, exponent %d;"
		    " expected normalized %a, exponent %d",
		    i, d1, f, e, 0.5, FLT_MIN_EXP - i);
	}
#endif

	d1 /= 2;
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnan(d1), "d1=%a", d1);
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(d1), "d1=%a", d1);
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(iszero(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(d1), FP_ZERO,
	    "fpclassify(%a)=%d FP_ZERO=%d",
	    d1, fpclassify(d1), FP_ZERO);
	CHECKEXCEPT();
	f = frexpf(d1, &e);
	ATF_CHECK_EQ_MSG(e, 0,
	    "frexpf(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d1, f, e, 0., 0);
	ATF_CHECK_EQ_MSG(f, 0,
	    "frexpf(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d1, f, e, 0., 0);

	if (isinf((float)INFINITY)) {
		volatile float inf = INFINITY;

		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(inf), "inf=%a", inf);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(inf), "inf=%a", inf);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(isinf(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issubnormal(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(inf), FP_INFINITE,
		    "fpclassify(%a)=%d FP_INFINITE=%d",
		    inf, fpclassify(inf), FP_INFINITE);
		CHECKEXCEPT();

		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(isinf(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issubnormal(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(-inf), FP_INFINITE,
		    "fpclassify(%a)=%d FP_INFINITE=%d",
		    -inf, fpclassify(inf), FP_INFINITE);
		CHECKEXCEPT();
	} else {
#ifdef _FLOAT_IEEE754
		atf_tc_fail_nonfatal("isinf((float)INFINITY=%a) failed",
		    (float)INFINITY);
#endif
	}

#ifdef NAN
    {
	volatile float nan = NAN;

	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	(void)issignalling(nan); /* could be quiet or signalling */
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%a [0x%s]", nan,
	    formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%a [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsf(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

#ifdef _FLOAT_IEEE754
	union {
		float f;
		uint32_t i;
	} u;

	/* test a quiet NaN */
	u.f = NAN;
	u.i &= ~FLT_SNANBIT;
	u.i |= FLT_QNANBIT;
	u.i |= 1;		/* significand all zero would be inf */
	nan = u.f;
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(nan), "nan=%a [0x%s]", nan,
	    formatbitsf(nan));
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%a [0x%s]", nan,
	    formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%a [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsf(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

	/* verify quiet NaN doesn't provoke exception */
	CLEAREXCEPT();
	volatile float y = nan + nan;
	(void)y;
	CHECKEXCEPT();

	/* test a signalling NaN */
	u.f = NAN;
	u.i &= ~FLT_QNANBIT;
	u.i |= FLT_SNANBIT;
	u.i |= 1;		/* significand all zero would be inf */
	nan = u.f;
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(issignalling(nan), "nan=%a [0x%s]", nan,
	    formatbitsf(nan));
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%a [0x%s]", nan,
	    formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%a [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsf(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

	/* verify signalling NaN does provoke exception */
	CLEAREXCEPT();
#if defined __clang__ && defined SOFTFLOAT
	/*
	 * The softfloat implementation in compiler-rt used by clang
	 * builds has no floating-point exceptions, so any ports with
	 * softfloat ABI will fail this test.
	 */
	atf_tc_expect_fail("PR lib/59853:"
	    " compiler-rt softfloat lacks floating-point exceptions");
#endif
	volatile float z = nan + nan;
	(void)z;
	ATF_CHECK_MSG(fetestexcept(FE_INVALID),
	    "signalling NaN %a [0x%s] failed to raise invalid operation",
	    nan, formatbitsf(nan));
#endif
    }
#endif
}

ATF_TC(fpclassify_double);
ATF_TC_HEAD(fpclassify_double, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test double operations");
}

ATF_TC_BODY(fpclassify_double, tc)
{
	volatile double d0, d1, f;
	int e;

	d0 = DBL_MIN;
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnan(d0), "d0=%a", d0);
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(d0), "d0=%a", d0);
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnormal(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(d0), "d0=%a", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(d0), FP_NORMAL,
	    "fpclassify(%a)=%d FP_NORMAL=%d",
	    d0, fpclassify(d0), FP_NORMAL);
	CHECKEXCEPT();
	f = frexp(d0, &e);
	ATF_CHECK_EQ_MSG(e, DBL_MIN_EXP,
	    "frexp(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d0, f, e, 0.5, DBL_MIN_EXP);
	ATF_CHECK_EQ_MSG(f, 0.5,
	    "frexp(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d0, f, e, 0.5, DBL_MIN_EXP);
	d1 = d0;

#if __DBL_HAS_DENORM__
	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (int i = 1; i < DBL_MANT_DIG; i++) {
		double d2, ip;

		d1 /= 2;
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(d1), "d1=%a", d1);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(d1), "d1=%a", d1);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isinf(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(issubnormal(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(d1), "d1=%a", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(d1), FP_SUBNORMAL,
		    "[%d] fpclassify(%a)=%d FP_SUBNORMAL=%d",
		    i, d1, fpclassify(d1), FP_SUBNORMAL);
		CHECKEXCEPT();
		ATF_CHECK_MSG(d1 > 0 && d1 < d0,
		    "[%d] d1=%a d0=%a", i, d1, d0);

		d2 = ldexp(d0, -i);
		ATF_CHECK_EQ_MSG(d2, d1, "[%d] ldexp(%a, -%d)=%a != %a",
		    i, d0, i, d2, d1);

		d2 = modf(d1, &ip);
		ATF_CHECK_EQ_MSG(d2, d1,
		    "[%d] modf(%a) returned int %a, frac %a;"
		    " expected int %a, frac %a",
		    i, d1, ip, d2, 0., d1);
		ATF_CHECK_EQ_MSG(ip, 0,
		    "[%d] modf(%a) returned int %a, frac %a;"
		    " expected int %a, frac %a",
		    i, d1, ip, d2, 0., d1);

		f = frexp(d1, &e);
		ATF_CHECK_EQ_MSG(e, DBL_MIN_EXP - i,
		    "[%d] frexp(%a) returned normalized %a, exponent %d;"
		    " expected normalized %a, exponent %d",
		    i, d1, f, e, 0.5, DBL_MIN_EXP - i);
		ATF_CHECK_EQ_MSG(f, 0.5,
		    "[%d] frexp(%a) returned normalized %a, exponent %d;"
		    " expected normalized %a, exponent %d",
		    i, d1, f, e, 0.5, DBL_MIN_EXP - i);
	}
#endif

	d1 /= 2;
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnan(d1), "d1=%a", d1);
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(d1), "d1=%a", d1);
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(iszero(d1), "d1=%a", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(d1), FP_ZERO,
	    "fpclassify(%a)=%d FP_ZERO=%d",
	    d1, fpclassify(d1), FP_ZERO);
	CHECKEXCEPT();
	f = frexp(d1, &e);
	ATF_CHECK_EQ_MSG(e, 0,
	    "frexp(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d1, f, e, 0., 0);
	ATF_CHECK_EQ_MSG(f, 0,
	    "frexp(%a) returned normalized %a, exponent %d;"
	    " expected normalized %a, exponent %d",
	    d1, f, e, 0., 0);

	if (isinf(INFINITY)) {
		volatile double inf = INFINITY;

		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(inf), "inf=%a", inf);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(inf), "inf=%a", inf);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(isinf(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issubnormal(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(inf), "inf=%a", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(inf), FP_INFINITE,
		    "fpclassify(%a)=%d FP_INFINITE=%d",
		    inf, fpclassify(inf), FP_INFINITE);
		CHECKEXCEPT();

		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(isinf(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issubnormal(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(-inf), "-inf=%a", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(-inf), FP_INFINITE,
		    "fpclassify(%a)=%d FP_INFINITE=%d",
		    -inf, fpclassify(-inf), FP_INFINITE);
		CHECKEXCEPT();
	} else {
#ifdef _FLOAT_IEEE754
		atf_tc_fail_nonfatal("isinf(INFINITY=%a) failed", INFINITY);
#endif
	}

#ifdef NAN
    {
	volatile double nan = NAN;

	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	(void)issignalling(nan); /* could be quiet or signalling */
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%a [0x%s]", nan,
	    formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%a [0x%s]", nan, formatbitsf(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%a [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsf(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

#ifdef _FLOAT_IEEE754
	union {
		double f;
		uint64_t i;
	} u;

	/* test a quiet NaN */
	u.f = NAN;
	u.i &= ~DBL_SNANBIT;
	u.i |= DBL_QNANBIT;
	u.i |= 1;		/* significand all zero would be inf */
	nan = u.f;
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(nan), "nan=%a [0x%s]", nan,
	    formatbits(nan));
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%a [0x%s]", nan,
	    formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%a [0x%s])=%d FP_NAN=%d",
	    nan, formatbits(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

	/* verify quiet NaN doesn't provoke exception */
	CLEAREXCEPT();
	volatile double y = nan + nan;
	(void)y;
	CHECKEXCEPT();

	/* test a signalling NaN */
	u.f = NAN;
	u.i &= ~DBL_QNANBIT;
	u.i |= DBL_SNANBIT;
	u.i |= 1;		/* significand all zero would be inf */
	nan = u.f;
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(issignalling(nan), "nan=%a [0x%s]", nan,
	    formatbits(nan));
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%a [0x%s]", nan,
	    formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%a [0x%s]", nan, formatbits(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%a [0x%s])=%d FP_NAN=%d",
	    nan, formatbits(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

	/* verify signalling NaN does provoke exception */
#if defined __clang__ && defined SOFTFLOAT
	/*
	 * The softfloat implementation in compiler-rt used by clang
	 * builds has no floating-point exceptions, so any ports with
	 * softfloat ABI will fail this test.
	 */
	atf_tc_expect_fail("PR lib/59853:"
	    " compiler-rt softfloat lacks floating-point exceptions");
#endif
	CLEAREXCEPT();
	volatile double z = nan + nan;
	(void)z;
	ATF_CHECK_MSG(fetestexcept(FE_INVALID),
	    "signalling NaN %a [0x%s] failed to raise invalid operation",
	    nan, formatbits(nan));
#endif
    }
#endif
}

#ifdef __HAVE_LONG_DOUBLE
ATF_TC(fpclassify_long_double);
ATF_TC_HEAD(fpclassify_long_double, tc)
{

	atf_tc_set_md_var(tc, "descr", "Test long double operations");
}

ATF_TC_BODY(fpclassify_long_double, tc)
{
	volatile long double d0, d1, f;
	int e;

	d0 = LDBL_MIN;
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnan(d0), "d0=%La", d0);
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(d0), "d0=%La", d0);
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(d0), "d0=%La", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnormal(d0), "d0=%La", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(d0), "d0=%La", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(d0), "d0=%La", d0);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(d0), FP_NORMAL,
	    "fpclassify(%La)=%d FP_NORMAL=%d",
	    d0, fpclassify(d0), FP_NORMAL);
	CHECKEXCEPT();
	f = frexpl(d0, &e);
	ATF_CHECK_EQ_MSG(e, LDBL_MIN_EXP,
	    "frexpl(%La) returned normalized %La, exponent %d;"
	    " expected normalized %La, exponent %d",
	    d0, f, e, 0.5L, LDBL_MIN_EXP);
	ATF_CHECK_EQ_MSG(f, 0.5,
	    "frexpl(%La) returned normalized %La, exponent %d;"
	    " expected normalized %La, exponent %d",
	    d0, f, e, 0.5L, LDBL_MIN_EXP);
	d1 = d0;

#if __LDBL_HAS_DENORM__
	/* shift a "1" bit through the mantissa (skip the implicit bit) */
	for (int i = 1; i < LDBL_MANT_DIG; i++) {
		long double d2, ip;

		d1 /= 2;
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(d1), "d1=%La", d1);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(d1), "d1=%La", d1);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isinf(d1), "d1=%La", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(d1), "d1=%La", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(issubnormal(d1), "d1=%La", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(d1), "d1=%La", d1);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(d1), FP_SUBNORMAL,
		    "[%d] fpclassify(%La)=%d FP_SUBNORMAL=%d",
		    i, d1, fpclassify(d1), FP_SUBNORMAL);
		CHECKEXCEPT();
		ATF_CHECK_MSG(d1 > 0 && d1 < d0,
		    "[%d] d1=%La d0=%La", i, d1, d0);

		d2 = ldexpl(d0, -i);
		ATF_CHECK_EQ_MSG(d2, d1, "[%d] ldexpl(%La, -%d)=%La != %La",
		    i, d0, i, d2, d1);

		d2 = modfl(d1, &ip);
		ATF_CHECK_EQ_MSG(d2, d1,
		    "[%d] modfl(%La) returned int %La, frac %La;"
		    " expected int %La, frac %La",
		    i, d1, ip, d2, 0.L, d1);
		ATF_CHECK_EQ_MSG(ip, 0,
		    "[%d] modfl(%La) returned int %La, frac %La;"
		    " expected int %La, frac %La",
		    i, d1, ip, d2, 0.L, d1);

		f = frexpl(d1, &e);
		ATF_CHECK_EQ_MSG(e, LDBL_MIN_EXP - i,
		    "[%d] frexpl(%La) returned normalized %La, exponent %d;"
		    " expected normalized %La, exponent %d",
		    i, d1, f, e, 0.5L, LDBL_MIN_EXP - i);
		ATF_CHECK_EQ_MSG(f, 0.5,
		    "[%d] frexpl(%La) returned normalized %La, exponent %d;"
		    " expected normalized %La, exponent %d",
		    i, d1, f, e, 0.5L, LDBL_MIN_EXP - i);
	}
#endif

	d1 /= 2;
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnan(d1), "d1=%La", d1);
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(d1), "d1=%La", d1);
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(d1), "d1=%La", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(d1), "d1=%La", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(d1), "d1=%La", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(iszero(d1), "d1=%La", d1);
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(d1), FP_ZERO,
	    "fpclassify(%La)=%d FP_ZERO=%d",
	    d1, fpclassify(d1), FP_ZERO);
	CHECKEXCEPT();
	f = frexpl(d1, &e);
	ATF_CHECK_EQ_MSG(e, 0,
	    "frexpl(%La) returned normalized %La, exponent %d;"
	    " expected normalized %La, exponent %d",
	    d1, f, e, 0.L, 0);
	ATF_CHECK_EQ_MSG(f, 0,
	    "frexpl(%La) returned normalized %La, exponent %d;"
	    " expected normalized %La, exponent %d",
	    d1, f, e, 0.L, 0);

	if (isinf((long double)INFINITY)) {
		volatile long double inf = INFINITY;

		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(inf), "inf=%La", inf);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(inf), "inf=%La", inf);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(isinf(inf), "inf=%La", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(inf), "inf=%La", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issubnormal(inf), "inf=%La", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(inf), "inf=%La", inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(inf), FP_INFINITE,
		    "fpclassify(%La)=%d FP_INFINITE=%d",
		    inf, fpclassify(inf), FP_INFINITE);
		CHECKEXCEPT();

		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnan(-inf), "-inf=%La", -inf);
		CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issignalling(-inf), "-inf=%La", -inf);
		CHECKEXCEPT();
#endif
		CLEAREXCEPT();
		ATF_CHECK_MSG(isinf(-inf), "-inf=%La", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!isnormal(-inf), "-inf=%La", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!issubnormal(-inf), "-inf=%La", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_MSG(!iszero(-inf), "-inf=%La", -inf);
		CHECKEXCEPT();
		CLEAREXCEPT();
		ATF_CHECK_EQ_MSG(fpclassify(-inf), FP_INFINITE,
		    "fpclassify(%La)=%d FP_INFINITE=%d",
		    -inf, fpclassify(-inf), FP_INFINITE);
		CHECKEXCEPT();
	} else {
#ifdef _FLOAT_IEEE754
		atf_tc_fail_nonfatal("isinf((long double)INFINITY=%La) failed",
		    (long double)INFINITY);
#endif
	}

#ifdef NAN
    {
	volatile long double nan = NAN;

	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	(void)issignalling(nan); /* could be quiet or signalling */
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%La [0x%s]", nan,
	    formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%La [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsl(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

#ifdef _FLOAT_IEEE754
	union ieee_ext_u u;

	/* test a quiet NaN */
	u.extu_ld = NAN;
	u.extu_frach &= ~LDBL_SNANBITH;
	u.extu_frach |= LDBL_QNANBITH;
	u.extu_fracl |= 1;	/* significand all zero would be inf */
	nan = u.extu_ld;
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issignalling(nan), "nan=%La [0x%s]", nan,
	    formatbitsl(nan));
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%La [0x%s]", nan,
	    formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%La [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsl(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

	/* verify quiet NaN doesn't provoke exception */
	CLEAREXCEPT();
	volatile long double y = nan + nan;
	(void)y;
	CHECKEXCEPT();

	/* test a signalling NaN */
	u.extu_ld = NAN;
	u.extu_frach &= ~LDBL_QNANBITH;
	u.extu_frach |= LDBL_SNANBITH;
	u.extu_fracl |= 1;	/* significand all zero would be inf */
	nan = u.extu_ld;
	CLEAREXCEPT();
	ATF_CHECK_MSG(isnan(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
#ifdef HAVE_ISSIGNALLING
	CLEAREXCEPT();
	ATF_CHECK_MSG(issignalling(nan), "nan=%La [0x%s]", nan,
	    formatbitsl(nan));
	CHECKEXCEPT();
#endif
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isinf(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!isnormal(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!issubnormal(nan), "nan=%La [0x%s]", nan,
	    formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_MSG(!iszero(nan), "nan=%La [0x%s]", nan, formatbitsl(nan));
	CHECKEXCEPT();
	CLEAREXCEPT();
	ATF_CHECK_EQ_MSG(fpclassify(nan), FP_NAN,
	    "fpclassify(%La [0x%s])=%d FP_NAN=%d",
	    nan, formatbitsl(nan), fpclassify(nan), FP_NAN);
	CHECKEXCEPT();

	/* verify signalling NaN does provoke exception */
	CLEAREXCEPT();
#if defined __clang__ && defined SOFTFLOAT_LONG_DOUBLE
	/*
	 * The softfloat implementation in compiler-rt used by clang
	 * builds has no floating-point exceptions, so any ports with
	 * softfloat long double ABI will fail this test.
	 */
	atf_tc_expect_fail("PR lib/59853:"
	    " compiler-rt softfloat lacks floating-point exceptions");
#endif
	volatile long double z = nan + nan;
	(void)z;
	ATF_CHECK_MSG(fetestexcept(FE_INVALID),
	    "signalling NaN %La [0x%s] failed to raise invalid operation",
	    nan, formatbitsl(nan));
#endif
    }
#endif
}
#endif /* __HAVE_LONG_DOUBLE */

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, fpclassify_float);
	ATF_TP_ADD_TC(tp, fpclassify_double);
#ifdef __HAVE_LONG_DOUBLE
	ATF_TP_ADD_TC(tp, fpclassify_long_double);
#endif

	return atf_no_error();
}
