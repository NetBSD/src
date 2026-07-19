/*	$NetBSD: pgmmu.h,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _PG68K_PGMMU_H_
#define	_PG68K_PGMMU_H_

/*
 * Definitions for the Phaethon 1 MMU.
 *
 * The Phaethon 1 MMU is inspired by the Sun3 MMU (which is itself similar
 * to the Sun2 MMU), but has some differences.  Here are the features of
 * the Phaethon 1's MMU:
 *
 * ==> 24 bits of virtual address (16MB, as limited by the 68010) is mapped
 *     onto 28 bits (256MB) of physical address.
 *
 * ==> 64 contexts; kernel accessess all use context 0, leaving 1-63
 *     available for user processes.  There is no restriction against
 *     using context 0 for user processes, and there is per-page kernel
 *     privilege protection in case you do.
 *
 * ==> 2-level address translation: each context is comprised of 512
 *     32KB segments, and each segment is comprised of 8 4KB pages.
 *
 * ==> A total of 262,144 32-bit Page Map entries.  Because of the 8-entry
 *     grouping, there are a total of 32,768 Page Map Entry Groups (PMEGs)
 *     shared by all contexts.  Thus, each Segment Map entry is 16 bits
 *     (15-bit PMEG index plus a VALID bit).
 *
 *     This is a key difference from the Sun MMU.  On the Sun MMU, PMEGs
 *     were a fairly scarce resource.  Here, there are a lot of them...
 *     in fact, enough to fully map the entirety of all 64 contexts
 *     (32,768 / 64 == 512).  This implies that we shouldn't burn too
 *     much memory on a per-PMEG basis to manage them.
 *
 * ==> Independent access to the Page Map entries.  This is a key difference
 *     from the Sun MMU, which always uses the context register + Segment
 *     Map to address Page Map entries.  The Phaethon 1's MMU has an address
 *     mux in front of the Page Map, which selects where the Page Map entry
 *     index comes from.
 *
 * ==> Protection on a per-page basis: K (kernel privilege required) and
 *     W (writable).
 *
 * ==> Page Referenced and Modified tracking.
 *
 * ==> Fast access to the kernel segment map via a dedicated address range
 *     for this purpose.
 *
 * ==> Separate reporting for privilege (user access to kernel-only page)
 *     and protection (write access to read-only page) violations, with
 *     privilege violation reporting taking priority.
 *
 * ==> Centralized bus error handling for the whole system, including a
 *     a bus cycle watchdog timer.
 *
 * ==> MMU register access via FC#4.
 *
 * ==> Separate MMU-enable input signal.  System board logic independent
 *     of the MMU is responsible for enabling the MMU post-reset and also
 *     dictates address decoding behvior before the MMU is enabled.  On
 *     all current implementations (heh), all {User,Super} {Data,Prog}
 *     cycles select the system ROM, which is reponsible for configuring
 *     and enabling the MMU.
 *
 * The MMU itself occupies about 1/3 of the board and sits between the CPU
 * and the rest of the system.  It is comprised of:
 *
 *	- A Microchip ATF1508AS-7AX100 7.5ns CPLD, logic written in
 *	  in Verilog and synthesized with Yosys.  This runs the bus
 *	  cycle state machine, houses the MMU's registers, computes
 *	  page faults, and swizzles the control signals for the MMU's
 *	  SRAMs.
 *	- 1x IS61C3216AL (32K x 16 12ns SRAM) for the SegMap.
 *	- 2x AS7C4098A (256K x 16 12ns SRAM) for the PageMap.
 *	- 10x 74AHCT157 quad 2-to-1 muxes to mux various address
 *	  inputs and outputs.
 *	- 3x 74ACHT373 octal transparent latch for the PageMap
 *	  index that in retrospect probably weren't necessary.
 *	- 6x 74AHCT245 8-bit bus transceivers for CPU data bus
 *	  interface to the SegMap and PageMap.
 *
 * The MMU runs on the main oscillator and generates the system clock
 * by dividing the main oscillator by 4.  This is typically a 40MHz
 * main oscillator, generating a 10MHz system clock.  No wait states
 * are incurred by the MMU.
 *
 * Address translation:
 *
 * +---------------+---------------------+
 * | Context [5:0] | Segment (VA[23:15]) |
 * +---------------+---------------------+--> 15-bit SegMap index --+
 *                                                                  |
 * +----------------------------------------------------------------+
 * |
 * +--> SegMap entry contains 15-bit PMEG number --+
 *                                                 |
 * +-----------------------------------------------+
 * |
 * +-------------------------+
 * |      PageMap index      |
 * | (PMEG << 3) | VA[14:12] |
 * +-------------------------+--> 18-bit PageMap index --+
 *                                                       |
 * +-----------------------------------------------------+
 * |
 * +--> PageMap entry (32-bits) (lower 16 bits are PFN) -----+
 *                                                           |
 * +---------------------------------------------------------+
 * |
 * +--> (PFN << 12) | VA[11:1] -> Physical address
 *
 * Format of a Page Map entry:
 *
 *  31          28   26         23         20 19 16 15                0
 * | V | W | K |  (r)  | R | M | s3 s2 s1 s0 | (r) | Page Frame Number |
 * | a   r   e     e     e   o   (software      e
 * | l   i   r     s     f   d      defined)    s
 * | i   t   n     e     e   i                  e
 * | d   e   e     r     r   f                  r
 *           l     v     e   i                  v
 *                 e     n   e                  e
 *                 d     c   d                  d
 *                       e
 *                       d
 */

#define	PGMMU_NUM_CONTEXTS	64	/* 64 total contexts */
#define	PGMMU_CONTEXT_SUPER	0	/* supervisor is always context 0 */

#define	PGMMU_PAGE_SHIFT	12	/* 4KB pages */
#define	PGMMU_PAGE_SIZE		(1U << PGMMU_PAGE_SHIFT)
#define	PGMMU_PAGE_OFFSET	(PGMMU_PAGE_SIZE - 1)
#define	PGMMU_PAGE_MASK		(~PGMMU_PAGE_OFFSET)

#define	PGMMU_NUM_SEGS		512	/* 512 segments per context */
#define	PGMMU_SEG_SHIFT		15	/* 32KB segments */
					/* == 16MB - go go 68010! */
#define	PGMMU_SEG_SIZE		(1U << PGMMU_SEG_SHIFT)
#define	PGMMU_SEG_OFFSET	(PGMMU_SEG_SIZE - 1)
#define	PGMMU_SEG_MASK		(~PGMMU_SEG_OFFSET)

#define	PGMMU_NUM_PMEGS		32768	/* 32K total Page Map Entry Groups */
#define	PGMMU_PMEG_SHIFT	3	/* 8 PMEs per PMEG */
#define	PGMMU_PMEG_SIZE		(1U << PGMMU_PMEG_SHIFT)
#define	PGMMU_PMEG_OFFSET	(PGMMU_PMEG_SIZE - 1)
#define	PGMMU_NUM_PMES		(PGMMU_NUM_PMEGS << PGMMU_PMEG_SHIFT)

#define	pgmmu_btos(va)		((vaddr_t)(va) >> PGMMU_SEG_SHIFT)
#define	pgmmu_stob(s)		((vaddr_t)(s) << PGMMU_SEG_SHIFT)
#define	pgmmu_trunc_seg(va)	((vaddr_t)(va) & PGMMU_SEG_MASK)
#define	pgmmu_round_seg(va)	pgmmu_trunc_seg((va) + PGMMU_SEG_OFFSET)
#define	pgmmu_next_seg(va)	pgmmu_round_seg((va) + PGMMU_PAGE_SIZE)

#define	pgmmu_btop(va)		((vaddr_t)(va) >> PGMMU_PAGE_SHIFT)
#define	pgmmu_ptob(p)		((vaddr_t)(p) << PGMMU_PAGE_SHIFT)
#define	pgmmu_trunc_page(va)	((vaddr_t)(va) & PGMMU_PAGE_MASK)
#define	pgmmu_round_page(va)	pgmmu_trunc_page((va) + PGMMU_PAGE_OFFSET)

/* Segment Map entry: */
typedef uint16_t sm_entry_t;
#define	SME_PMEG	__BITS(0,14)	/* PMEG for segment */
#define	SME_V		__BIT(15)	/* PMEG is valid */

/* Page Map entry: */
typedef uint32_t pm_entry_t;
#define	PME_PFN		__BITS(0,15)
	/* 16-19			   reserved */
#define	PME_SW0		__BIT(20)	/* software bit 0 */
#define	PME_SW1		__BIT(21)	/* software bit 1 */
#define	PME_SW2		__BIT(22)	/* software bit 2 */
#define	PME_SW3		__BIT(23)	/* software bit 3 */
#define	PME_M		__BIT(24)	/* modified */
#define	PME_R		__BIT(25)	/* referenced */
	/* 26-28			   reserved */
#define	PME_K		__BIT(29)	/* kernel priv required */
#define	PME_W		__BIT(30)	/* writable page */
#define	PME_V		__BIT(31)	/* valid mapping */

#define	PME_WIRED	PME_SW0		/* wired mapping */
#define	PME_PVLIST	PME_SW1		/* managed mapping */

/*
 * FC#4 is used by the MMU as "control space".  CPU address bits A3..A1
 * are used as a register selector, and the space is decoded like so:
 *
 *                          selector bits
 *                               vvv
 *      xxxx.xxxx xxxx.xxxx xxxx.000x   non-MMU control space (ignored)
 *      SSSS.SSSS Sxxx.xxxx xxxx.0010   SegMap entry for Context 0
 *      SSSS.SSSS Sxxx.xxxx xxxx.0100   SegMap entry (relative to Context Reg)
 *      xxxx.xxxx xxxx.xxxx xxxx.0110   Context Register (byte)
 *      xxGG.GGGG GGGG.GGGG GEEE.1000   PageMap entry (upper word)
 *      xxGG.GGGG GGGG.GGGG GEEE.1010   PageMap entry (lower word)
 *        ^^^^^^^^^^^^^^^^^^^|||
 *                 PMEG      |||
 *                           ^^^
 *                    Entry within PMEG
 *      xxxx.xxxx xxxx.xxxx xxxx.1100   Bus Error Register (byte)
 *      xxxx.xxxx xxxx.xxxx xxxx.1110   (unused; reserved)
 */

#define	PGMMU_CTLSEL_SEGMAP0	(1 << 1)
#define	PGMMU_CTLSEL_SEGMAP	(2 << 1)
#define	PGMMU_CTLSEL_CONTEXT	(3 << 1)
#define	PGMMU_CTLSEL_PAGEMAP_U	(4 << 1)
#define	PGMMU_CTLSEL_PAGEMAP_L	(5 << 1)
#define	PGMMU_CTLSEL_PAGEMAP	PGMMU_CTLSEL_PAGEMAP_U
#define	PGMMU_CTLSEL_BUSERROR	(6 << 1)

#define	PGMMU_REG_SEGMAP0_ENTRY(va)	\
	(pgmmu_trunc_seg(va) | PGMMU_CTLSEL_SEGMAP0)

#define	PGMMU_REG_SEGMAP_ENTRY(va)	\
	(pgmmu_trunc_seg(va) | PGMMU_CTLSEL_SEGMAP)

#define	PGMMU_REG_CONTEXT		\
	PGMMU_CTLSEL_CONTEXT

#define	PGMMU_REG_PAGEMAP_ENTRY(e)	\
	(((e) << 4) | PGMMU_CTLSEL_PAGEMAP)

#define	PGMMU_REG_BUSERROR		\
	PGMMU_CTLSEL_BUSERROR

/*
 * Bus Error Register
 *
 * The Bus Error Register contains the cause of the most recent
 * bus error.  It is reset when read; writes are ignored.
 */
#define	PGMMU_BERR_INVALID	__BIT(0) /* invalid translation */
#define	PGMMU_BERR_PROT		__BIT(1) /* protection error (ro page) */
#define	PGMMU_BERR_PRIV		__BIT(2) /* privilege error (ko page) */
#define	PGMMU_BERR_TIMEOUT	__BIT(4) /* bus cycle watchdog timed out */
#define	PGMMU_BERR_VME		__BIT(5) /* VMEbus /BERR asserted */

	/* short-hand for "any MMU page fault type" */
#define	PGMMU_BERR_PAGEFAULT	(PGMMU_BERR_INVALID | \
				 PGMMU_BERR_PROT | \
				 PGMMU_BERR_PRIV)

#ifdef _KERNEL
#include <hb68k/pg68k/control.h>

#define	pgmmu_getsme0(va)	\
	control_inw(PGMMU_REG_SEGMAP0_ENTRY(va))
#define	pgmmu_setsme0(va, sme)	\
	control_outw(PGMMU_REG_SEGMAP0_ENTRY(va), (sme))

#define	pgmmu_getsme(va)	\
	control_inw(PGMMU_REG_SEGMAP_ENTRY(va))
#define	pgmmu_setsme(va, sme)	\
	control_outw(PGMMU_REG_SEGMAP_ENTRY(va), (uint16_t)(sme))

#define	pgmmu_getcontext()	\
	control_inb(PGMMU_REG_CONTEXT)
#define	pgmmu_setcontext(c)	\
	control_outb(PGMMU_REG_CONTEXT, (uint8_t)(c))

#define	pgmmu_getpme(e)		\
	control_inl(PGMMU_REG_PAGEMAP_ENTRY(e))
#define	pgmmu_setpme(e, pme)	\
	control_outl(PGMMU_REG_PAGEMAP_ENTRY(e), (pme))

#define	pgmmu_getberr()		\
	control_inb(PGMMU_REG_BUSERROR)

#endif /* _KERNEL */

#endif /* _PG68K_PGMMU_H_ */
