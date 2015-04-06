/*	$NetBSD: ieeefp.h,v 1.8.16.1 2015/04/06 15:18:02 skrll Exp $	*/

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _SPARC_IEEEFP_H_
#define _SPARC_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#define	FE_TONEAREST	0	/* round to nearest representable number */
#define	FE_TOWARDZERO	1	/* round to zero (truncate) */
#define	FE_UPWARD	2	/* round toward positive infinity */
#define	FE_DOWNWARD	3	/* round toward negative infinity */

#if !defined(_ISOC99_SOURCE)

typedef unsigned int fp_except;
#define FP_X_IMP	0x01		/* imprecise (loss of precision) */
#define FP_X_DZ		0x02		/* divide-by-zero exception */
#define FP_X_UFL	0x04		/* underflow exception */
#define FP_X_OFL	0x08		/* overflow exception */
#define FP_X_INV	0x10		/* invalid operation exception */

typedef enum {
    FP_RN=0,			/* round to nearest representable number */
    FP_RZ=1,			/* round to zero (truncate) */
    FP_RP=2,			/* round toward positive infinity */
    FP_RM=3			/* round toward negative infinity */
} fp_rnd;

#endif /* !_ISOC99_SOURCE */

#endif /* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _SPARC_IEEEFP_H_ */
