/*	$NetBSD: math.h,v 1.30 2004/01/17 01:04:46 uwe Exp $	*/

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
 * @(#)fdlibm.h 5.1 93/09/24
 */

#ifndef _MATH_H_
#define _MATH_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>

union __float_u {
	unsigned char __dummy[sizeof(float)];
	float __val;
};

union __double_u {
	unsigned char __dummy[sizeof(double)];
	double __val;
};

union __long_double_u {
	unsigned char __dummy[sizeof(long double)];
	long double __val;
};

#include <machine/math.h>		/* may use __float_u, __double_u,
					   or __long_double_u */

#ifdef __HAVE_LONG_DOUBLE
#define	__fpmacro_unary_floating(__name, __arg0)			\
	((sizeof (__arg0) == sizeof (float))				\
	?	__ ## __name ## f (__arg0)				\
	: (sizeof (__arg0) == sizeof (double))				\
	?	__ ## __name ## d (__arg0)				\
	:	__ ## __name ## l (__arg0))
#else
#define	__fpmacro_unary_floating(__name, __arg0)			\
	((sizeof (__arg0) == sizeof (float))				\
	?	__ ## __name ## f (__arg0)				\
	:	__ ## __name ## d (__arg0))
#endif /* __HAVE_LONG_DOUBLE */

/*
 * ANSI/POSIX
 */
/* 7.12#3 HUGE_VAL, HUGELF, HUGE_VALL */
extern __const union __double_u __infinity;
#define HUGE_VAL	__infinity.__val

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
/* 7.12#3 HUGE_VAL, HUGELF, HUGE_VALL */
extern __const union __float_u __infinityf;
#define	HUGE_VALF	__infinityf.__val

extern __const union __long_double_u __infinityl;
#define	HUGE_VALL	__infinityl.__val

/* 7.12#4 INFINITY */
#ifdef __INFINITY
#define	INFINITY	__INFINITY	/* float constant which overflows */
#else
#define	INFINITY	HUGE_VALF	/* positive infinity */
#endif /* __INFINITY */

/* 7.12#5 NAN: a quiet NaN, if supported */
#ifdef __HAVE_NANF
extern __const union __float_u __nanf;
#define	NAN		__nanf.__val
#endif /* __HAVE_NANF */

/* 7.12#6 number classification macros */
#define	FP_INFINITE	0x00
#define	FP_NAN		0x01
#define	FP_NORMAL	0x02
#define	FP_SUBNORMAL	0x03
#define	FP_ZERO		0x04
/* NetBSD extensions */
#define	_FP_LOMD	0x80		/* range for machine-specific classes */
#define	_FP_HIMD	0xff

#endif /* !_ANSI_SOURCE && ... */

/*
 * XOPEN/SVID
 */
#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#define	MAXFLOAT	((float)3.40282346638528860e+38)
extern int signgam;
#endif /* _XOPEN_SOURCE || _NETBSD_SOURCE */

#if defined(_NETBSD_SOURCE)
enum fdversion {fdlibm_ieee = -1, fdlibm_svid, fdlibm_xopen, fdlibm_posix};

#define _LIB_VERSION_TYPE enum fdversion
#define _LIB_VERSION _fdlib_version  

/* if global variable _LIB_VERSION is not desirable, one may 
 * change the following to be a constant by: 
 *	#define _LIB_VERSION_TYPE const enum version
 * In that case, after one initializes the value _LIB_VERSION (see
 * s_lib_version.c) during compile time, it cannot be modified
 * in the middle of a program
 */ 
extern  _LIB_VERSION_TYPE  _LIB_VERSION;

#define _IEEE_  fdlibm_ieee
#define _SVID_  fdlibm_svid
#define _XOPEN_ fdlibm_xopen
#define _POSIX_ fdlibm_posix

#ifndef __cplusplus
struct exception {
	int type;
	char *name;
	double arg1;
	double arg2;
	double retval;
};
#endif

#define	HUGE		MAXFLOAT

/* 
 * set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
 * (one may replace the following line by "#include <values.h>")
 */

#define X_TLOSS		1.41484755040568800000e+16 

#define	DOMAIN		1
#define	SING		2
#define	OVERFLOW	3
#define	UNDERFLOW	4
#define	TLOSS		5
#define	PLOSS		6

#endif /* _NETBSD_SOURCE */

__BEGIN_DECLS
/*
 * ANSI/POSIX
 */
double	acos __P((double));
double	asin __P((double));
double	atan __P((double));
double	atan2 __P((double, double));
double	cos __P((double));
double	sin __P((double));
double	tan __P((double));

double	cosh __P((double));
double	sinh __P((double));
double	tanh __P((double));

double	exp __P((double));
double	frexp __P((double, int *));
double	ldexp __P((double, int));
double	log __P((double));
double	log10 __P((double));
double	modf __P((double, double *));

double	pow __P((double, double));
double	sqrt __P((double));

double	ceil __P((double));
double	fabs __P((double));
double	floor __P((double));
double	fmod __P((double, double));

#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
double	erf __P((double));
double	erfc __P((double));
double	gamma __P((double));
double	hypot __P((double, double));
int	isnan __P((double));
#if defined(_LIBC)
int	isnanl __P((long double));
#endif
int	finite __P((double));
double	j0 __P((double));
double	j1 __P((double));
double	jn __P((int, double));
double	lgamma __P((double));
double	y0 __P((double));
double	y1 __P((double));
double	yn __P((int, double));

#if (_XOPEN_SOURCE - 0) >= 500 || defined(_NETBSD_SOURCE)
double	acosh __P((double));
double	asinh __P((double));
double	atanh __P((double));
double	cbrt __P((double));
double	expm1 __P((double));
int	ilogb __P((double));
double	log1p __P((double));
double	logb __P((double));
double	nextafter __P((double, double));
double	remainder __P((double, double));
double	rint __P((double));
double	scalb __P((double, double));
#endif /* (_XOPEN_SOURCE - 0) >= 500 || defined(_NETBSD_SOURCE)*/
#endif /* _XOPEN_SOURCE || _NETBSD_SOURCE */

/*
 * ISO C99
 */
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
/* 7.12.3.1 int fpclassify(real-floating x) */
#define	fpclassify(__x)	__fpmacro_unary_floating(fpclassify, __x)

/* 7.12.3.2 int isfinite(real-floating x) */
#define	isfinite(__x)	__fpmacro_unary_floating(isfinite, __x)

/* 7.12.3.5 int isnormal(real-floating x) */
#define	isnormal(__x)	(fpclassify(__x) == FP_NORMAL)

/* 7.12.3.6 int signbit(real-floating x) */
#define	signbit(__x)	__fpmacro_unary_floating(signbit, __x)

#endif /* !_ANSI_SOURCE && ... */

#if defined(_NETBSD_SOURCE)
#ifndef __cplusplus
int	matherr __P((struct exception *));
#endif

/*
 * IEEE Test Vector
 */
double	significand __P((double));

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
double	copysign __P((double, double));
double	scalbn __P((double, int));

/*
 * BSD math library entry points
 */
#ifndef __MATH_PRIVATE__
double	cabs __P((/* struct complex { double r; double i; } */));
#endif
double	drem __P((double, double));

#endif /* _NETBSD_SOURCE */

#if defined(_NETBSD_SOURCE) || defined(_REENTRANT)
/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
double	gamma_r __P((double, int *));
double	lgamma_r __P((double, int *));
#endif /* _NETBSD_SOURCE || _REENTRANT */


#if defined(_NETBSD_SOURCE)
int	isinf __P((double));
#if defined(_LIBC)
int	isinfl __P((long double));
#endif

/* float versions of ANSI/POSIX functions */
float	acosf __P((float));
float	asinf __P((float));
float	atanf __P((float));
float	atan2f __P((float, float));
float	cosf __P((float));
float	sinf __P((float));
float	tanf __P((float));

float	coshf __P((float));
float	sinhf __P((float));
float	tanhf __P((float));

float	expf __P((float));
float	frexpf __P((float, int *));
float	ldexpf __P((float, int));
float	logf __P((float));
float	log10f __P((float));
float	modff __P((float, float *));

float	powf __P((float, float));
float	sqrtf __P((float));

float	ceilf __P((float));
float	fabsf __P((float));
float	floorf __P((float));
float	fmodf __P((float, float));

float	erff __P((float));
float	erfcf __P((float));
float	gammaf __P((float));
float	hypotf __P((float, float));
int	isinff __P((float));
int	isnanf __P((float));
int	finitef __P((float));
float	j0f __P((float));
float	j1f __P((float));
float	jnf __P((int, float));
float	lgammaf __P((float));
float	y0f __P((float));
float	y1f __P((float));
float	ynf __P((int, float));

float	acoshf __P((float));
float	asinhf __P((float));
float	atanhf __P((float));
float	cbrtf __P((float));
float	logbf __P((float));
float	nextafterf __P((float, float));
float	remainderf __P((float, float));
float	scalbf __P((float, float));

/*
 * float version of IEEE Test Vector
 */
float	significandf __P((float));

/*
 * Float versions of functions callable from C, intended to support
 * IEEE arithmetic.
 */
float	copysignf __P((float, float));
int	ilogbf __P((float));
float	rintf __P((float));
float	scalbnf __P((float, int));

/*
 * float versions of BSD math library entry points
 */
#ifndef __MATH_PRIVATE__
float	cabsf __P((/* struct complex { float r; float i; } */));
#endif
float	dremf __P((float, float));
float	expm1f __P((float));
float	log1pf __P((float));
#endif /* _NETBSD_SOURCE */

#if defined(_NETBSD_SOURCE) || defined(_REENTRANT)
/*
 * Float versions of reentrant version of gamma & lgamma; passes
 * signgam back by reference as the second argument; user must
 * allocate space for signgam.
 */
float	gammaf_r __P((float, int *));
float	lgammaf_r __P((float, int *));
#endif /* !... || _REENTRANT */

/*
 * Library implementation
 */
int	__fpclassifyf __P((float));
int	__fpclassifyd __P((double));
int	__isfinitef __P((float));
int	__isfinited __P((double));
int	__signbitf __P((float));
int	__signbitd __P((double));

#ifdef __HAVE_LONG_DOUBLE
int	__fpclassifyl __P((long double));
int	__isfinitel __P((long double));
int	__signbitl __P((long double));
#endif
__END_DECLS

#endif /* _MATH_H_ */
