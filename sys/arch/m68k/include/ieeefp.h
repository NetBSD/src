/*	$NetBSD: ieeefp.h,v 1.7.42.1 2015/12/27 12:09:37 skrll Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Modified by Jason R. Thorpe, June 22, 2003
 * Public domain.
 */

#ifndef _M68K_IEEEFP_H_
#define _M68K_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#include <m68k/fenv.h>

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
    FP_RM=FE_DOWNWARD,		/* round toward negative infinity */
    FP_RP=FE_UPWARD		/* round toward positive infinity */
} fp_rnd;

typedef enum {
    FP_PE=0,			/* extended-precision (64-bit) */
    FP_PS=1,			/* single-precision (24-bit) */
    FP_PD=2			/* double-precision (53-bit) */
} fp_prec;

#endif /* !_ISOC99_SOURCE */

#define	__HAVE_FP_PREC

#endif	/* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _M68K_IEEEFP_H_ */
