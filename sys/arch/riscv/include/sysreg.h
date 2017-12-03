/* $NetBSD: sysreg.h,v 1.3.16.2 2017/12/03 11:36:39 jdolecek Exp $ */
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

#ifndef _RISCV_SYSREG_H_
#define _RISCV_SYSREG_H_

#ifndef _KERNEL
#include <sys/param.h>
#endif

#define FCSR_FMASK	0	// no exception bits
#define FCSR_FRM	__BITS(7,5)
#define FCSR_FRM_RNE	0b000	// Round Nearest, ties to Even
#define FCSR_FRM_RTZ	0b001	// Round Towards Zero
#define FCSR_FRM_RDN	0b010	// Round DowN (-infinity)
#define FCSR_FRM_RUP	0b011	// Round UP (+infinity)
#define FCSR_FRM_RMM	0b100	// Round to nearest, ties to Max Magnitude
#define FCSR_FFLAGS	__BITS(4,0)	// Sticky bits
#define FCSR_NV		__BIT(4)	// iNValid operation
#define FCSR_DZ		__BIT(3)	// Divide by Zero
#define FCSR_OF		__BIT(2)	// OverFlow
#define FCSR_UF		__BIT(1)	// UnderFlow
#define FCSR_NX		__BIT(0)	// iNeXact

static inline uint32_t
riscvreg_fcsr_read(void)
{
	uint32_t __fcsr;
	__asm("frcsr %0" : "=r"(__fcsr));
	return __fcsr;
}


static inline uint32_t
riscvreg_fcsr_write(uint32_t __new)
{
	uint32_t __old;
	__asm("fscsr %0, %1" : "=r"(__old) : "r"(__new));
	return __old;
}

static inline uint32_t
riscvreg_fcsr_read_fflags(void)
{
	uint32_t __old;
	__asm("frflags %0" : "=r"(__old));
	return __SHIFTOUT(__old, FCSR_FFLAGS);
}

static inline uint32_t
riscvreg_fcsr_write_fflags(uint32_t __new)
{
	uint32_t __old;
	__new = __SHIFTIN(__new, FCSR_FFLAGS);
	__asm("fsflags %0, %1" : "=r"(__old) : "r"(__new));
	return __SHIFTOUT(__old, FCSR_FFLAGS);
}

static inline uint32_t
riscvreg_fcsr_read_frm(void)
{
	uint32_t __old;
	__asm("frrm\t%0" : "=r"(__old));
	return __SHIFTOUT(__old, FCSR_FRM);
}

static inline uint32_t
riscvreg_fcsr_write_frm(uint32_t __new)
{
	uint32_t __old;
	__new = __SHIFTIN(__new, FCSR_FRM);
	__asm volatile("fsrm\t%0, %1" : "=r"(__old) : "r"(__new));
	return __SHIFTOUT(__old, FCSR_FRM);
}

// Status Register
#define SR_IP		__BITS(31,24)	// Pending interrupts
#define SR_IM		__BITS(23,16)	// Interrupt Mask
#define SR_VM		__BIT(7)	// MMU On
#define SR_S64		__BIT(6)	// RV64 supervisor mode
#define SR_U64		__BIT(5)	// RV64 user mode
#define SR_EF		__BIT(4)	// Enable Floating Point
#define SR_PEI		__BIT(3)	// Previous EI setting
#define SR_EI		__BIT(2)	// Enable interrupts
#define SR_PS		__BIT(1)	// Previous (S) supervisor setting
#define SR_S		__BIT(0)	// Supervisor

#ifdef _LP64
#define	SR_USER		(SR_EI|SR_U64|SR_S64|SR_VM|SR_IM)
#define	SR_USER32	(SR_USER & ~SR_U64)
#define	SR_KERNEL	(SR_S|SR_EI|SR_U64|SR_S64|SR_VM)
#else
#define	SR_USER		(SR_EI|SR_VM|SR_IM)
#define	SR_KERNEL	(SR_S|SR_EI|SR_VM)
#endif

static inline uint32_t
riscvreg_status_read(void)
{
	uint32_t __sr;
	__asm("csrr\t%0, sstatus" : "=r"(__sr));
	return __sr;
}

static inline uint32_t
riscvreg_status_clear(uint32_t __mask)
{
	uint32_t __sr;
	if (__builtin_constant_p(__mask) && __mask < 0x20) {
		__asm("csrrci\t%0, sstatus, %1" : "=r"(__sr) : "i"(__mask));
	} else {
		__asm("csrrc\t%0, sstatus, %1" : "=r"(__sr) : "r"(__mask));
	}
	return __sr;
}

static inline uint32_t
riscvreg_status_set(uint32_t __mask)
{
	uint32_t __sr;
	if (__builtin_constant_p(__mask) && __mask < 0x20) {
		__asm("csrrsi\t%0, sstatus, %1" : "=r"(__sr) : "i"(__mask));
	} else {
		__asm("csrrs\t%0, sstatus, %1" : "=r"(__sr) : "r"(__mask));
	}
	return __sr;
}

// Cause register
#define CAUSE_MISALIGNED_FETCH		0
#define CAUSE_FAULT_FETCH		1
#define CAUSE_ILLEGAL_INSTRUCTION	2
#define CAUSE_PRIVILEGED_INSTRUCTION	3
#define CAUSE_MISALIGNED_LOAD		4
#define CAUSE_FAULT_LOAD		5
#define CAUSE_MISALIGNED_STORE		6
#define CAUSE_FAULT_STORE		7
#define CAUSE_SYSCALL			8
#define CAUSE_BREAKPOINT		9
#define CAUSE_FP_DISABLED		10
#define CAUSE_ACCELERATOR_DISABLED	12

static inline uint64_t
riscvreg_cycle_read(void)
{
#ifdef _LP64
	uint64_t __lo;
	__asm __volatile("csrr\t%0, cycle" : "=r"(__lo));
	return __lo;
#else
	uint32_t __hi0, __hi1, __lo0;
	do {
		__asm __volatile(
			"csrr\t%[__hi0], cycleh" 
		"\n\t"	"csrr\t%[__lo0], cycle"
		"\n\t"	"csrr\t%[__hi1], cycleh"
		   :	[__hi0] "=r"(__hi0),
			[__lo0] "=r"(__lo0),
			[__hi1] "=r"(__hi1));
	} while (__hi0 != __hi1);
	return ((uint64_t)__hi0 << 32) | (uint64_t)__lo0;
#endif
}

static inline uintptr_t
riscvreg_ptbr_read(void)
{
	uintptr_t __ptbr;
	__asm("csrr\t%0, sptbr" : "=r"(__ptbr));
	return __ptbr;
}

static inline void
riscvreg_ptbr_write(uint32_t __ptbr)
{
	__asm("csrw\tsptbr, %0" :: "r"(__ptbr));
}

static inline uint32_t
riscvreg_asid_read(void)
{
	uint32_t __asid;
	__asm __volatile("csrr\t%0, sasid" : "=r"(__asid));
	return __asid;
}

static inline void
riscvreg_asid_write(uint32_t __asid)
{
	__asm __volatile("csrw\tsasid, %0" :: "r"(__asid));
}

#endif /* _RISCV_SYSREG_H_ */
