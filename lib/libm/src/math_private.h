/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $NetBSD: math_private.h,v 1.32 2024/04/03 04:40:23 kre Exp $
 */

#ifndef _MATH_PRIVATE_H_
#define _MATH_PRIVATE_H_

#include <assert.h>
#include <sys/types.h>

/* The original fdlibm code used statements like:
	n0 = ((*(int*)&one)>>29)^1;		* index of high word *
	ix0 = *(n0+(int*)&x);			* high word of x *
	ix1 = *((1-n0)+(int*)&x);		* low word of x *
   to dig two 32 bit words out of the 64 bit IEEE floating point
   value.  That is non-ANSI, and, moreover, the gcc instruction
   scheduler gets it wrong.  We instead use the following macros.
   Unlike the original code, we determine the endianness at compile
   time, not at run time; I don't see much benefit to selecting
   endianness at run time.  */

/* A union which permits us to convert between a double and two 32 bit
   ints.  */

/*
 * A union which permits us to convert between a double and two 32 bit
 * ints.
 */

#ifdef __arm__
#if defined(__VFP_FP__) || defined(__ARM_EABI__)
#define	IEEE_WORD_ORDER	BYTE_ORDER
#else
#define	IEEE_WORD_ORDER	BIG_ENDIAN
#endif
#else /* __arm__ */
#define	IEEE_WORD_ORDER	BYTE_ORDER
#endif

/* A union which permits us to convert between a long double and
   four 32 bit ints.  */

#if IEEE_WORD_ORDER == BIG_ENDIAN

typedef union
{
  long double value;
  struct {
    u_int32_t mswhi;
    u_int32_t mswlo;
    u_int32_t lswhi;
    u_int32_t lswlo;
  } parts32;
  struct {
    u_int64_t msw;
    u_int64_t lsw;
  } parts64;
} ieee_quad_shape_type;

#endif

#if IEEE_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  long double value;
  struct {
    u_int32_t lswlo;
    u_int32_t lswhi;
    u_int32_t mswlo;
    u_int32_t mswhi;
  } parts32;
  struct {
    u_int64_t lsw;
    u_int64_t msw;
  } parts64;
} ieee_quad_shape_type;

#endif

#if IEEE_WORD_ORDER == BIG_ENDIAN

typedef union
{
  double value;
  struct
  {
    u_int32_t msw;
    u_int32_t lsw;
  } parts;
  struct
  {
    u_int64_t w;
  } xparts;
} ieee_double_shape_type;

#endif

#if IEEE_WORD_ORDER == LITTLE_ENDIAN

typedef union
{
  double value;
  struct
  {
    u_int32_t lsw;
    u_int32_t msw;
  } parts;
  struct
  {
    u_int64_t w;
  } xparts;
} ieee_double_shape_type;

#endif

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

/* Get a 64-bit int from a double. */
#define EXTRACT_WORD64(ix,d)					\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix) = ew_u.xparts.w;						\
} while (0)


/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.parts.msw;						\
} while (0)

/* Get the less significant 32 bit int from a double.  */

#define GET_LOW_WORD(i,d)					\
do {								\
  ieee_double_shape_type gl_u;					\
  gl_u.value = (d);						\
  (i) = gl_u.parts.lsw;						\
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)

/* Set a double from a 64-bit int. */
#define INSERT_WORD64(d,ix)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.xparts.w = (ix);						\
  (d) = iw_u.value;						\
} while (0)


/* Set the more significant 32 bits of a double from an int.  */

#define SET_HIGH_WORD(d,v)					\
do {								\
  ieee_double_shape_type sh_u;					\
  sh_u.value = (d);						\
  sh_u.parts.msw = (v);						\
  (d) = sh_u.value;						\
} while (0)

/* Set the less significant 32 bits of a double from an int.  */

#define SET_LOW_WORD(d,v)					\
do {								\
  ieee_double_shape_type sl_u;					\
  sl_u.value = (d);						\
  sl_u.parts.lsw = (v);						\
  (d) = sl_u.value;						\
} while (0)

/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union
{
  float value;
  u_int32_t word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)					\
do {								\
  ieee_float_shape_type gf_u;					\
  gf_u.value = (d);						\
  (i) = gf_u.word;						\
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)					\
do {								\
  ieee_float_shape_type sf_u;					\
  sf_u.word = (i);						\
  (d) = sf_u.value;						\
} while (0)

#define GET_EXPSIGN(u)						\
  (((u)->extu_sign << EXT_EXPBITS) | (u)->extu_exp)
#define SET_EXPSIGN(u, x)					\
  (u)->extu_exp = (x),						\
  (u)->extu_sign = ((x) >> EXT_EXPBITS)
#define GET_LDBL80_MAN(u)					\
  (((uint64_t)(u)->extu_frach << EXT_FRACLBITS) | (u)->extu_fracl)
#define SET_LDBL80_MAN(u, v)					\
  ((u)->extu_fracl = (v) & ((1ULL << EXT_FRACLBITS) - 1),	\
  (u)->extu_frach = (v) >> EXT_FRACLBITS)


/*
 * Get expsign and mantissa as 16 bit and 64 bit ints from an 80 bit long
 * double.
 */

#define	EXTRACT_LDBL80_WORDS(ix0,ix1,d)				\
do {								\
  union ieee_ext_u ew_u;					\
  ew_u.extu_ld = (d);						\
  (ix0) = GET_EXPSIGN(&ew_u);					\
  (ix1) = GET_LDBL80_MAN(&ew_u);				\
} while (0)

/*
 * Get expsign and mantissa as one 16 bit and two 64 bit ints from a 128 bit
 * long double.
 */

#define	EXTRACT_LDBL128_WORDS(ix0,ix1,ix2,d)			\
do {								\
  union ieee_ext_u ew_u;					\
  ew_u.extu_ld = (d);						\
  (ix0) = GET_EXPSIGN(&ew_u);					\
  (ix1) = ew_u.extu_fracl;					\
  (ix2) = ew_u.extu_frach;					\
} while (0)

/* Get expsign as a 16 bit int from a long double.  */

#define	GET_LDBL_EXPSIGN(i,d)					\
do {								\
  union ieee_ext_u ge_u;					\
  ge_u.extu_ld = (d);						\
  (i) = GET_EXPSIGN(&ge_u);					\
} while (0)

/*
 * Set an 80 bit long double from a 16 bit int expsign and a 64 bit int
 * mantissa.
 */

#define	INSERT_LDBL80_WORDS(d,ix0,ix1)				\
do {								\
  union ieee_ext_u iw_u;					\
  SET_EXPSIGN(&iw_u, ix0);					\
  SET_LDBL80_MAN(&iw_u, ix1);					\
  (d) = iw_u.extu_ld;						\
} while (0)

/*
 * Set a 128 bit long double from a 16 bit int expsign and two 64 bit ints
 * comprising the mantissa.
 */

#define	INSERT_LDBL128_WORDS(d,ix0,ix1,ix2)			\
do {								\
  union ieee_ext_u iw_u;					\
  SET_EXPSIGN(&iw_u, ix0);					\
  iw_u.extu_frach = (ix1);					\
  iw_u.extu_fracl = (ix2);					\
  (d) = iw_u.extu_ld;						\
} while (0)

/* Set expsign of a long double from a 16 bit int.  */

#define	SET_LDBL_EXPSIGN(d,v)					\
do {								\
  union ieee_ext_u se_u;					\
  se_u.extu_ld = (d);						\
  SET_EXPSIGN(&se_u, v);						\
  (d) = se_u.extu_ld;						\
} while (0)

#ifdef __i386__
/* Long double constants are broken on i386. */
#define	LD80C(m, ex, v) {						\
	.extu_fracl = (uint32_t)(__CONCAT(m, ULL)),			\
	.extu_frach = __CONCAT(m, ULL) >> EXT_FRACLBITS,		\
	.extu_exp = (0x3fff + (ex)),					\
	.extu_sign = ((v) < 0),						\
}
#else
/**XXX: the following comment may no longer be true:  kre 20240122 **/
/* The above works on non-i386 too, but we use this to check v. */
#define	LD80C(m, ex, v)	{ .extu_ld = (v), }
#endif

/*
 * Attempt to get strict C99 semantics for assignment with non-C99 compilers.
 */
#if FLT_EVAL_METHOD == 0 || __GNUC__ == 0
#define	STRICT_ASSIGN(type, lval, rval)	((lval) = (rval))
#else
#define	STRICT_ASSIGN(type, lval, rval) do {	\
	volatile type __lval;			\
						\
	if (sizeof(type) >= sizeof(long double))	\
		(lval) = (rval);		\
	else {					\
		__lval = (rval);		\
		(lval) = __lval;		\
	}					\
} while (0)
#endif

/* Support switching the mode to FP_PE if necessary. */
#if defined(__i386__) && !defined(NO_FPSETPREC)

#include <ieeefp.h>

#define	ENTERI() ENTERIT(long double)
#define	ENTERIT(returntype)			\
	returntype __retval;			\
	fp_prec_t __oprec;			\
						\
	if ((__oprec = fpgetprec()) != FP_PE)	\
		fpsetprec(FP_PE)
#define	RETURNI(x) do {				\
	__retval = (x);				\
	if (__oprec != FP_PE)			\
		fpsetprec(__oprec);		\
	RETURNF(__retval);			\
} while (0)
#define	ENTERV()				\
	fp_prec_t __oprec;			\
						\
	if ((__oprec = fpgetprec()) != FP_PE)	\
		fpsetprec(FP_PE)
#define	RETURNV() do {				\
	if (__oprec != FP_PE)			\
		fpsetprec(__oprec);		\
	return;			\
} while (0)
#else
#define	ENTERI()
#define	ENTERIT(x)
#define	RETURNI(x)	RETURNF(x)
#define	ENTERV()
#define	RETURNV()	return
#endif

/* Default return statement if hack*_t() is not used. */
#define      RETURNF(v)      return (v)

/*
 * 2sum gives the same result as 2sumF without requiring |a| >= |b| or
 * a == 0, but is slower.
 */
#define	_2sum(a, b) do {	\
	__typeof(a) __s, __w;	\
				\
	__w = (a) + (b);	\
	__s = __w - (a);	\
	(b) = ((a) - (__w - __s)) + ((b) - __s); \
	(a) = __w;		\
} while (0)

/*
 * 2sumF algorithm.
 *
 * "Normalize" the terms in the infinite-precision expression a + b for
 * the sum of 2 floating point values so that b is as small as possible
 * relative to 'a'.  (The resulting 'a' is the value of the expression in
 * the same precision as 'a' and the resulting b is the rounding error.)
 * |a| must be >= |b| or 0, b's type must be no larger than 'a's type, and
 * exponent overflow or underflow must not occur.  This uses a Theorem of
 * Dekker (1971).  See Knuth (1981) 4.2.2 Theorem C.  The name "TwoSum"
 * is apparently due to Skewchuk (1997).
 *
 * For this to always work, assignment of a + b to 'a' must not retain any
 * extra precision in a + b.  This is required by C standards but broken
 * in many compilers.  The brokenness cannot be worked around using
 * STRICT_ASSIGN() like we do elsewhere, since the efficiency of this
 * algorithm would be destroyed by non-null strict assignments.  (The
 * compilers are correct to be broken -- the efficiency of all floating
 * point code calculations would be destroyed similarly if they forced the
 * conversions.)
 *
 * Fortunately, a case that works well can usually be arranged by building
 * any extra precision into the type of 'a' -- 'a' should have type float_t,
 * double_t or long double.  b's type should be no larger than 'a's type.
 * Callers should use these types with scopes as large as possible, to
 * reduce their own extra-precision and efficiciency problems.  In
 * particular, they shouldn't convert back and forth just to call here.
 */
#ifdef DEBUG
#define	_2sumF(a, b) do {				\
	__typeof(a) __w;				\
	volatile __typeof(a) __ia, __ib, __r, __vw;	\
							\
	__ia = (a);					\
	__ib = (b);					\
	assert(__ia == 0 || fabsl(__ia) >= fabsl(__ib));	\
							\
	__w = (a) + (b);				\
	(b) = ((a) - __w) + (b);			\
	(a) = __w;					\
							\
	/* The next 2 assertions are weak if (a) is already long double. */ \
	assert((long double)__ia + __ib == (long double)(a) + (b));	\
	__vw = __ia + __ib;				\
	__r = __ia - __vw;				\
	__r += __ib;					\
	assert(__vw == (a) && __r == (b));		\
} while (0)
#else /* !DEBUG */
#define	_2sumF(a, b) do {	\
	__typeof(a) __w;	\
				\
	__w = (a) + (b);	\
	(b) = ((a) - __w) + (b); \
	(a) = __w;		\
} while (0)
#endif /* DEBUG */

/*
 * Set x += c, where x is represented in extra precision as a + b.
 * x must be sufficiently normalized and sufficiently larger than c,
 * and the result is then sufficiently normalized.
 *
 * The details of ordering are that |a| must be >= |c| (so that (a, c)
 * can be normalized without extra work to swap 'a' with c).  The details of
 * the normalization are that b must be small relative to the normalized 'a'.
 * Normalization of (a, c) makes the normalized c tiny relative to the
 * normalized a, so b remains small relative to 'a' in the result.  However,
 * b need not ever be tiny relative to 'a'.  For example, b might be about
 * 2**20 times smaller than 'a' to give about 20 extra bits of precision.
 * That is usually enough, and adding c (which by normalization is about
 * 2**53 times smaller than a) cannot change b significantly.  However,
 * cancellation of 'a' with c in normalization of (a, c) may reduce 'a'
 * significantly relative to b.  The caller must ensure that significant
 * cancellation doesn't occur, either by having c of the same sign as 'a',
 * or by having |c| a few percent smaller than |a|.  Pre-normalization of
 * (a, b) may help.
 *
 * This is a variant of an algorithm of Kahan (see Knuth (1981) 4.2.2
 * exercise 19).  We gain considerable efficiency by requiring the terms to
 * be sufficiently normalized and sufficiently increasing.
 */
#define	_3sumF(a, b, c) do {	\
	__typeof(a) __tmp;	\
				\
	__tmp = (c);		\
	_2sumF(__tmp, (a));	\
	(b) += (a);		\
	(a) = __tmp;		\
} while (0)

/*
 * Common routine to process the arguments to nan(), nanf(), and nanl().
 */
void _scan_nan(uint32_t *__words, int __num_words, const char *__s);

/*
 * Mix 0, 1 or 2 NaNs.  First add 0 to each arg.  This normally just turns
 * signaling NaNs into quiet NaNs by setting a quiet bit.  We do this
 * because we want to never return a signaling NaN, and also because we
 * don't want the quiet bit to affect the result.  Then mix the converted
 * args using the specified operation.
 *
 * When one arg is NaN, the result is typically that arg quieted.  When both
 * args are NaNs, the result is typically the quietening of the arg whose
 * mantissa is largest after quietening.  When neither arg is NaN, the
 * result may be NaN because it is indeterminate, or finite for subsequent
 * construction of a NaN as the indeterminate 0.0L/0.0L.
 *
 * Technical complications: the result in bits after rounding to the final
 * precision might depend on the runtime precision and/or on compiler
 * optimizations, especially when different register sets are used for
 * different precisions.  Try to make the result not depend on at least the
 * runtime precision by always doing the main mixing step in long double
 * precision.  Try to reduce dependencies on optimizations by adding the
 * the 0's in different precisions (unless everything is in long double
 * precision).
 */
#define	nan_mix(x, y)		(nan_mix_op((x), (y), +))
#define	nan_mix_op(x, y, op)	(((x) + 0.0L) op ((y) + 0))

#ifdef	_COMPLEX_H

/*
 * Quoting from ISO/IEC 9899:TC2:
 *
 * 6.2.5.13 Types
 * Each complex type has the same representation and alignment requirements as
 * an array type containing exactly two elements of the corresponding real type;
 * the first element is equal to the real part, and the second element to the
 * imaginary part, of the complex number.
 */
typedef union {
	float complex z;
	float parts[2];
} float_complex;

typedef union {
	double complex z;
	double parts[2];
} double_complex;

typedef union {
	long double complex z;
	long double parts[2];
} long_double_complex;

#define	REAL_PART(z)	((z).parts[0])
#define	IMAG_PART(z)	((z).parts[1])

/*
 * Inline functions that can be used to construct complex values.
 *
 * The C99 standard intends x+I*y to be used for this, but x+I*y is
 * currently unusable in general since gcc introduces many overflow,
 * underflow, sign and efficiency bugs by rewriting I*y as
 * (0.0+I)*(y+0.0*I) and laboriously computing the full complex product.
 * In particular, I*Inf is corrupted to NaN+I*Inf, and I*-0 is corrupted
 * to -0.0+I*0.0.
 *
 * The C11 standard introduced the macros CMPLX(), CMPLXF() and CMPLXL()
 * to construct complex values.  Compilers that conform to the C99
 * standard require the following functions to avoid the above issues.
 */

#ifndef CMPLXF
static __inline float complex
CMPLXF(float x, float y)
{
	float_complex z;

	REAL_PART(z) = x;
	IMAG_PART(z) = y;
	return (z.z);
}
#endif

#ifndef CMPLX
static __inline double complex
CMPLX(double x, double y)
{
	double_complex z;

	REAL_PART(z) = x;
	IMAG_PART(z) = y;
	return (z.z);
}
#endif

#ifndef CMPLXL
static __inline long double complex
CMPLXL(long double x, long double y)
{
	long_double_complex z;

	REAL_PART(z) = x;
	IMAG_PART(z) = y;
	return (z.z);
}
#endif

#endif	/* _COMPLEX_H */

/* ieee style elementary functions */
extern double __ieee754_sqrt __P((double));
extern double __ieee754_acos __P((double));
extern double __ieee754_acosh __P((double));
extern double __ieee754_log __P((double));
extern double __ieee754_atanh __P((double));
extern double __ieee754_asin __P((double));
extern double __ieee754_atan2 __P((double,double));
extern double __ieee754_exp __P((double));
extern double __ieee754_cosh __P((double));
extern double __ieee754_fmod __P((double,double));
extern double __ieee754_pow __P((double,double));
extern double __ieee754_lgamma_r __P((double,int *));
extern double __ieee754_gamma_r __P((double,int *));
extern double __ieee754_lgamma __P((double));
extern double __ieee754_gamma __P((double));
extern double __ieee754_log10 __P((double));
extern double __ieee754_log2 __P((double));
extern double __ieee754_sinh __P((double));
extern double __ieee754_hypot __P((double,double));
extern double __ieee754_j0 __P((double));
extern double __ieee754_j1 __P((double));
extern double __ieee754_y0 __P((double));
extern double __ieee754_y1 __P((double));
extern double __ieee754_jn __P((int,double));
extern double __ieee754_yn __P((int,double));
extern double __ieee754_remainder __P((double,double));
extern int32_t __ieee754_rem_pio2 __P((double,double*));
extern double __ieee754_scalb __P((double,double));

/* fdlibm kernel function */
extern double __kernel_standard __P((double,double,int));
extern double __kernel_sin __P((double,double,int));
extern double __kernel_cos __P((double,double));
extern double __kernel_tan __P((double,double,int));
extern int    __kernel_rem_pio2 __P((double*,double*,int,int,int));


/* ieee style elementary float functions */
extern float __ieee754_sqrtf __P((float));
extern float __ieee754_acosf __P((float));
extern float __ieee754_acoshf __P((float));
extern float __ieee754_logf __P((float));
extern float __ieee754_atanhf __P((float));
extern float __ieee754_asinf __P((float));
extern float __ieee754_atan2f __P((float,float));
extern float __ieee754_expf __P((float));
extern float __ieee754_coshf __P((float));
extern float __ieee754_fmodf __P((float,float));
extern float __ieee754_powf __P((float,float));
extern float __ieee754_lgammaf_r __P((float,int *));
extern float __ieee754_gammaf_r __P((float,int *));
extern float __ieee754_lgammaf __P((float));
extern float __ieee754_gammaf __P((float));
extern float __ieee754_log10f __P((float));
extern float __ieee754_log2f __P((float));
extern float __ieee754_sinhf __P((float));
extern float __ieee754_hypotf __P((float,float));
extern float __ieee754_j0f __P((float));
extern float __ieee754_j1f __P((float));
extern float __ieee754_y0f __P((float));
extern float __ieee754_y1f __P((float));
extern float __ieee754_jnf __P((int,float));
extern float __ieee754_ynf __P((int,float));
extern float __ieee754_remainderf __P((float,float));
extern int32_t __ieee754_rem_pio2f __P((float,float*));
extern float __ieee754_scalbf __P((float,float));

/* float versions of fdlibm kernel functions */
extern float __kernel_sinf __P((float,float,int));
extern float __kernel_cosf __P((float,float));
extern float __kernel_tanf __P((float,float,int));
extern int   __kernel_rem_pio2f __P((float*,float*,int,int,int,const int32_t*));

/* ieee style elementary long double functions */
extern long double __ieee754_fmodl(long double, long double);
extern long double __ieee754_sqrtl(long double);

/*
 * TRUNC() is a macro that sets the trailing 27 bits in the mantissa of an
 * IEEE double variable to zero.  It must be expression-like for syntactic
 * reasons, and we implement this expression using an inline function
 * instead of a pure macro to avoid depending on the gcc feature of
 * statement-expressions.
 */
#define	TRUNC(d)	(_b_trunc(&(d)))

static __inline void
_b_trunc(volatile double *_dp)
{
	uint32_t _lw;

	GET_LOW_WORD(_lw, *_dp);
	SET_LOW_WORD(*_dp, _lw & 0xf8000000);
}

struct Double {
	double	a;
	double	b;
};

/*
 * Functions internal to the math package, yet not static.
 */
double	__exp__D(double, double);
struct Double __log__D(double);

/*
 * The rnint() family rounds to the nearest integer for a restricted range
 * range of args (up to about 2**MANT_DIG).  We assume that the current
 * rounding mode is FE_TONEAREST so that this can be done efficiently.
 * Extra precision causes more problems in practice, and we only centralize
 * this here to reduce those problems, and have not solved the efficiency
 * problems.  The exp2() family uses a more delicate version of this that
 * requires extracting bits from the intermediate value, so it is not
 * centralized here and should copy any solution of the efficiency problems.
 */

static inline double
rnint(double x)
{
	/*
	 * This casts to double to kill any extra precision.  This depends
	 * on the cast being applied to a double_t to avoid compiler bugs
	 * (this is a cleaner version of STRICT_ASSIGN()).  This is
	 * inefficient if there actually is extra precision, but is hard
	 * to improve on.  We use double_t in the API to minimise conversions
	 * for just calling here.  Note that we cannot easily change the
	 * magic number to the one that works directly with double_t, since
	 * the rounding precision is variable at runtime on x86 so the
	 * magic number would need to be variable.  Assuming that the
	 * rounding precision is always the default is too fragile.  This
	 * and many other complications will move when the default is
	 * changed to FP_PE.
	 */
	return ((double)(x + 0x1.8p52) - 0x1.8p52);
}

static inline float
rnintf(float x)
{
	/*
	 * As for rnint(), except we could just call that to handle the
	 * extra precision case, usually without losing efficiency.
	 */
	return ((float)(x + 0x1.8p23F) - 0x1.8p23F);
}

#ifdef LDBL_MANT_DIG
/*
 * The complications for extra precision are smaller for rnintl() since it
 * can safely assume that the rounding precision has been increased from
 * its default to FP_PE on x86.  We don't exploit that here to get small
 * optimizations from limiting the range to double.  We just need it for
 * the magic number to work with long doubles.  ld128 callers should use
 * rnint() instead of this if possible.  ld80 callers should prefer
 * rnintl() since for amd64 this avoids swapping the register set, while
 * for i386 it makes no difference (assuming FP_PE), and for other arches
 * it makes little difference.
 */
static inline long double
rnintl(long double x)
{
	return (x + ___CONCAT(0x1.8p,LDBL_MANT_DIG) / 2 -
	    ___CONCAT(0x1.8p,LDBL_MANT_DIG) / 2);
}
#endif /* LDBL_MANT_DIG */

/*
 * irint() and i64rint() give the same result as casting to their integer
 * return type provided their arg is a floating point integer.  They can
 * sometimes be more efficient because no rounding is required.
 */
#if (defined(amd64) || defined(__i386__)) && defined(__GNUCLIKE_ASM)
#define	irint(x)						\
    (sizeof(x) == sizeof(float) &&				\
    sizeof(__float_t) == sizeof(long double) ? irintf(x) :	\
    sizeof(x) == sizeof(double) &&				\
    sizeof(__double_t) == sizeof(long double) ? irintd(x) :	\
    sizeof(x) == sizeof(long double) ? irintl(x) : (int)(x))
#else
#define	irint(x)	((int)(x))
#endif

#define	i64rint(x)	((int64_t)(x))	/* only needed for ld128 so not opt. */

#if defined(__i386__) && defined(__GNUCLIKE_ASM)
static __inline int
irintf(float x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}

static __inline int
irintd(double x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}
#endif

#if (defined(__amd64__) || defined(__i386__)) && defined(__GNUCLIKE_ASM)
static __inline int
irintl(long double x)
{
	int n;

	__asm("fistl %0" : "=m" (n) : "t" (x));
	return (n);
}
#endif

/*
 * The following are fast floor macros for 0 <= |x| < 0x1p(N-1), where
 * N is the precision of the type of x. These macros are used in the
 * half-cycle trignometric functions (e.g., sinpi(x)).
 */
#define	FFLOORF(x, j0, ix) do {			\
	(j0) = (((ix) >> 23) & 0xff) - 0x7f;	\
	(ix) &= ~(0x007fffff >> (j0));		\
	SET_FLOAT_WORD((x), (ix));		\
} while (0)

#define	FFLOOR(x, j0, ix, lx) do {				\
	(j0) = (((ix) >> 20) & 0x7ff) - 0x3ff;			\
	if ((j0) < 20) {					\
		(ix) &= ~(0x000fffff >> (j0));			\
		(lx) = 0;					\
	} else {						\
		(lx) &= ~((uint32_t)0xffffffff >> ((j0) - 20));	\
	}							\
	INSERT_WORDS((x), (ix), (lx));				\
} while (0)

#define	FFLOORL80(x, j0, ix, lx) do {			\
	j0 = ix - 0x3fff + 1;				\
	if ((j0) < 32) {				\
		(lx) = ((lx) >> 32) << 32;		\
		(lx) &= ~((((lx) << 32)-1) >> (j0));	\
	} else {					\
		uint64_t _m;				\
		_m = (uint64_t)-1 >> (j0);		\
		if ((lx) & _m) (lx) &= ~_m;		\
	}						\
	INSERT_LDBL80_WORDS((x), (ix), (lx));		\
} while (0)

#define FFLOORL128(x, ai, ar) do {			\
	union ieee_ext_u u;				\
	uint64_t m;					\
	int e;						\
	u.extu_ld = (x);					\
	e = u.extu_exp - 16383;				\
	if (e < 48) {					\
		m = ((1llu << 49) - 1) >> (e + 1);	\
		u.extu_frach &= ~m;			\
		u.extu_fracl = 0;			\
	} else {					\
		m = (uint64_t)-1 >> (e - 48);		\
		u.extu_fracl &= ~m;			\
	}						\
	(ai) = u.extu_ld;					\
	(ar) = (x) - (ai);				\
} while (0)

#ifdef DEBUG
#if defined(__amd64__) || defined(__i386__)
#define breakpoint()    asm("int $3")
#else
#include <signal.h>
 
#define breakpoint()    raise(SIGTRAP)
#endif  
#endif  

#ifdef STRUCT_RETURN
#define	RETURNSP(rp) do {		\
	if (!(rp)->lo_set)		\
		RETURNF((rp)->hi);	\
	RETURNF((rp)->hi + (rp)->lo);	\
} while (0)
#define	RETURNSPI(rp) do {		\
	if (!(rp)->lo_set)		\
		RETURNI((rp)->hi);	\
	RETURNI((rp)->hi + (rp)->lo);	\
} while (0)
#endif

#define	SUM2P(x, y) ({			\
	const __typeof (x) __x = (x);	\
	const __typeof (y) __y = (y);	\
	__x + __y;			\
})

#ifndef INLINE_KERNEL_SINDF
float   __kernel_sindf(double);
#endif
#ifndef INLINE_KERNEL_COSDF
float   __kernel_cosdf(double);
#endif
#ifndef INLINE_KERNEL_TANDF
float   __kernel_tandf(double,int);
#endif

/* long double precision kernel functions */
long double __kernel_sinl(long double, long double, int);
long double __kernel_cosl(long double, long double);
long double __kernel_tanl(long double, long double, int);

#endif /* _MATH_PRIVATE_H_ */
