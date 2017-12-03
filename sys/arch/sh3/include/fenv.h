/*	$NetBSD: fenv.h,v 1.3.12.2 2017/12/03 11:36:42 jdolecek Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_FENV_H_
#define _SH3_FENV_H_

#include <sys/stdint.h>
#include <sh3/float.h>
#include <sh3/fpreg.h>

#ifndef __fenv_static   
#define __fenv_static   static
#endif

/* Exception bits, from FPSCR */
#define	FE_INEXACT	((uint32_t)FPSCR_EXCEPTION_INEXACT >> 5)
#define	FE_DIVBYZERO	((uint32_t)FPSCR_EXCEPTION_ZERODIV >> 5)
#define	FE_UNDERFLOW	((uint32_t)FPSCR_EXCEPTION_UNDERFLOW >> 5)
#define	FE_OVERFLOW	((uint32_t)FPSCR_EXCEPTION_OVERFLOW >> 5)
#define	FE_INVALID	((uint32_t)FPSCR_EXCEPTION_INVALID >> 5)

#define FE_ALL_EXCEPT \
    (FE_INEXACT | FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID)

/* Rounding modes, from FPSCR */
#define FE_TONEAREST	FPSCR_ROUND_NEAREST
#define	FE_TOWARDZERO	FPSCR_ROUND_ZERO
/* These two don't exist and are only defined for the benefit of softfloat */
#define	FE_DOWNWARD	(FPSCR_ROUND_ZERO + 1)
#define	FE_UPWARD	(FPSCR_ROUND_ZERO + 2)

#define _ROUND_MASK	\
    (FE_TONEAREST | FE_TOWARDZERO)

#ifdef __SH_FPU_ANY__

typedef uint32_t fexcept_t;

typedef struct {
	uint32_t __fpscr;
} fenv_t;
			 
#define FE_DFL_ENV      ((fenv_t *) -1) 

#define __get_fpscr(__fpscr) \
    __asm__ __volatile__ ("sts fpscr,%0" : "=r" (__fpscr))
#define __set_fpscr(__fpscr) \
    __asm__ __volatile__ ("lds %0,fpscr" : : "r" (__fpscr))

__BEGIN_DECLS

__fenv_static inline int
feclearexcept(int __excepts)
{
	fexcept_t __fpscr;

	__excepts &= FE_ALL_EXCEPT;

	__get_fpscr(__fpscr);
	__fpscr &= ~__excepts;
	__set_fpscr(__fpscr);

	return 0;
}

__fenv_static inline int
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);

	*__flagp = __fpscr & __excepts & FE_ALL_EXCEPT;

	return 0;
}

__fenv_static inline int
fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);

	__fpscr &= ~(__excepts & FE_ALL_EXCEPT);
	__fpscr |= *__flagp & __excepts & FE_ALL_EXCEPT;

	__set_fpscr(__fpscr);

	return 0;
}

static inline void
__fmul(double a, double b)
{
#ifdef __sh4__
	__asm__ __volatile__ ("fmul %1, %0" : "+d" (a) : "d" (b));
#endif
}

static inline void
__fdiv(double a, double b) {
#ifdef __sh4__
	__asm__ __volatile__ ("fdiv %1, %0" : "+d" (a) : "d" (b));
#endif
}

__fenv_static inline int
feraiseexcept(int __excepts)
{
	fexcept_t __fpscr;

	if (__excepts & FE_INVALID)	/* Inf * 0 */
		__fmul(__builtin_huge_val(), 0.0);

	if (__excepts & FE_DIVBYZERO)	/* 1.0 / 0 */
		__fdiv(1.0, 0.0);

	if (__excepts & FE_OVERFLOW)	/* MAX * MAX */
		__fmul(LDBL_MAX, LDBL_MAX);

	if (__excepts & FE_UNDERFLOW)	/* MIN / 10.0 */
		__fdiv(LDBL_MIN, 10.0);

	if (__excepts & FE_INEXACT)	/* 1 / 3 */
		__fdiv(1.0, 3.0);

	__get_fpscr(__fpscr);

	__fpscr |= __excepts & FE_ALL_EXCEPT;

	__set_fpscr(__fpscr);

	return 0;
}

__fenv_static inline int
fetestexcept(int __excepts)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);

	return __fpscr & __excepts & FE_ALL_EXCEPT;
}

__fenv_static inline int
fegetround(void)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);
	return __fpscr & _ROUND_MASK;
}

__fenv_static inline int
fesetround(int __round)
{
	fexcept_t __fpscr;

	if (__round & ~_ROUND_MASK)
		return -1;

	__get_fpscr(__fpscr);

	__fpscr &= ~_ROUND_MASK;
	__fpscr |= __round;

	__set_fpscr(__fpscr);

	return 0;
}

__fenv_static inline int
fegetenv(fenv_t *__envp)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);
	__envp->__fpscr = __fpscr;

	return 0;
}

__fenv_static inline int
feholdexcept(fenv_t *__envp)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);
	__envp->__fpscr = __fpscr;

	__fpscr &= ~FE_ALL_EXCEPT;
	__fpscr &= ~(FE_ALL_EXCEPT << 5);
	__set_fpscr(__fpscr);	/* clear all */

	return 0;
}

__fenv_static inline int
fesetenv(const fenv_t *__envp)
{
	if (__envp == FE_DFL_ENV)
		__set_fpscr(FPSCR_DEFAULT);
	else
		__set_fpscr(__envp->__fpscr);

	return 0;
}

__fenv_static inline int
feupdateenv(const fenv_t *__envp)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);
	__fpscr &= FE_ALL_EXCEPT;
	fesetenv(__envp);
	feraiseexcept((int)__fpscr);
	return 0;
}

#if defined(_NETBSD_SOURCE) || defined(_GNU_SOURCE)

/* We currently provide no external definitions of the functions below. */

static inline int
feenableexcept(int __mask)
{
	fexcept_t __fpscr, __oldmask;

	__get_fpscr(__fpscr);
	__oldmask = (__fpscr >> 5) & FE_ALL_EXCEPT;
	__fpscr |= (__mask & FE_ALL_EXCEPT) << 5;
	__set_fpscr(__fpscr);

	return __oldmask;
}

static inline int
fedisableexcept(int __mask)
{
	fexcept_t __fpscr, __oldmask;

	__get_fpscr(__fpscr);
	__oldmask = (__fpscr >> 5) & FE_ALL_EXCEPT;
	__fpscr &= ~(__mask & FE_ALL_EXCEPT) << 5;
	__set_fpscr(__fpscr);

	return __oldmask;
}

static inline int
fegetexcept(void)
{
	fexcept_t __fpscr;

	__get_fpscr(__fpscr);

	return (__fpscr >> 5) & FE_ALL_EXCEPT;
}

#endif /* _NETBSD_SOURCE || _GNU_SOURCE */

__END_DECLS

#endif /* __SH_FPU_ANY__ */

#endif /* _SH3_FENV_H_ */
