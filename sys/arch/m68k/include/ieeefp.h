/*	$NetBSD: ieeefp.h,v 1.5.84.1 2008/10/19 22:15:51 haad Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Modified by Jason R. Thorpe, June 22, 2003
 * Public domain.
 */

#ifndef _M68K_IEEEFP_H_
#define _M68K_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

typedef int fenv_t;
typedef int fexcept_t;

#define	FE_INEXACT	0x01	/* imprecise (loss of precision) */
#define	FE_DIVBYZERO	0x02	/* divide-by-zero exception */
#define	FE_UNDERFLOW	0x04	/* overflow exception */
#define	FE_OVERFLOW	0x08	/* underflow exception */
#define	FE_INVALID	0x10	/* invalid operation exception */

#define	FE_ALL_EXCEPT	0x1f

#define	FE_TONEAREST	0	/* round to nearest representable number */
#define	FE_TOWARDZERO	1	/* round to zero (truncate) */
#define	FE_UPWARD	2	/* round toward positive infinity */
#define	FE_DOWNWARD	3	/* round toward negative infinity */

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

#endif /* defined(_NETBSD_SOURCE) */

#define	__HAVE_FP_PREC


#endif	/* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _M68K_IEEEFP_H_ */
