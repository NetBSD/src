/*	$NetBSD: mcontext.h,v 1.10.32.3 2018/03/21 10:08:03 martin Exp $	*/

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

#ifndef _SH3_MCONTEXT_H_
#define _SH3_MCONTEXT_H_

/*
 * Layout of mcontext_t for the sh3 architecture.
 */

#define _NGREG		22
typedef int		__greg_t;
typedef __greg_t	__gregset_t[_NGREG];

#define	_REG_GBR	0
#define	_REG_PC		1
#define	_REG_SR		2
#define	_REG_MACL	3
#define	_REG_MACH	4
#define	_REG_PR		5
#define	_REG_R14	6
#define	_REG_R13	7
#define	_REG_R12	8
#define	_REG_R11	9
#define	_REG_R10	10
#define	_REG_R9		11
#define	_REG_R8		12
#define	_REG_R7		13
#define	_REG_R6		14
#define	_REG_R5		15
#define	_REG_R4		16
#define	_REG_R3		17
#define	_REG_R2		18
#define	_REG_R1		19
#define	_REG_R0		20
#define	_REG_R15	21
/* Convenience synonym */
#define	_REG_SP		_REG_R15

/*
 * FPU state description.
 * XXX: kernel doesn't support FPU yet, so this is just a placeholder.
 */
typedef struct {
	int		__fpr_fpscr;
	int		__fpr_fpul;
	/* XXX: redefine as a union when we do support FPU */
	int		__fpr_regs[32];	/* SH3E has 16, SH4 has 32 */
} __fpregset_t;

typedef struct {
	__gregset_t	__gregs;
	__fpregset_t	__fpregs;
} mcontext_t;

#define	_UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])
#define	_UC_MACHINE_FP(uc)	((uc)->uc_mcontext.__gregs[_REG_R14])
#define	_UC_MACHINE_PC(uc)	((uc)->uc_mcontext.__gregs[_REG_PC])
#define	_UC_MACHINE_INTRV(uc)	((uc)->uc_mcontext.__gregs[_REG_R0])

#define	_UC_MACHINE_SET_PC(uc, pc)	_UC_MACHINE_PC(uc) = (pc)

/*
 * Machine dependent uc_flags
 */
#define	_UC_SETSTACK		0x10000
#define	_UC_CLRSTACK		0x20000
#define	_UC_TLSBASE		0x80000

static __inline void *
__lwp_getprivate_fast(void)
{
	register void *__gbr;

	__asm volatile("stc gbr, %0" : "=r" (__gbr));

	return __gbr;
}

#endif /* !_SH3_MCONTEXT_H_ */
