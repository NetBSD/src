/* $NetBSD: ieeefp.h,v 1.3 1999/04/29 02:55:50 ross Exp $ */

/* 
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _ALPHA_IEEEFP_H_
#define _ALPHA_IEEEFP_H_

typedef int fp_except;
#define	FP_X_INV	0x01	/* invalid operation exception */
#define	FP_X_DZ		0x02	/* divide-by-zero exception */
#define	FP_X_OFL	0x04	/* overflow exception */
#define	FP_X_UFL	0x08	/* underflow exception */
#define	FP_X_IMP	0x10	/* imprecise (loss of precision; "inexact") */
#define	FP_X_IOV	0x20    /* integer overflow XXX? */

typedef enum {
    FP_RZ=0,			/* round to zero (truncate) */
    FP_RM=1,			/* round toward negative infinity */
    FP_RN=2,			/* round to nearest representable number */
    FP_RP=3			/* round toward positive infinity */
} fp_rnd;

#ifdef _KERNEL
#define	FPCR_SUM	(1UL << 63)
#define	FPCR_INED	(1UL << 62)
#define	FPCR_UNFD	(1UL << 61)
#define	FPCR_UNDZ	(1UL << 60)
#define	FPCR_DYN(rm)	((unsigned long)(rm) << 58)
#define	FPCR_IOV	(1UL << 57)
#define	FPCR_INE	(1UL << 56)
#define	FPCR_UNF	(1UL << 55)
#define	FPCR_OVF	(1UL << 54)
#define	FPCR_DZE	(1UL << 53)
#define	FPCR_INV	(1UL << 52)
#define	FPCR_OVFD	(1UL << 51)
#define	FPCR_DZED	(1UL << 50)
#define	FPCR_INVD	(1UL << 49)
#define	FPCR_DNZ	(1UL << 48)
#define	FPCR_DNOD	(1UL << 47)
#endif

#endif /* _ALPHA_IEEEFP_H_ */
