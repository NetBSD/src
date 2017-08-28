/* $NetBSD: ieeefp.h,v 1.1.18.1 2017/08/28 17:51:42 skrll Exp $ */

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _IA64_IEEEFP_H_
#define _IA64_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#include <ia64/fenv.h>

/*
 * These bits match the fpcr as well as bits 12:11
 * in fp operate instructions
 */
/* XXX remove since fenv.h now contains (but with different values)? */
#if 0
#define	FE_TOWARDZERO	0	/* round to zero (truncate) */
#define	FE_DOWNWARD	1	/* round toward negative infinity */
#define	FE_TONEAREST	2	/* round to nearest representable number */
#define	FE_UPWARD	3	/* round toward positive infinity */
#endif

#if !defined(_ISOC99_SOURCE)

typedef int fp_except;

#define	FP_X_INV	FE_INVALID	/* invalid operation exception */
#define	FP_X_DZ		FE_DIVBYZERO	/* divide-by-zero exception */
#define	FP_X_OFL	FE_OVERFLOW	/* overflow exception */
#define	FP_X_UFL	FE_UNDERFLOW	/* underflow exception */
#define	FP_X_IMP	FE_INEXACT	/* imprecise (prec. loss; "inexact") */
#define	FP_X_IOV	FE_IOVERFLOW	/* integer overflow */

/*
 * fp_rnd bits match the fpcr, below, as well as bits 12:11
 * in fp operate instructions
 */
typedef enum {
    FP_RZ = FE_TOWARDZERO,	/* round to zero (truncate) */
    FP_RM = FE_DOWNWARD,	/* round toward negative infinity */
    FP_RN = FE_TONEAREST,	/* round to nearest representable number */
    FP_RP = FE_UPWARD,		/* round toward positive infinity */
    _FP_DYNAMIC=FP_RP
} fp_rnd;

#endif /* !_ISOC99_SOURCE */

#endif	/* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _IA64_IEEEFP_H_ */










