/* $NetBSD: sysreg.h,v 1.10 2020/11/04 20:05:47 skrll Exp $ */

/*
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
#define FCSR_FRM_DYN	0b111	// Dynamic rounding
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

/* Supervisor Status Register */
#ifdef _LP64
#define SR_WPRI		__BITS(62, 34) | __BITS(31,20) | __BIT(17) | \
			    __BITS(12,9) | __BITS(7,6) | __BITS(3,2)
#define SR_SD		__BIT(63)
			/* Bits 62-34 are WPRI */
#define SR_UXL		__BITS(33,32)
#define  SR_UXL_32	1
#define  SR_UXL_64	2
#define  SR_UXL_128	3
			/* Bits 31-20 are WPRI*/
#else
#define SR_WPRI		__BITS(30,20) | __BIT(17) | __BITS(12,9) | \
			    __BITS(7,6) | __BITS(3,2)
#define SR_SD		__BIT(31)
			/* Bits 30-20 are WPRI*/
#endif /* _LP64 */

/* Both RV32 and RV64 have the bottom 20 bits shared */
#define SR_MXR		__BIT(19)
#define SR_SUM		__BIT(18)
			/* Bit 17 is WPRI */
#define SR_XS		__BITS(16,15)
#define SR_FS		__BITS(14,13)
#define  SR_FS_OFF	0
#define  SR_FS_INITIAL	1
#define  SR_FS_CLEAN	2
#define  SR_FS_DIRTY	3

			/* Bits 12-9 are WPRI */
#define SR_SPP		__BIT(8)
			/* Bits 7-6 are WPRI */
#define SR_SPIE		__BIT(5)
#define SR_UPIE		__BIT(4)
			/* Bits 3-2 are WPRI */
#define SR_SIE		__BIT(1)
#define SR_UIE		__BIT(0)

/* Supervisor interrupt registers */
/* ... interupt pending register (sip) */
			/* Bit (XLEN-1)-10 is WIRI */
#define SIP_SEIP	__BIT(9)
#define SIP_UEIP	__BIT(8)
			/* Bit 7-6 is WIRI */
#define SIP_STIP	__BIT(5)
#define SIP_UTIP	__BIT(4)
			/* Bit 3-2 is WIRI */
#define SIP_SSIP	__BIT(1)
#define SIP_USIP	__BIT(0)

/* ... interupt-enable register (sie) */
			/* Bit (XLEN-1) - 10 is WIRI */
#define SIE_SEIE	__BIT(9)
#define SIE_UEIE	__BIT(8)
			/* Bit 7-6 is WIRI */
#define SIE_STIE	__BIT(5)
#define SIE_UTIE	__BIT(4)
			/* Bit 3-2 is WIRI */
#define SIE_SSIE	__BIT(1)
#define SIE_USIE	__BIT(0)

/* Mask for all interrupts */
#define SIE_IM		(SIE_SEI|SIE_UEIE|SIE_STIE|SIE_UTIE|SIE_SSIE|SIE_USIE)

#ifdef _LP64
#define	SR_USER		(SR_UIE)
#define	SR_USER32	(SR_USER)
#define	SR_KERNEL	(SR_SIE | SR_UIE)
#else
#define	SR_USER		(SR_UIE)
#define	SR_KERNEL	(SR_SIE | SR_UIE)
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
#define CAUSE_FETCH_MISALIGNED		0
#define CAUSE_FETCH_ACCESS		1
#define CAUSE_ILLEGAL_INSTRUCTION	2
#define CAUSE_BREAKPOINT		3
#define CAUSE_LOAD_MISALIGNED		4
#define CAUSE_LOAD_ACCESS		5
#define CAUSE_STORE_MISALIGNED		6
#define CAUSE_STORE_ACCESS		7
#define CAUSE_SYSCALL			8
#define CAUSE_USER_ECALL		8
#define CAUSE_SUPERVISOR_ECALL		9
/* 10 is reserved */
#define CAUSE_MACHINE_ECALL		11
#define CAUSE_FETCH_PAGE_FAULT		12
#define CAUSE_LOAD_PAGE_FAULT		13
/* 14 is Reserved */
#define CAUSE_STORE_PAGE_FAULT		15
/* >= 16 is reserved */

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

#ifdef _LP64
#define SATP_MODE		__BITS(63,60)
#define  SATP_MODE_SV39		8
#define  SATP_MODE_SV48		9
#define SATP_ASID		__BITS(59,44)
#define SATP_PPN		__BITS(43,0)
#else
#define SATP_MODE		__BIT(31)
#define  SATP_MODE_SV32		1
#define SATP_ASID		__BITS(30,22)
#define SATP_PPN		__BITS(21,0)
#endif

static inline uint32_t
riscvreg_asid_read(void)
{
	uintptr_t satp;
	__asm __volatile("csrr	%0, satp" : "=r" (satp));
	return __SHIFTOUT(satp, SATP_ASID);
}

static inline void
riscvreg_asid_write(uint32_t asid)
{
	uintptr_t satp;
	__asm __volatile("csrr	%0, satp" : "=r" (satp));
	satp &= ~SATP_ASID;
	satp |= __SHIFTIN((uintptr_t)asid, SATP_ASID);
	__asm __volatile("csrw	satp, %0" :: "r" (satp));
}

#endif /* _RISCV_SYSREG_H_ */
