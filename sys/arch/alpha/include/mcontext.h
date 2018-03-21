/*	$NetBSD: mcontext.h,v 1.8.32.3 2018/03/21 10:08:02 martin Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#ifndef _ALPHA_MCONTEXT_H_
#define _ALPHA_MCONTEXT_H_

/*
 * General register state (important: 0-31 maps to `struct reg')
 */
#define _NGREG		34	/* 0-31, PC, PS */
typedef	unsigned long	__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

/* Convenience synonyms */
#define	_REG_V0		0
#define	_REG_T0		1
#define	_REG_T1		2
#define	_REG_T2		3
#define	_REG_T3		4
#define	_REG_T4		5
#define	_REG_T5		6
#define	_REG_T6		7
#define	_REG_T7		8
#define	_REG_S0		9
#define	_REG_S1		10
#define	_REG_S2		11
#define	_REG_S3		12
#define	_REG_S4		13
#define	_REG_S5		14
#define	_REG_S6		15
#define	_REG_A0		16
#define	_REG_A1		17
#define	_REG_A2		18
#define	_REG_A3		19
#define	_REG_A4		20
#define	_REG_A5		21
#define	_REG_T8		22
#define	_REG_T9		23
#define	_REG_T10	24
#define	_REG_T11	25
#define	_REG_RA		26
#define	_REG_T12	27
#define	_REG_PV		27
#define	_REG_AT		28
#define	_REG_GP		29
#define	_REG_SP		30
#define	_REG_UNIQUE	31
#define	_REG_PC		32
#define	_REG_PS		33

/*
 * Floating point register state (important: maps to `struct fpreg')
 */
typedef struct {
	union {
		unsigned long	__fp_regs[32];
		double		__fp_dregs[32];
	}		__fp_fr;
	unsigned long	__fp_fpcr;
} __fpregset_t;

typedef struct {
	__gregset_t	__gregs;
	__fpregset_t	__fpregs;
} mcontext_t;

/* Machine-dependent uc_flags */
#define _UC_TLSBASE	0x20	/* valid process-unique value in _REG_UNIQUE */

#define _UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])
#define _UC_MACHINE_FP(uc)	((uc)->uc_mcontext.__gregs[_REG_S6])
#define _UC_MACHINE_PC(uc)	((uc)->uc_mcontext.__gregs[_REG_PC])
#define _UC_MACHINE_INTRV(uc)	((uc)->uc_mcontext.__gregs[_REG_V0])

#define	_UC_MACHINE_SET_PC(uc, pc)	_UC_MACHINE_PC(uc) = (pc)

static __inline void *
__lwp_getprivate_fast(void)
{
	register void *__tmp __asm("$0");

	__asm volatile("call_pal %1 # PAL_rdunique"
		: "=r" (__tmp)
		: "i" (0x009e /* PAL_rdunique */));

	return __tmp;
}

#endif	/* !_ALPHA_MCONTEXT_H_ */
