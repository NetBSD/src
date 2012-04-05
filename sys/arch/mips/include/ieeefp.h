/*	$NetBSD: ieeefp.h,v 1.6.8.1 2012/04/05 21:33:17 mrg Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#ifndef _MIPS_IEEEFP_H_
#define _MIPS_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

typedef unsigned int fenv_t;
typedef unsigned int fexcept_t;

#define	FE_INEXACT	0x01	/* imprecise (loss of precision) */
#define	FE_UNDERFLOW	0x02	/* underflow exception */
#define	FE_OVERFLOW	0x04	/* overflow exception */
#define	FE_DIVBYZERO	0x08	/* divide-by-zero exception */
#define	FE_INVALID	0x10	/* invalid operation exception */

#define	FE_ALL_EXCEPT	0x1f

#define	FE_TONEAREST	0	/* round to nearest representable number */
#define	FE_TOWARDZERO	1	/* round to zero (truncate) */
#define	FE_UPWARD	2	/* round toward positive infinity */
#define	FE_DOWNWARD	3	/* round toward negative infinity */

#if !defined(_ISOC99_SOURCE)

typedef unsigned int fp_except;
#define FP_X_IMP	FE_INEXACT	/* imprecise (loss of precision) */
#define FP_X_UFL	FE_UNDERFLOW	/* underflow exception */
#define FP_X_OFL	FE_OVERFLOW	/* overflow exception */
#define FP_X_DZ		FE_DIVBYZERO	/* divide-by-zero exception */
#define FP_X_INV	FE_INVALID	/* invalid operation exception */

typedef enum {
    FP_RN=FE_TONEAREST,		/* round to nearest representable number */
    FP_RZ=FE_TOWARDZERO,	/* round to zero (truncate) */
    FP_RP=FE_UPWARD,		/* round toward positive infinity */
    FP_RM=FE_DOWNWARD		/* round toward negative infinity */
} fp_rnd;

#endif /* !_ISOC99_SOURCE */

#endif /* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _MIPS_IEEEFP_H_ */
