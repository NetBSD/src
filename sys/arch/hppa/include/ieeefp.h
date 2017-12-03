/*	$NetBSD: ieeefp.h,v 1.4.38.1 2017/12/03 11:36:16 jdolecek Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _HPPA_IEEEFP_H_
#define _HPPA_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#if !defined(_ISOC99_SOURCE)

typedef int fp_except;
#define FP_X_INV	0x10		/* invalid operation exception */
#define FP_X_DZ		0x08		/* divide-by-zero exception */
#define FP_X_OFL	0x04		/* overflow exception */
#define FP_X_UFL	0x02		/* underflow exception */
#define FP_X_IMP	0x01		/* imprecise (loss of precision) */

typedef enum {
    FP_RN=0,			/* round to nearest representable number */
    FP_RZ=1,			/* round to zero (truncate) */
    FP_RP=2,			/* round toward positive infinity */
    FP_RM=3			/* round toward negative infinity */
} fp_rnd;

#endif /* !_ISOC99_SOURCE */

#endif /* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* _HPPA_IEEEFP_H_ */
