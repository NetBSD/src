/*	$NetBSD: pim.h,v 1.1.2.2 2002/06/23 17:36:23 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 * This describes the Processor Internal Memory data for various checks.
 *
 * References:
 *
 * "PA/RISC 1.1 I/O Firmware Architecture Reference Specification",
 * Version 1.0, August 22, 2001.
 */

/* The PIM data for HPMC and TOC contains these register arrays. */
struct hp700_pim_regs {

	/* The general registers. */
	u_int	pim_regs_r0;
	u_int	pim_regs_r1;
	u_int	pim_regs_r2;
	u_int	pim_regs_r3;
	u_int	pim_regs_r4;
	u_int	pim_regs_r5;
	u_int	pim_regs_r6;
	u_int	pim_regs_r7;
	u_int	pim_regs_r8;
	u_int	pim_regs_r9;
	u_int	pim_regs_r10;
	u_int	pim_regs_r11;
	u_int	pim_regs_r12;
	u_int	pim_regs_r13;
	u_int	pim_regs_r14;
	u_int	pim_regs_r15;
	u_int	pim_regs_r16;
	u_int	pim_regs_r17;
	u_int	pim_regs_r18;
	u_int	pim_regs_r19;
	u_int	pim_regs_r20;
	u_int	pim_regs_r21;
	u_int	pim_regs_r22;
	u_int	pim_regs_r23;
	u_int	pim_regs_r24;
	u_int	pim_regs_r25;
	u_int	pim_regs_r26;
	u_int	pim_regs_r27;
	u_int	pim_regs_r28;
	u_int	pim_regs_r29;
	u_int	pim_regs_r30;
	u_int	pim_regs_r31;

	/* The control registers. */
	u_int	pim_regs_cr0;
	u_int	pim_regs_cr1;
	u_int	pim_regs_cr2;
	u_int	pim_regs_cr3;
	u_int	pim_regs_cr4;
	u_int	pim_regs_cr5;
	u_int	pim_regs_cr6;
	u_int	pim_regs_cr7;
	u_int	pim_regs_cr8;
	u_int	pim_regs_cr9;
	u_int	pim_regs_cr10;
	u_int	pim_regs_cr11;
	u_int	pim_regs_cr12;
	u_int	pim_regs_cr13;
	u_int	pim_regs_cr14;
	u_int	pim_regs_cr15;
	u_int	pim_regs_cr16;
	u_int	pim_regs_cr17;
	u_int	pim_regs_cr18;
	u_int	pim_regs_cr19;
	u_int	pim_regs_cr20;
	u_int	pim_regs_cr21;
	u_int	pim_regs_cr22;
	u_int	pim_regs_cr23;
	u_int	pim_regs_cr24;
	u_int	pim_regs_cr25;
	u_int	pim_regs_cr26;
	u_int	pim_regs_cr27;
	u_int	pim_regs_cr28;
	u_int	pim_regs_cr29;
	u_int	pim_regs_cr30;
	u_int	pim_regs_cr31;

	/* The space registers. */
	u_int	pim_regs_sr0;
	u_int	pim_regs_sr1;
	u_int	pim_regs_sr2;
	u_int	pim_regs_sr3;
	u_int	pim_regs_sr4;
	u_int	pim_regs_sr5;
	u_int	pim_regs_sr6;
	u_int	pim_regs_sr7;

	/* The back entries of the instruction address queues. */
	u_int	pim_regs_iisq_tail;
	u_int	pim_regs_iioq_tail;
};

/* The PIM data for HPMC and LPMC contains this check information. */
struct hp700_pim_checks {

	/* The Check Type. */
	u_int	pim_check_type;
#define	PIM_CHECK_CACHE		(1 << 31)
#define	PIM_CHECK_TLB		(1 << 30)
#define	PIM_CHECK_BUS		(1 << 29)
#define	PIM_CHECK_ASSISTS	(1 << 28)
#define	PIM_CHECK_BITS		"\020\040CACHE\037TLB\036BUS\035ASSISTS"

	/*
	 * The CPU State.  In addition to the common PIM_CPU_
	 * bits defined below, some fields are HPMC-specific.
	 */
	u_int	pim_check_cpu_state;
#define	PIM_CPU_IQV	(1 << 31)
#define	PIM_CPU_IQF	(1 << 30)
#define	PIM_CPU_IPV	(1 << 29)
#define	PIM_CPU_GRV	(1 << 28)
#define	PIM_CPU_CRV	(1 << 27)
#define	PIM_CPU_SRV	(1 << 26)
#define	PIM_CPU_TRV	(1 << 25)
#define	PIM_CPU_BITS	"\020\040IQV\037IQF\036IPV\035GRV\034CRV\033SRV\032TRV"
#define	PIM_CPU_HPMC_TL(cs)	(((cs) >> 4) & 0x3)
#define	PIM_CPU_HPMC_HD		(1 << 3)
#define	PIM_CPU_HPMC_SIS	(1 << 2)
#define	PIM_CPU_HPMC_CS(cs)	((cs) & 0x3)
#define	PIM_CPU_HPMC_BITS	PIM_CPU_BITS "\004HD\003SIS"

	u_int	pim_check_reserved_0;

	/* The Cache Check word. */
	u_int	pim_check_cache;
#define	PIM_CACHE_ICC	(1 << 31)
#define	PIM_CACHE_DCC	(1 << 30)
#define	PIM_CACHE_TC	(1 << 29)
#define	PIM_CACHE_DC	(1 << 28)
#define	PIM_CACHE_CRG	(1 << 27)
#define	PIM_CACHE_LC	(1 << 26)
#define	PIM_CACHE_RCC	(1 << 25)
#define	PIM_CACHE_PADD(cc)	((cc) & 0x000fffff)
#define	PIM_CACHE_BITS	"\020\040ICC\037DCC\036TC\035DC\034CRG\033LC\032RCC"

	/* The TLB Check word. */
	u_int	pim_check_tlb;
#define	PIM_TLB_ITC	(1 << 31)
#define	PIM_TLB_DTC	(1 << 30)
#define	PIM_TLB_TRG	(1 << 29)
#define	PIM_TLB_TUC	(1 << 28)
#define	PIM_TLB_TNF	(1 << 27)
#define	PIM_TLB_BITS	"\020\040ITC\037DTC\036TRG\035TUC\034TNF"

	/* The Bus Check word. */
	u_int	pim_check_bus;
#define	PIM_BUS_RSV		(1 << 21)
#define	PIM_BUS_RQV		(1 << 20)
#define	PIM_BUS_VAR(bc)		(((bc) >> 16) & 0xf)
#define	PIM_BUS_TYPE(bc)	(((bc) >> 12) & 0xf)
#define	PIM_BUS_SIZE(bc)	(((bc) >> 8) & 0xf)
#define	PIM_BUS_PIV		(1 << 7)
#define	PIM_BUS_BSV		(1 << 6)
#define	PIM_BUS_STAT(bc)	((bc) & 0x3f)
#define	PIM_BUS_BITS		"\020\026RSV\025RQV\010PIV\007BSV"

	/* The Assist Check word. */
	u_int	pim_check_assist;
#define	PIM_ASSIST_COC		(1 << 31)
#define	PIM_ASSIST_SC		(1 << 30)
#define	PIM_ASSIST_BITS		"\020\040COC\037SC"

	u_int	pim_check_reserved_1;

	/* Additional information about the check. */
	u_int	pim_check_assist_state;
	u_int	pim_check_responder;
	u_int	pim_check_requestor;
	u_int	pim_check_path_info;
};

/* The PIM data for HPMC and LPMC contains this register array. */
struct hp700_pim_fpregs {

	/* The FPU state. */
	u_int64_t	pim_fpregs_fp0;
	u_int64_t	pim_fpregs_fp1;
	u_int64_t	pim_fpregs_fp2;
	u_int64_t	pim_fpregs_fp3;
	u_int64_t	pim_fpregs_fp4;
	u_int64_t	pim_fpregs_fp5;
	u_int64_t	pim_fpregs_fp6;
	u_int64_t	pim_fpregs_fp7;
	u_int64_t	pim_fpregs_fp8;
	u_int64_t	pim_fpregs_fp9;
	u_int64_t	pim_fpregs_fp10;
	u_int64_t	pim_fpregs_fp11;
	u_int64_t	pim_fpregs_fp12;
	u_int64_t	pim_fpregs_fp13;
	u_int64_t	pim_fpregs_fp14;
	u_int64_t	pim_fpregs_fp15;
	u_int64_t	pim_fpregs_fp16;
	u_int64_t	pim_fpregs_fp17;
	u_int64_t	pim_fpregs_fp18;
	u_int64_t	pim_fpregs_fp19;
	u_int64_t	pim_fpregs_fp20;
	u_int64_t	pim_fpregs_fp21;
	u_int64_t	pim_fpregs_fp22;
	u_int64_t	pim_fpregs_fp23;
	u_int64_t	pim_fpregs_fp24;
	u_int64_t	pim_fpregs_fp25;
	u_int64_t	pim_fpregs_fp26;
	u_int64_t	pim_fpregs_fp27;
	u_int64_t	pim_fpregs_fp28;
	u_int64_t	pim_fpregs_fp29;
	u_int64_t	pim_fpregs_fp30;
	u_int64_t	pim_fpregs_fp31;
};

/* The HPMC PIM data. */
struct hp700_pim_hpmc {
	struct	hp700_pim_regs pim_hpmc_regs;
	struct	hp700_pim_checks pim_hpmc_checks;
	struct	hp700_pim_fpregs pim_hpmc_fpregs;
};

/* The LPMC PIM data. */
struct hp700_pim_lpmc {
	u_int	pim_lpmc_hversion_dep[74];
	struct	hp700_pim_checks pim_lpmc_checks;
	struct	hp700_pim_fpregs pim_lpmc_fpregs;
};

/* The TOC PIM data. */
struct hp700_pim_toc {
	struct	hp700_pim_regs pim_toc_regs;
	u_int	pim_toc_hversion_dep;
	u_int	pim_toc_cpu_state;
};
