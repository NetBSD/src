/* $NetBSD: mcontext.h,v 1.1 2014/08/10 05:47:38 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#ifndef _AARCH64_MCONTEXT_H_
#define _AARCH64_MCONTEXT_H_

#ifdef __aarch64__

/*
 * Layout of mcontext_t based on the System V Application Binary Interface,
 * Edition 4.1, PowerPC Processor ABI Supplement - September 1995, and
 * extended for the AltiVec Register File.  Note that due to the increased
 * alignment requirements of the latter, the offset of mcontext_t within
 * an ucontext_t is different from System V.
 */

#define	_NGREG	35		/* GR0-30, SP, PC, APSR, TPIDR */

typedef	__int64_t	__greg_t;
typedef	__greg_t	__gregset_t[_NGREG];

#define	_REG_X0		0
#define	_REG_X1		1
#define	_REG_X2		2
#define	_REG_X3		3
#define	_REG_X4		4
#define	_REG_X5		5
#define	_REG_X6		6
#define	_REG_X7		7
#define	_REG_X8		8
#define	_REG_X9		9
#define	_REG_X10	10
#define	_REG_X11	11
#define	_REG_X12	12
#define	_REG_X13	13
#define	_REG_X14	14
#define	_REG_X15	15
#define	_REG_X16	16
#define	_REG_X17	17
#define	_REG_X18	18
#define	_REG_X19	19
#define	_REG_X20	20
#define	_REG_X21	21
#define	_REG_X22	22
#define	_REG_X23	23
#define	_REG_X24	24
#define	_REG_X25	25
#define	_REG_X26	26
#define	_REG_X27	27
#define	_REG_X28	28
#define	_REG_X29	29
#define	_REG_X30	30
#define	_REG_SP		31
#define	_REG_PC		32
#define	_REG_SPSR	33
#define	_REG_TPIDR	34

#define	_NFREG	32			/* Number of SIMD registers */

typedef struct {
	union __freg {
		__uint8_t	__b8[16];
		__uint16_t	__h16[8];
		__uint32_t	__s32[4];
		__uint64_t	__d64[2];
		__uint128_t	__q128[1];
	} 	__qregs[_NFREG] __aligned(16);
	__uint32_t	__fpcr;		/* FPCR */
	__uint32_t	__fpsr;		/* FPSR */
} __fregset_t;

typedef struct {
	__gregset_t	__gregs;	/* General Purpose Register set */
	__fregset_t	__fregs;	/* FPU/SIMD Register File */
	__greg_t	__spare[8];	/* future proof */
} mcontext_t;

/* Machine-dependent uc_flags */
#define	_UC_TLSBASE	0x00080000	/* see <sys/ucontext.h> */

#define _UC_MACHINE_SP(uc)	((uc)->uc_mcontext.__gregs[_REG_SP])
#define _UC_MACHINE_PC(uc)	((uc)->uc_mcontext.__gregs[_REG_PC])
#define _UC_MACHINE_INTRV(uc)	((uc)->uc_mcontext.__gregs[_REG_X0])

#define	_UC_MACHINE_SET_PC(uc, pc)	_UC_MACHINE_PC(uc) = (pc)

#if defined(_RTLD_SOURCE) || defined(_LIBC_SOURCE) || defined(__LIBPTHREAD_SOURCE__)
#include <sys/tls.h>

static __inline void *
__lwp_getprivate_fast(void)
{
	void *__tpidr;
	__asm __volatile("mrs\t%0, tpidr_el0" : "=r"(__tpidr));
	return __tpidr;
}

static __inline void *
__lwp_gettcb_fast(void)
{
	void *__tpidr;
	__asm __volatile("mrs\t%0, tpidr_el0" : "=r"(__tpidr));
	return __tpidr;
}

static __inline void
__lwp_settcb(void *__tcb)
{
	__asm __volatile("msr\ttpidr_el0, %0" :: "r"(__tcb));
}
#endif /* _RTLD_SOURCE || _LIBC_SOURCE || __LIBPTHREAD_SOURCE__ */

#elif defined(__arm__)

#include <arm/mcontext.h>

#endif /* __aarch64__/__arm__ */

#endif /* !_AARCH64_MCONTEXT_H_ */
