/*	$NetBSD: r3900regs.h,v 1.4.10.2 2002/04/01 07:40:59 nathanw Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#if 0
#undef MIPS1_PARITY_ERR
#undef MIPS1_CACHE_MISS
#undef MIPS1_PARITY_ZERO
#undef MIPS1_SWAP_CACHES
#undef MIPS1_ISOL_CACHES
#endif

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
 *	CACHE
 */
/* Cache size (limit) */
/* R3900/R3920 */
#define R3900_C_SIZE_MIN		1024
#define R3900_C_SIZE_MAX		8192
/* Cache line size */
/* R3900 */
#define R3900_C_LSIZE_I			16
#define R3900_C_LSIZE_D			4
/* R3920 */
#define R3920_C_LSIZE_I			16
#define R3920_C_LSIZE_D			16
/* Cache operation */
/* R3900 */
#define R3900_C_IINV_I			0x00
#define R3900_C_IWBINV_D		0x01
#define R3900_C_ILRUC_I			0x04
#define R3900_C_ILRUC_D			0x05
#define R3900_C_ILCKC_D			0x09 /* R3900 only */
#define R3900_C_HINV_D			0x11
/* R3920 */
#define R3920_C_IINV_I			0x00
#define R3920_C_IWBINV_D		0x01
#define R3920_C_ILRUC_I			0x04
#define R3920_C_ILRUC_D			0x05
#define R3920_C_ILDTAG_I		0x0c /* R3920 only */
#define R3920_C_ILDTAG_D		0x0d /* R3920 only */
#define R3920_C_HINV_I			0x10 /* R3920 only */
#define R3920_C_HINV_D			0x11
#define R3920_C_HWBINV_D		0x14 /* R3920 only */
#define R3920_C_HWB_D			0x18 /* R3920 only */
#define R3920_C_ISTTAG_I		0x1c /* R3920 only */
#define R3920_C_ISTTAG_D		0x1d /* R3920 only */
