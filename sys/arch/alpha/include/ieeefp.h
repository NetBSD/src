/* $NetBSD: ieeefp.h,v 1.8.24.1 2016/10/05 20:55:23 skrll Exp $ */

/*
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _ALPHA_IEEEFP_H_
#define _ALPHA_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

#include <machine/fenv.h>

#if !defined(_ISOC99_SOURCE)

typedef int fp_except;

#if defined(_KERNEL)

#include <sys/param.h>
#include <sys/proc.h>
#include <machine/fpu.h>
#include <machine/alpha.h>

/* FP_X_IOV is intentionally omitted from the architecture flags mask */

#define	FP_AA_FLAGS (FP_X_INV | FP_X_DZ | FP_X_OFL | FP_X_UFL | FP_X_IMP)

#define float_raise(f)						\
	do curlwp->l_md.md_flags |= NETBSD_FLAG_TO_FP_C(f);	\
	while(0)

#define float_set_inexact()	float_raise(FP_X_IMP)
#define float_set_invalid()	float_raise(FP_X_INV)
#define	fpgetround()		(alpha_read_fpcr() >> 58 & 3)

#endif /* _KERNEL */

#define	FP_X_INV	FE_INVALID	/* invalid operation exception */
#define	FP_X_DZ		FE_DIVBYZERO	/* divide-by-zero exception */
#define	FP_X_OFL	FE_OVERFLOW	/* overflow exception */
#define	FP_X_UFL	FE_UNDERFLOW	/* underflow exception */
#define	FP_X_IMP	FE_INEXACT	/* imprecise (prec. loss; "inexact") */
#define	FP_X_IOV	FE_INTOVF	/* integer overflow */

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

#endif /* _ALPHA_IEEEFP_H_ */
