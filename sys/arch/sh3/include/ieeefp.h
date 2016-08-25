/* $NetBSD: ieeefp.h,v 1.5 2016/08/25 12:19:42 christos Exp $ */

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _SH3_IEEEFP_H_
#define	_SH3_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#include <fenv.h>

#if defined(_NETBSD_SOURCE)

#define	FP_X_INV	FE_INVALID	/* invalid operation exception */
#define	FP_X_DZ		FE_DIVBYZERO	/* divide-by-zero exception */
#define	FP_X_OFL	FE_OVERFLOW	/* overflow exception */
#define	FP_X_UFL	FE_UNDERFLOW	/* underflow exception */
#define	FP_X_IMP	FE_INEXACT	/* imprecise (loss of precision) */

typedef enum {
	FP_RN=FE_TONEAREST,	/* round to nearest representable number */
	FP_RZ=FE_TOWARDZERO	/* round to zero (truncate) */
} fp_rnd;

#endif /* !_ISOC99_SOURCE */

#endif /* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* !_SH3_IEEEFP_H_ */
