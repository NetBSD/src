/*	$NetBSD: ieeefp.h,v 1.5.12.1 2006/05/24 15:48:14 tron Exp $	*/

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _PC532_IEEEFP_H_
#define	_PC532_IEEEFP_H_

typedef int fp_except;
#define	FP_X_IMP	0x0020	/* imprecise (loss of precision) */
#define	FP_X_OFL	0x0200	/* overflow exception */
#define	FP_X_INV	0x0800	/* invalid operation exception */
#define	FP_X_DZ		0x2000	/* divide-by-zero exception */
#define	FP_X_UFL	0x8000	/* underflow exception */

typedef enum {
	FP_RN=0,		/* round to nearest representable number */
	FP_RZ=1,		/* round to zero (truncate) */
	FP_RP=2,		/* round toward positive infinity */
	FP_RM=3			/* round toward negative infinity */
} fp_rnd;

#endif /* _PC532_IEEEFP_H_ */
