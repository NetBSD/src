/*	$NetBSD: r3900regs.h,v 1.2 2000/05/23 04:21:40 soren Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 *	[address space]
 *	kseg2		0xc0000000 - 0xfeffffff
 *	reserved	0xff000000 - 0xfffeffff
 *	kseg2		0xffff0000 - 0xffffffff
 * -> vmparam.h VM_MAX_KERNEL_ADDRESS
 */

/*
 *	[cause register]
 */
#define	R3900_CR_EXC_CODE	MIPS3_CR_EXC_CODE /* five bits */
#undef MIPS1_CR_EXC_CODE
#define MIPS1_CR_EXC_CODE	R3900_CR_EXC_CODE

/*
 *	[status register]
 *	R3900 don't have PE, CM, PZ, SwC and IsC.
 */
#define R3900_SR_NMI		0x00100000 /* r3k PE position */
#undef MIPS1_PARITY_ERR
#undef MIPS1_CACHE_MISS
#undef MIPS1_PARITY_ZERO
#undef MIPS1_SWAP_CACHES
#undef MIPS1_ISOL_CACHES

/*
 *	[context register]
 * - no changes.
 */


/*
 *	TX3900 Coprocessor 0 registers
 */
#define	R3900_COP_0_CONFIG	$3
#define	R3900_COP_0_DEBUG	$16
#define	R3900_COP_0_DEPC	$17

#define R3920_COP_0_PAGEMASK	$5
#define R3920_COP_0_WIRED	$6
#define	R3920_COP_0_CACHE	$7
#define R3920_COP_0_TAG_LO	$20

/*
 *	TLB entry
 *	3912 ... TLB entry is 64bits wide and R3000A compatible
 *	3922 ... TLB entry is 96bits wide
 */

/*
 *	Index register
 *	3912 ... index field[8:12] (32 entry)
 */
#define R3900_TLB_NUM_TLB_ENTRIES	32
#define R3920_TLB_NUM_TLB_ENTRIES	64
#undef MIPS1_TLB_NUM_TLB_ENTRIES
#ifdef TX391X
#define MIPS1_TLB_NUM_TLB_ENTRIES	R3900_TLB_NUM_TLB_ENTRIES
#elif defined TX392X
#define MIPS1_TLB_NUM_TLB_ENTRIES	R3920_TLB_NUM_TLB_ENTRIES
#endif

/*
 *	Config register (R3900 specific)
 */
#define R3900_CONFIG_ICS_SHIFT		19
#define R3900_CONFIG_ICS_MASK		0x00380000
#define R3900_CONFIG_ICS_1KB		0x00000000
#define R3900_CONFIG_ICS_2KB		0x00080000
#define R3900_CONFIG_ICS_4KB		0x00100000
#define R3900_CONFIG_ICS_8KB		0x00180000
#define R3900_CONFIG_ICS_16KB		0x00200000

#define R3900_CONFIG_DCS_SHIFT		16
#define R3900_CONFIG_DCS_1KB		0x00000000
#define R3900_CONFIG_DCS_2KB		0x00010000
#define R3900_CONFIG_DCS_4KB		0x00020000
#define R3900_CONFIG_DCS_8KB		0x00030000
#define R3900_CONFIG_DCS_16KB		0x00040000


#define R3900_CONFIG_DCS_MASK		0x00070000
#define R3900_CONFIG_CWFON		0x00004000
#define R3900_CONFIG_WBON		0x00002000
#define R3900_CONFIG_RF_SHIFT		10
#define R3900_CONFIG_RF_MASK		0x00000c00
#define R3900_CONFIG_DOZE		0x00000200
#define R3900_CONFIG_HALT		0x00000100
#define R3900_CONFIG_LOCK		0x00000080
#define R3900_CONFIG_ICE		0x00000020
#define R3900_CONFIG_DCE		0x00000010
#define R3900_CONFIG_IRSIZE_SHIFT	2
#define R3900_CONFIG_IRSIZE_MASK	0x0000000c
#define R3900_CONFIG_DRSIZE_SHIFT	0
#define R3900_CONFIG_DRSIZE_MASK	0x00000003

/*
 *	R3900 CACHE instruction (not MIPS3 cache op)
 */
#define R3900_MIN_CACHE_SIZE		1024
#define R3900_MAX_DCACHE_SIZE		(8 * 1024)
#ifndef OP_CACHE
#define OP_CACHE	057
#endif
#define R3900_CACHE(op, offset, base) \
	.word (OP_CACHE << 26 | ((base) << 21) | ((op) << 16) | \
	((offset) & 0xffff))
#define R3900_CACHE_I_INDEXINVALIDATE	0
#define R3900_CACHE_D_HITINVALIDATE	0x11

#define	CPUREG_A0	4	
#define CPUREG_T0	8
