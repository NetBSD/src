/*	$NetBSD: cpuregs.h,v 1.22 1999/05/21 06:37:39 nisimura Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machConst.h 8.1 (Berkeley) 6/10/93
 *
 * machConst.h --
 *
 *	Machine dependent constants.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machConst.h,
 *	v 9.2 89/10/21 15:55:22 jhh Exp	 SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAddrs.h,
 *	v 1.2 89/08/15 18:28:21 rab Exp	 SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/RCS/vmPmaxConst.h,
 *	v 9.1 89/09/18 17:33:00 shirriff Exp  SPRITE (DECWRL)
 */

#ifndef _MIPS_CPUREGS_H_
#define _MIPS_CPUREGS_H_

/*
 * Address space.
 * 32-bit mips CPUS partition their 32-bit address space into four segments:
 *
 * kuseg   0x00000000 - 0x7fffffff  User virtual mem,  mapped
 * kseg0   0x80000000 - 0x9fffffff  Physical memory, cached, unmapped
 * kseg1   0xa0000000 - 0xbfffffff  Physical memory, uncached, unmapped
 * kseg2   0xc0000000 - 0xffffffff  kernel-virtual,  mapped
 *
 * mips1 physical memory is limited to 512Mbytes, which is
 * doubly mapped in kseg0 (cached) and kseg1 (uncached.)
 * Caching of mapped addresses is controlled by bits in the TLB entry.
 */

#define MIPS_KUSEG_START		0x0
#define MIPS_KSEG0_START		0x80000000
#define MIPS_KSEG1_START		0xa0000000
#define MIPS_KSEG2_START		0xc0000000
#define MIPS_MAX_MEM_ADDR		0xbe000000
#define MIPS_RESERVED_ADDR		0xbfc80000

#define MIPS_KSEG0_TO_PHYS(x)	((unsigned)(x) & 0x1fffffff)
#define MIPS_PHYS_TO_KSEG0(x)	((unsigned)(x) | MIPS_KSEG0_START)
#define MIPS_KSEG1_TO_PHYS(x)	((unsigned)(x) & 0x1fffffff)
#define MIPS_PHYS_TO_KSEG1(x)	((unsigned)(x) | MIPS_KSEG1_START)

/* Map virtual address to index in mips3 r4k virtually-indexed cache */
#define MIPS3_VA_TO_CINDEX(x) \
		((unsigned)(x) & 0xffffff | MIPS_KSEG0_START)


/*
 * The bits in the cause register.
 *
 * Bits common to r3000 and r4000:
 *
 *	MIPS_CR_BR_DELAY	Exception happened in branch delay slot.
 *	MIPS_CR_COP_ERR		Coprocessor error.
 *	MIPS_CR_IP		Interrupt pending bits defined below.
 *				(same meaning as in CAUSE register).
 *	MIPS_CR_EXC_CODE	The exception type (see exception codes below).
 *
 * Differences:
 *  r3k has 4 bits of execption type, r4k has 5 bits.
 */
#define MIPS_CR_BR_DELAY	0x80000000
#define MIPS_CR_COP_ERR		0x30000000
#define MIPS1_CR_EXC_CODE	0x0000003C	/* four bits */
#define MIPS3_CR_EXC_CODE	0x0000007C	/* five bits */
#define MIPS_CR_IP		0x0000FF00
#define MIPS_CR_EXC_CODE_SHIFT	2

/*
 * The bits in the status register.  All bits are active when set to 1.
 *
 *	R3000 status register fields:
 *	MIPS_SR_CO_USABILITY	Control the usability of the four coprocessors.
 *	MIPS_SR_BOOT_EXC_VEC	Use alternate exception vectors.
 *	MIPS_SR_TLB_SHUTDOWN	TLB disabled.
 *
 *	MIPS_SR_INT_IE		Master (current) interrupt enable bit.
 *
 * Differences:
 *	r3k has cache control is via frobbing SR register bits, whereas the
 *	r4k cache control is via explicit instructions.
 *	r3k has a 3-entry stack of kernel/user bits, whereas the
 *	r4k has kernel/supervisor/user.
 */
#define MIPS_SR_COP_USABILITY	0xf0000000
#define MIPS_SR_COP_0_BIT	0x10000000
#define MIPS_SR_COP_1_BIT	0x20000000

	/* r4k and r3k differences, see below */

#define MIPS_SR_BOOT_EXC_VEC	0x00400000
#define MIPS_SR_TLB_SHUTDOWN	0x00200000

	/* r4k and r3k differences, see below */

#define MIPS_SR_INT_IE		0x00000001
/*#define MIPS_SR_MBZ		0x0f8000c0*/	/* Never used, true for r3k */
/*#define MIPS_SR_INT_MASK	0x0000ff00*/

#define MIPS_SR_INT_ENAB	MIPS_SR_INT_IE	/* backwards compatibility */
#define MIPS_SR_INT_ENA_CUR	MIPS_SR_INT_IE	/* backwards compatibility */



/*
 * The R2000/R3000-specific status register bit definitions.
 * all bits are active when set to 1.
 *
 *	MIPS_SR_PARITY_ERR	Parity error.
 *	MIPS_SR_CACHE_MISS	Most recent D-cache load resulted in a miss.
 *	MIPS_SR_PARITY_ZERO	Zero replaces outgoing parity bits.
 *	MIPS_SR_SWAP_CACHES	Swap I-cache and D-cache.
 *	MIPS_SR_ISOL_CACHES	Isolate D-cache from main memory.
 *				Interrupt enable bits defined below.
 *	MIPS_SR_KU_OLD		Old kernel/user mode bit. 1 => user mode.
 *	MIPS_SR_INT_ENA_OLD	Old interrupt enable bit.
 *	MIPS_SR_KU_PREV		Previous kernel/user mode bit. 1 => user mode.
 *	MIPS_SR_INT_ENA_PREV	Previous interrupt enable bit.
 *	MIPS_SR_KU_CUR		Current kernel/user mode bit. 1 => user mode.
 */

#define MIPS1_PARITY_ERR	0x00100000
#define MIPS1_CACHE_MISS	0x00080000
#define MIPS1_PARITY_ZERO	0x00040000
#define MIPS1_SWAP_CACHES	0x00020000
#define MIPS1_ISOL_CACHES	0x00010000

#define MIPS1_SR_KU_OLD		0x00000020	/* 2nd stacked KU/IE*/
#define MIPS1_SR_INT_ENA_OLD	0x00000010	/* 2nd stacked KU/IE*/
#define MIPS1_SR_KU_PREV	0x00000008	/* 1st stacked KU/IE*/
#define MIPS1_SR_INT_ENA_PREV	0x00000004	/* 1st stacked KU/IE*/
#define MIPS1_SR_KU_CUR		0x00000002	/* current KU */

/* backwards compatibility */
#define MIPS_SR_PARITY_ERR	MIPS1_PARITY_ERR
#define MIPS_SR_CACHE_MISS	MIPS1_CACHE_MISS
#define MIPS_SR_PARITY_ZERO	MIPS1_PARITY_ZERO
#define MIPS_SR_SWAP_CACHES	MIPS1_SWAP_CACHES
#define MIPS_SR_ISOL_CACHES	MIPS1_ISOL_CACHES

#define MIPS_SR_KU_OLD		MIPS1_SR_KU_OLD
#define MIPS_SR_INT_ENA_OLD	MIPS1_SR_INT_ENA_OLD
#define MIPS_SR_KU_PREV		MIPS1_SR_KU_PREV
#define MIPS_SR_KU_CUR		MIPS1_SR_KU_CUR
#define MIPS_SR_INT_ENA_PREV	MIPS1_SR_INT_ENA_PREV

/*
 * R4000 status register bit definitons,
 * where different from r2000/r3000.
 */
#define MIPS3_SR_XX		0x80000000
#define MIPS3_SR_RP		0x08000000
#define MIPS3_SR_FR_32		0x04000000
#define MIPS3_SR_RE		0x02000000

#define MIPS3_SR_SOFT_RESET	0x00100000
#define MIPS3_SR_DIAG_CH	0x00040000
#define MIPS3_SR_DIAG_CE	0x00020000
#define MIPS3_SR_DIAG_PE	0x00010000
#define MIPS3_SR_KX		0x00000080
#define MIPS3_SR_SX		0x00000040
#define MIPS3_SR_UX		0x00000020
#define MIPS3_SR_KSU_MASK	0x00000018
#define MIPS3_SR_KSU_USER	0x00000010
#define MIPS3_SR_KSU_SUPER	0x00000008
#define MIPS3_SR_KSU_KERNEL	0x00000000
#define MIPS3_SR_ERL		0x00000004
#define MIPS3_SR_EXL		0x00000002

#define MIPS_SR_SOFT_RESET	MIPS3_SR_SOFT_RESET
#define MIPS_SR_DIAG_CH		MIPS3_SR_DIAG_CH
#define MIPS_SR_DIAG_CE		MIPS3_SR_DIAG_CE
#define MIPS_SR_DIAG_PE		MIPS3_SR_DIAG_PE
#define MIPS_SR_KX		MIPS3_SR_KX
#define MIPS_SR_SX		MIPS3_SR_SX
#define MIPS_SR_UX		MIPS3_SR_UX

#define MIPS_SR_KSU_MASK	MIPS3_SR_KSU_MASK
#define MIPS_SR_KSU_USER	MIPS3_SR_KSU_USER
#define MIPS_SR_KSU_SUPER	MIPS3_SR_KSU_SUPER
#define MIPS_SR_KSU_KERNEL	MIPS3_SR_KSU_KERNEL
#define MIPS_SR_ERL		MIPS3_SR_ERL
#define MIPS_SR_EXL		MIPS3_SR_EXL


/*
 * The interrupt masks.
 * If a bit in the mask is 1 then the interrupt is enabled (or pending).
 */
#define MIPS_INT_MASK		0xff00
#define MIPS_INT_MASK_5		0x8000
#define MIPS_INT_MASK_4		0x4000
#define MIPS_INT_MASK_3		0x2000
#define MIPS_INT_MASK_2		0x1000
#define MIPS_INT_MASK_1		0x0800
#define MIPS_INT_MASK_0		0x0400
#define MIPS_HARD_INT_MASK	0xfc00
#define MIPS_SOFT_INT_MASK_1	0x0200
#define MIPS_SOFT_INT_MASK_0	0x0100

/*
 * mips3 CPUs have on-chip timer at INT_MASK_5. We don't support it yet.
 */
#define MIPS3_INT_MASK			(MIPS_INT_MASK &  ~MIPS_INT_MASK_5)
#define MIPS3_HARD_INT_MASK		(MIPS_HARD_INT_MASK & ~MIPS_INT_MASK_5)


/*
 * The bits in the context register.
 */
#define MIPS1_CNTXT_PTE_BASE	0xFFE00000
#define MIPS1_CNTXT_BAD_VPN	0x001FFFFC

#define MIPS3_CNTXT_PTE_BASE	0xFF800000
#define MIPS3_CNTXT_BAD_VPN2	0x007FFFF0

/*
 * The bits in the MIPS3 config register.
 *
 *	bit 0..5: R/W, Bit 6..31: R/O
 */

/* kseg0 coherency algorithm - see MIPS3_TLB_ATTR values */
#define MIPS3_CONFIG_K0_MASK	0x00000007

/*
 * R/W Update on Store Conditional
 *	0: Store Conditional uses coherency algorithm specified by TLB
 *	1: Store Conditional uses cacheable coherent update on write
 */
#define MIPS3_CONFIG_CU		0x00000008

#define MIPS3_CONFIG_DB		0x00000010	/* Primary D-cache line size */
#define MIPS3_CONFIG_IB		0x00000020	/* Primary I-cache line size */
#define MIPS3_CONFIG_CACHE_L1_LSIZE(config, bit) \
	(((config) & (bit)) ? 32 : 16)

#define MIPS3_CONFIG_DC_MASK	0x000001c0	/* Primary D-cache size */
#define MIPS3_CONFIG_DC_SHIFT	6
#define MIPS3_CONFIG_IC_MASK	0x00000e00	/* Primary I-cache size */
#define MIPS3_CONFIG_IC_SHIFT	9
#define MIPS3_CONFIG_CACHE_SIZE(config, mask, shift) \
	(0x1000 << (((config) & (mask)) >> (shift)))

/* Block ordering: 0: sequential, 1: sub-block */
#define MIPS3_CONFIG_EB		0x00002000

/* ECC mode - 0: ECC mode, 1: parity mode */
#define MIPS3_CONFIG_EM		0x00004000

/* BigEndianMem - 0: kernel and memory are little endian, 1: big endian */
#define MIPS3_CONFIG_BE		0x00008000

/* Dirty Shared coherency state - 0: enabled, 1: disabled */
#define MIPS3_CONFIG_SM		0x00010000

/* Secondary Cache - 0: present, 1: not present */
#define MIPS3_CONFIG_SC		0x00020000

/* System Port width - 0: 64-bit, 1,2,3: reserved */
#define MIPS3_CONFIG_EW_MASK	0x000c0000
#define MIPS3_CONFIG_EW_SHIFT	18

/* Secondary Cache port width - 0: 128-bit data path to S-cache, 1: reserved */
#define MIPS3_CONFIG_SW		0x00100000

/* Split Secondary Cache Mode - 0: I/D mixed, 1: I/D separated by SCAddr(17) */
#define MIPS3_CONFIG_SS		0x00200000

/* Secondary Cache line size */
#define MIPS3_CONFIG_SB_MASK	0x00c00000
#define MIPS3_CONFIG_SB_SHIFT	22
#define MIPS3_CONFIG_CACHE_L2_LSIZE(config) \
	(0x10 << (((config) & MIPS3_CONFIG_SB_MASK) >> MIPS3_CONFIG_SB_SHIFT))

/* write back data rate */
#define MIPS3_CONFIG_EP_MASK	0x0f000000
#define MIPS3_CONFIG_EP_SHIFT	24

/* System clock ratio - this value is CPU dependent */
#define MIPS3_CONFIG_EC_MASK	0x70000000
#define MIPS3_CONFIG_EC_SHIFT	28

/* Master-Checker Mode - 1: enabled */
#define MIPS3_CONFIG_CM		0x80000000

/*
 * Location of exception vectors.
 *
 * Common vectors:  reset and UTLB miss.
 */
#define MIPS_RESET_EXC_VEC	0xBFC00000
#define MIPS_UTLB_MISS_EXC_VEC	0x80000000

/*
 * R3000 general exception vector (everything else)
 */
#define MIPS1_GEN_EXC_VEC	0x80000080

/*
 * R4000 MIPS-III exception vectors
 */
#define MIPS3_XTLB_MISS_EXC_VEC 0x80000080
#define MIPS3_CACHE_ERR_EXC_VEC 0x80000100
#define MIPS3_GEN_EXC_VEC	0x80000180

/*
 * Coprocessor 0 registers:
 *
 *	MIPS_COP_0_TLB_INDEX	TLB index.
 *	MIPS_COP_0_TLB_RANDOM	TLB random.
 *	MIPS_COP_0_TLB_LOW	r3k TLB entry low.
 *	MIPS_COP_0_TLB_LO0	r4k TLB entry low.
 *	MIPS_COP_0_TLB_LO1	r4k TLB entry low, extended.
 *	MIPS_COP_0_TLB_CONTEXT	TLB context.
 *	MIPS_COP_0_BAD_VADDR	Bad virtual address.
 *	MIPS_COP_0_TLB_HI	TLB entry high.
 *	MIPS_COP_0_STATUS	Status register.
 *	MIPS_COP_0_CAUSE	Exception cause register.
 *	MIPS_COP_0_EXC_PC	Exception PC.
 *	MIPS_COP_0_PRID		Processor revision identifier.
 */
#define MIPS_COP_0_TLB_INDEX	$0
#define MIPS_COP_0_TLB_RANDOM	$1
	/* Name and meaning of	TLB bits for $2 differ on r3k and r4k. */

#define MIPS_COP_0_TLB_CONTEXT	$4
					/* $5 and $6 new with MIPS-III */
#define MIPS_COP_0_BAD_VADDR	$8
#define MIPS_COP_0_TLB_HI	$10
#define MIPS_COP_0_STATUS_REG	$12
#define MIPS_COP_0_CAUSE_REG	$13
#define MIPS_COP_0_STATUS	$12
#define MIPS_COP_0_CAUSE	$13
#define MIPS_COP_0_EXC_PC	$14
#define MIPS_COP_0_PRID		$15


/* MIPS-I */
#define MIPS_COP_0_TLB_LOW	$2

/* MIPS-III */
#define MIPS_COP_0_TLB_LO0	$2
#define MIPS_COP_0_TLB_LO1	$3

#define MIPS_COP_0_TLB_PG_MASK	$5
#define MIPS_COP_0_TLB_WIRED	$6

#define MIPS_COP_0_COUNT	$9
#define MIPS_COP_0_COMPARE	$11

#define MIPS_COP_0_CONFIG	$16
#define MIPS_COP_0_LLADDR	$17
#define MIPS_COP_0_WATCH_LO	$18
#define MIPS_COP_0_WATCH_HI	$19
#define MIPS_COP_0_TLB_XCONTEXT $20
#define MIPS_COP_0_ECC		$26
#define MIPS_COP_0_CACHE_ERR	$27
#define MIPS_COP_0_TAG_LO	$28
#define MIPS_COP_0_TAG_HI	$29
#define MIPS_COP_0_ERROR_PC	$30



/*
 * Values for the code field in a break instruction.
 */
#define MIPS_BREAK_INSTR	0x0000000d
#define MIPS_BREAK_VAL_MASK	0x03ff0000
#define MIPS_BREAK_VAL_SHIFT	16
#define MIPS_BREAK_KDB_VAL	512
#define MIPS_BREAK_SSTEP_VAL	513
#define MIPS_BREAK_BRKPT_VAL	514
#define MIPS_BREAK_SOVER_VAL	515
#define MIPS_BREAK_KDB		(MIPS_BREAK_INSTR | \
				(MIPS_BREAK_KDB_VAL << MIPS_BREAK_VAL_SHIFT))
#define MIPS_BREAK_SSTEP	(MIPS_BREAK_INSTR | \
				(MIPS_BREAK_SSTEP_VAL << MIPS_BREAK_VAL_SHIFT))
#define MIPS_BREAK_BRKPT	(MIPS_BREAK_INSTR | \
				(MIPS_BREAK_BRKPT_VAL << MIPS_BREAK_VAL_SHIFT))
#define MIPS_BREAK_SOVER	(MIPS_BREAK_INSTR | \
				(MIPS_BREAK_SOVER_VAL << MIPS_BREAK_VAL_SHIFT))

/*
 * Mininum and maximum cache sizes.
 */
#define MIPS_MIN_CACHE_SIZE	(16 * 1024)
#define MIPS_MAX_CACHE_SIZE	(256 * 1024)

/*
 * The floating point version and status registers.
 */
#define MIPS_FPU_ID	$0
#define MIPS_FPU_CSR	$31

/*
 * The floating point coprocessor status register bits.
 */
#define MIPS_FPU_ROUNDING_BITS		0x00000003
#define MIPS_FPU_ROUND_RN		0x00000000
#define MIPS_FPU_ROUND_RZ		0x00000001
#define MIPS_FPU_ROUND_RP		0x00000002
#define MIPS_FPU_ROUND_RM		0x00000003
#define MIPS_FPU_STICKY_BITS		0x0000007c
#define MIPS_FPU_STICKY_INEXACT		0x00000004
#define MIPS_FPU_STICKY_UNDERFLOW	0x00000008
#define MIPS_FPU_STICKY_OVERFLOW	0x00000010
#define MIPS_FPU_STICKY_DIV0		0x00000020
#define MIPS_FPU_STICKY_INVALID		0x00000040
#define MIPS_FPU_ENABLE_BITS		0x00000f80
#define MIPS_FPU_ENABLE_INEXACT		0x00000080
#define MIPS_FPU_ENABLE_UNDERFLOW	0x00000100
#define MIPS_FPU_ENABLE_OVERFLOW	0x00000200
#define MIPS_FPU_ENABLE_DIV0		0x00000400
#define MIPS_FPU_ENABLE_INVALID		0x00000800
#define MIPS_FPU_EXCEPTION_BITS		0x0003f000
#define MIPS_FPU_EXCEPTION_INEXACT	0x00001000
#define MIPS_FPU_EXCEPTION_UNDERFLOW	0x00002000
#define MIPS_FPU_EXCEPTION_OVERFLOW	0x00004000
#define MIPS_FPU_EXCEPTION_DIV0		0x00008000
#define MIPS_FPU_EXCEPTION_INVALID	0x00010000
#define MIPS_FPU_EXCEPTION_UNIMPL	0x00020000
#define MIPS_FPU_COND_BIT		0x00800000
#define MIPS_FPU_FLUSH_BIT		0x01000000	/* r4k,	 MBZ on r3k */
#define MIPS1_FPC_MBZ_BITS		0xff7c0000
#define MIPS3_FPC_MBZ_BITS		0xfe7c0000


/*
 * Constants to determine if have a floating point instruction.
 */
#define MIPS_OPCODE_SHIFT	26
#define MIPS_OPCODE_C1		0x11



/*
 * The low part of the TLB entry.
 */
#define MIPS1_TLB_PFN			0xfffff000
#define MIPS1_TLB_NON_CACHEABLE_BIT	0x00000800
#define MIPS1_TLB_DIRTY_BIT		0x00000400
#define MIPS1_TLB_VALID_BIT		0x00000200
#define MIPS1_TLB_GLOBAL_BIT		0x00000100

#define MIPS3_TLB_PFN			0x3fffffc0
#define MIPS3_TLB_ATTR_MASK		0x00000038
#define MIPS3_TLB_ATTR_SHIFT		3
#define MIPS3_TLB_DIRTY_BIT		0x00000004
#define MIPS3_TLB_VALID_BIT		0x00000002
#define MIPS3_TLB_GLOBAL_BIT		0x00000001

/* XXX XXX XXX */
#define MIPS1_TLB_PHYS_PAGE_SHIFT	12
#define MIPS3_TLB_PHYS_PAGE_SHIFT	6
#define MIPS1_TLB_PF_NUM		MIPS1_TLB_PFN
#define MIPS3_TLB_PF_NUM		MIPS3_TLB_PFN
#define MIPS1_TLB_MOD_BIT		MIPS1_TLB_DIRTY_BIT
#define MIPS3_TLB_MOD_BIT		MIPS3_TLB_DIRTY_BIT
/* XXX XXX XXX */

/*
 * MIPS3_TLB_ATTR values - coherency algorithm:
 * 0: cacheable, noncoherent, write-through, no write allocate
 * 1: cacheable, noncoherent, write-through, write allocate
 * 2: uncached
 * 3: cacheable, noncoherent, write-back (noncoherent)
 * 4: cacheable, coherent, write-back, exclusive (exclusive)
 * 5: cacheable, coherent, write-back, exclusive on write (sharable)
 * 6: cacheable, coherent, write-back, update on write (update)
 * 7: uncached, accelerated (gather STORE operations)
 */
#define MIPS3_TLB_ATTR_WT		0 /* IDT */
#define MIPS3_TLB_ATTR_WT_WRITEALLOCATE 1 /* IDT */
#define MIPS3_TLB_ATTR_UNCACHED		2 /* R4000/R4400, IDT */
#define MIPS3_TLB_ATTR_WB_NONCOHERENT	3 /* R4000/R4400, IDT */
#define MIPS3_TLB_ATTR_WB_EXCLUSIVE	4 /* R4000/R4400 */
#define MIPS3_TLB_ATTR_WB_SHARABLE	5 /* R4000/R4400 */
#define MIPS3_TLB_ATTR_WB_UPDATE	6 /* R4000/R4400 */
#define MIPS4_TLB_ATTR_UNCACHED_ACCELERATED 7 /* R10000 */


/*
 * The high part of the TLB entry.
 */
#define MIPS1_TLB_VPN			0xfffff000
#define MIPS1_TLB_PID			0x00000fc0
#define MIPS1_TLB_PID_SHIFT		6

#define MIPS3_TLB_VPN2			0xffffe000
#define MIPS3_TLB_ASID			0x000000ff

/* XXX XXX XXX */
#define MIPS1_TLB_VIRT_PAGE_NUM		MIPS1_TLB_VPN
#define MIPS3_TLB_VIRT_PAGE_NUM		MIPS3_TLB_VPN2
#define MIPS3_TLB_PID			MIPS3_TLB_ASID
#define MIPS_TLB_VIRT_PAGE_SHIFT	12
/* XXX XXX XXX */

/*
 * r3000: shift count to put the index in the right spot.
 */
#define MIPS1_TLB_INDEX_SHIFT		8

/*
 * The number of TLB entries and the first one that write random hits.
 */
#define MIPS1_TLB_NUM_TLB_ENTRIES	64
#define MIPS1_TLB_FIRST_RAND_ENTRY	8

#define MIPS3_TLB_NUM_TLB_ENTRIES	48
#define MIPS_R4300_TLB_NUM_TLB_ENTRIES	32
#define MIPS3_TLB_WIRED_ENTRIES		8	/* XXX gross XXX */


/*
 * The number of process id entries.
 */
#define MIPS1_TLB_NUM_PIDS		64
#define MIPS3_TLB_NUM_ASIDS		256

/*
 * Patch codes to hide CPU design differences between MIPS1 and MIPS3.
 *
 * XXX INT_MASK and HARD_INT_MASK are here only because we dont
 * support the mips3 on-chip timer which is tied to INT_5.
 */

#if !defined(MIPS3) && defined(MIPS1)
#define MIPS_TLB_PID_SHIFT		MIPS1_TLB_PID_SHIFT
#define MIPS_TLB_NUM_PIDS		MIPS1_TLB_NUM_PIDS
#endif

#if defined(MIPS3) && !defined(MIPS1)
#define MIPS_TLB_PID_SHIFT		0
#define MIPS_TLB_NUM_PIDS		MIPS3_TLB_NUM_ASIDS
#endif


#if defined(MIPS1) && defined(MIPS3)
#define MIPS_TLB_PID_SHIFT \
    ((CPUISMIPS3)? 0 : MIPS1_TLB_PID_SHIFT)

#define MIPS_TLB_NUM_PIDS \
    ((CPUISMIPS3)? MIPS3_TLB_NUM_ASIDS : MIPS1_TLB_NUM_PIDS)

#endif

/*
 * CPU processor revision ID
 */
#define MIPS_R2000	0x01	/* MIPS R2000 CPU		ISA I	*/
#define MIPS_R3000	0x02	/* MIPS R3000 CPU		ISA I	*/
#define MIPS_R6000	0x03	/* MIPS R6000 CPU		ISA II	*/
#define MIPS_R4000	0x04	/* MIPS R4000/4400 CPU		ISA III */
#define MIPS_R3LSI	0x05	/* LSI Logic R3000 derivate	ISA I	*/
#define MIPS_R6000A	0x06	/* MIPS R6000A CPU		ISA II	*/
#define MIPS_R3IDT	0x07	/* IDT R3041 or RC36100 CPU	ISA I	*/
#define MIPS_R10000	0x09	/* MIPS R10000/T5 CPU		ISA IV	*/
#define MIPS_R4200	0x0a	/* NEC VR4200 CPU		ISA III */
#define MIPS_R4300	0x0b	/* NEC VR4300 CPU		ISA III */
#define MIPS_R4100	0x0c	/* NEC VR4100 CPU		ISA III */
#define MIPS_R8000	0x10	/* MIPS R8000 Blackbird/TFP	ISA IV	*/
#define MIPS_R4600	0x20	/* QED R4600 Orion		ISA III */
#define MIPS_R4700	0x21	/* QED R4700 Orion		ISA III */
#define MIPS_R4650	0x22	/* !ID crash! QED R4650 CPU	ISA III */
#define MIPS_TX3900	0x22	/* !ID crash! Toshiba R3000 CPU ISA I	*/
#define MIPS_R5000	0x23	/* MIPS R5000 CPU		ISA IV	*/
#define MIPS_RC32364	0x26	/* IDT RC32364 CPU		ISA II	*/
#define MIPS_RM5230	0x28	/* QED RM5230 CPU		ISA IV	*/
#define MIPS_RC64470	0x30	/* IDT RC64474/RC64475 CPU	ISA III */
#define MIPS_R3SONY	0x21	/* ? Sony R3000 based CPU	ISA I	*/
#define MIPS_R3NKK	0x23	/* ? NKK R3000 based CPU	ISA I	*/
#define MIPS_R5400	0x54	/* NEC VR5400 CPU		ISA IV	*/

/*
 * FPU processor revision ID
 */
#define MIPS_SOFT	0x00	/* Software emulation		ISA I	*/
#define MIPS_R2360	0x01	/* MIPS R2360 FPC		ISA I	*/
#define MIPS_R2010	0x02	/* MIPS R2010 FPC		ISA I	*/
#define MIPS_R3010	0x03	/* MIPS R3010 FPC		ISA I	*/
#define MIPS_R6010	0x04	/* MIPS R6010 FPC		ISA II	*/
#define MIPS_R4010	0x05	/* MIPS R4010 FPC		ISA II	*/
#define MIPS_R31LSI	0x06	/* LSI Logic derivate		ISA I	*/
#define MIPS_R10010	0x09	/* MIPS R10000/T5 FPU		ISA IV	*/
#define MIPS_R4210	0x0a	/* NEC VR4210 FPC		ISA III */
#define MIPS_R4300	0x0b	/* NEC VR4300 FPC		ISA III */
#define MIPS_R8000	0x10	/* MIPS R8000 Blackbird/TFP	ISA IV	*/
#define MIPS_R4600	0x20	/* QED R4600 Orion FPU		ISA III */
#define MIPS_R5010	0x23	/* MIPS R5000 FPU		ISA IV	*/
#define MIPS_RC32364	0x26	/* IDT RC32364 FPU		ISA II	*/
#define MIPS_RM5230	0x28	/* QED RM5230 FPU		ISA IV	*/
#define MIPS_R3SONY	0x21	/* ? Sony R3000 based FPU	ISA I	*/
#define MIPS_R3TOSH	0x22	/* ? Toshiba R3000 based FPU	ISA I	*/
#define MIPS_R3NKK	0x23	/* ? NKK R3000 based FPU	ISA I	*/

#endif /* _MIPS_CPUREGS_H_ */
