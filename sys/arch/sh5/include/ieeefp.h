/*	$NetBSD: ieeefp.h,v 1.3.2.2 2002/07/16 00:41:16 gehenna Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Updated for SH5 by Steve C. Woodford of Wasabi Systems.
 * Public domain.
 */

#ifndef _SH5_IEEEFP_H_
#define	_SH5_IEEEFP_H_

typedef int fp_except;
#define	FP_X_IMP	0x01	/* imprecise (loss of precision) */
#define	FP_X_UFL	0x02	/* underflow exception */
#define	FP_X_OFL	0x04	/* overflow exception */
#define	FP_X_DZ		0x08	/* divide-by-zero exception */
#define	FP_X_INV	0x10	/* invalid operation exception */

typedef enum {
	FP_RZ=0,	/* round to zero (truncate) */
	FP_RN=1,	/* round to nearest representable number */
	FP_RM=2,	/* round toward negative infinity (no h/w support) */
	FP_RP=3		/* round toward positive infinity (no h/w support) */
} fp_rnd;

#endif /* !_SH5_IEEEFP_H_ */
