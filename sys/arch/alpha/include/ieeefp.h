/* $NetBSD: ieeefp.h,v 1.4.4.2 2001/08/30 23:43:43 nathanw Exp $ */

/* 
 * Written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _ALPHA_IEEEFP_H_
#define _ALPHA_IEEEFP_H_

typedef int fp_except;

#ifdef _KERNEL

#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <machine/fpu.h>
#include <machine/alpha.h>

/* FP_X_IOV is intentionally omitted from the architecture flags mask */

#define	FP_AA_FLAGS (FP_X_INV | FP_X_DZ | FP_X_OFL | FP_X_UFL | FP_X_IMP)

#define float_raise(f)						\
	do curproc->l_md.md_flags |= NETBSD_FLAG_TO_FP_C(f);	\
	while(0)

#define float_set_inexact()	float_raise(FP_X_IMP)
#define float_set_invalid()	float_raise(FP_X_INV)
#define	fpgetround()		(alpha_read_fpcr() >> 58 & 3)

#endif

#define	FP_X_INV	0x01	/* invalid operation exception */
#define	FP_X_DZ		0x02	/* divide-by-zero exception */
#define	FP_X_OFL	0x04	/* overflow exception */
#define	FP_X_UFL	0x08	/* underflow exception */
#define	FP_X_IMP	0x10	/* imprecise (loss of precision; "inexact") */
#define	FP_X_IOV	0x20    /* integer overflow */

/*
 * fp_rnd bits match the fpcr, below, as well as bits 12:11
 * in fp operate instructions
 */
typedef enum {
    FP_RZ = 0,			/* round to zero (truncate) */
    FP_RM = 1,			/* round toward negative infinity */
    FP_RN = 2,			/* round to nearest representable number */
    FP_RP = 3,			/* round toward positive infinity */
    _FP_DYNAMIC=FP_RP
} fp_rnd;

#endif /* _ALPHA_IEEEFP_H_ */
