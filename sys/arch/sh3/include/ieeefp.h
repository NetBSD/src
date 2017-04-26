/* $NetBSD: ieeefp.h,v 1.4.62.1 2017/04/26 02:53:07 pgoyette Exp $ */

/*
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _SH3_IEEEFP_H_
#define	_SH3_IEEEFP_H_

#include <sys/featuretest.h>

#if defined(_NETBSD_SOURCE) || defined(_ISOC99_SOURCE)

typedef int fenv_t;
typedef int fexcept_t;

#define	FE_INVALID	0x01	/* invalid operation exception */
#define	FE_DENORMAL	0x02	/* denormalization exception */
#define	FE_DIVBYZERO	0x04	/* divide-by-zero exception */
#define	FE_OVERFLOW	0x08	/* overflow exception */
#define	FE_UNDERFLOW	0x10	/* underflow exception */
#define	FE_INEXACT	0x20	/* imprecise (loss of precision) */

#define	FE_ALL_EXCEPT	0x3f

#define	FE_TONEAREST	0	/* round to nearest representable number */
#define	FE_DOWNWARD	1	/* round toward negative infinity */
#define	FE_UPWARD	2	/* round toward positive infinity */
#define	FE_TOWARDZERO	3	/* round to zero (truncate) */

#if defined(_NETBSD_SOURCE)

typedef int fp_except;
<<<<<<< ieeefp.h
=======

#ifdef	__SH_FPU_ANY__

/* hardfloat */

>>>>>>> 1.7
#define	FP_X_INV	FE_INVALID	/* invalid operation exception */
#define	FP_X_DNML	FE_DENORMAL	/* denormalization exception */
#define	FP_X_DZ		FE_DIVBYZERO	/* divide-by-zero exception */
#define	FP_X_OFL	FE_OVERFLOW	/* overflow exception */
#define	FP_X_UFL	FE_UNDERFLOW	/* underflow exception */
#define	FP_X_IMP	FE_INEXACT	/* imprecise (loss of precision) */

typedef enum {
	FP_RN=FE_TONEAREST,	/* round to nearest representable number */
	FP_RM=FE_DOWNWARD,	/* round toward negative infinity */
	FP_RP=FE_UPWARD,	/* round toward positive infinity */
	FP_RZ=FE_TOWARDZERO	/* round to zero (truncate) */
} fp_rnd;

#else /* __SH_FPU_ANY__ */

/* softfloat */

#define	FP_X_INV	0x01	/* invalid operation exception */
#define	FP_X_DZ		0x04	/* divide-by-zero exception */
#define	FP_X_OFL	0x08	/* overflow exception */
#define	FP_X_UFL	0x10	/* underflow exception */
#define	FP_X_IMP	0x20	/* imprecise (loss of precision) */

typedef enum {
	FP_RN=0,		/* round to nearest representable number */
	FP_RM=1,		/* round toward negative infinity */
	FP_RP=2,        	/* round toward positive infinity */
	FP_RZ=3			/* round to zero (truncate) */
} fp_rnd;

/* adjust for FP_* and FE_* value differences */ 

static inline fp_except
__FPE(int __fe)
{
	int __fp = 0;

	if (__fe & FE_DIVBYZERO)
		__fp |= FP_X_DZ;
	if (__fe & FE_INEXACT)
		__fp |= FP_X_IMP;
	if (__fe & FE_INVALID)
		__fp |= FP_X_INV;
	if (__fe & FE_OVERFLOW)
		__fp |= FP_X_OFL;
	if (__fe & FE_UNDERFLOW)
		__fp |= FP_X_UFL;
	return __fp;
}

static inline int
__FEE(fp_except __fp)
{
	int __fe = 0;

	if (__fp & FP_X_DZ)
		__fe |= FE_DIVBYZERO;
	if (__fp & FP_X_IMP)
		__fe |= FE_INEXACT;
	if (__fp & FP_X_INV)
		__fe |= FE_INVALID;
	if (__fp & FP_X_OFL)
		__fe |= FE_OVERFLOW;
	if (__fp & FP_X_UFL)
		__fe |= FE_UNDERFLOW;
	return __fe;
}

static inline fp_rnd
__FPR(int __fe)
{

	switch (__fe) {
	case FE_TONEAREST:
		return FP_RN;
	case FE_DOWNWARD:
		return FP_RM;
	case FE_UPWARD:
		return FP_RP;
	case FE_TOWARDZERO:
		return FP_RZ;
	default:
		return FP_RN;
	}
}

static inline int
__FER(fp_rnd __fp)
{

	switch (__fp) {
	case FP_RN:
		return FE_TONEAREST;
	case FP_RM:
		return FE_DOWNWARD;
	case FP_RP:
		return FE_UPWARD;
	case FP_RZ:
		return FE_TOWARDZERO;
	default:
		return FE_TONEAREST;
	}
}

#endif /* __SH_FPU_ANY__ */

#endif /* !_ISOC99_SOURCE */

#endif /* _NETBSD_SOURCE || _ISOC99_SOURCE */

#endif /* !_SH3_IEEEFP_H_ */
