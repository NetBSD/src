/*	$NetBSD: ieeefp.h,v 1.3.58.1 2015/12/27 12:09:40 skrll Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _POWERPC_IEEEFP_H_
#define _POWERPC_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#include <powerpc/fenv.h>

#if !defined(_ISOC99_SOURCE)

typedef int fp_except;
#define FP_X_IMP	FE_INEXACT	/* imprecise (loss of precision) */
#define FP_X_DZ		FE_DIVBYZERO	/* divide-by-zero exception */
#define FP_X_UFL	FE_UNDERFLOW	/* underflow exception */
#define FP_X_OFL	FE_OVERFLOW	/* overflow exception */
#define FP_X_INV	FE_INVALID	/* invalid operation exception */

typedef enum {
    FP_RN=FE_TONEAREST,		/* round to nearest representable number */
    FP_RZ=FE_TOWARDZERO,	/* round to zero (truncate) */
    FP_RP=FE_UPWARD,		/* round toward positive infinity */
    FP_RM=FE_DOWNWARD		/* round toward negative infinity */
} fp_rnd;

#endif /* !_ISOC99_SOURCE */

#endif /* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _POWERPC_IEEEFP_H_ */
