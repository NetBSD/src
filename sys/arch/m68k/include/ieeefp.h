/*	$NetBSD: ieeefp.h,v 1.3.48.2 2004/09/18 14:36:17 skrll Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Modified by Jason R. Thorpe, June 22, 2003
 * Public domain.
 */

#ifndef _M68K_IEEEFP_H_
#define _M68K_IEEEFP_H_

typedef int fp_except;
#define FP_X_IMP	0x01	/* imprecise (loss of precision) */
#define FP_X_DZ		0x02	/* divide-by-zero exception */
#define FP_X_UFL	0x04	/* underflow exception */
#define FP_X_OFL	0x08	/* overflow exception */
#define FP_X_INV	0x10	/* invalid operation exception */

typedef enum {
    FP_RN=0,			/* round to nearest representable number */
    FP_RZ=1,			/* round to zero (truncate) */
    FP_RM=2,			/* round toward negative infinity */
    FP_RP=3			/* round toward positive infinity */
} fp_rnd;

typedef enum {
    FP_PE=0,			/* extended-precision (64-bit) */
    FP_PS=1,			/* single-precision (24-bit) */
    FP_PD=2			/* double-precision (53-bit) */
} fp_prec;

#define	__HAVE_FP_PREC

#endif /* _M68K_IEEEFP_H_ */
