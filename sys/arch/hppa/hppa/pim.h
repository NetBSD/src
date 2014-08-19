/*	$NetBSD: pim.h,v 1.1.10.2 2014/08/20 00:03:04 tls Exp $	*/

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
 *
 * "PA/RISC 2.0 I/O Firmware Architecture Reference Specification",
 * Version 1.0, August 22, 2001.
 */

/* The PIM data for HPMC and TOC contains these register arrays. */
struct hppa_pim_regs {

	/* The general registers. */
	uint32_t	pim_regs_r0;
	uint32_t	pim_regs_r1;
	uint32_t	pim_regs_r2;
	uint32_t	pim_regs_r3;
	uint32_t	pim_regs_r4;
	uint32_t	pim_regs_r5;
	uint32_t	pim_regs_r6;
	uint32_t	pim_regs_r7;
	uint32_t	pim_regs_r8;
	uint32_t	pim_regs_r9;
	uint32_t	pim_regs_r10;
	uint32_t	pim_regs_r11;
	uint32_t	pim_regs_r12;
	uint32_t	pim_regs_r13;
	uint32_t	pim_regs_r14;
	uint32_t	pim_regs_r15;
	uint32_t	pim_regs_r16;
	uint32_t	pim_regs_r17;
	uint32_t	pim_regs_r18;
	uint32_t	pim_regs_r19;
	uint32_t	pim_regs_r20;
	uint32_t	pim_regs_r21;
	uint32_t	pim_regs_r22;
	uint32_t	pim_regs_r23;
	uint32_t	pim_regs_r24;
	uint32_t	pim_regs_r25;
	uint32_t	pim_regs_r26;
	uint32_t	pim_regs_r27;
	uint32_t	pim_regs_r28;
	uint32_t	pim_regs_r29;
	uint32_t	pim_regs_r30;
	uint32_t	pim_regs_r31;

	/* The control registers. */
	uint32_t	pim_regs_cr0;
	uint32_t	pim_regs_cr1;
	uint32_t	pim_regs_cr2;
	uint32_t	pim_regs_cr3;
	uint32_t	pim_regs_cr4;
	uint32_t	pim_regs_cr5;
	uint32_t	pim_regs_cr6;
	uint32_t	pim_regs_cr7;
	uint32_t	pim_regs_cr8;
	uint32_t	pim_regs_cr9;
	uint32_t	pim_regs_cr10;
	uint32_t	pim_regs_cr11;
	uint32_t	pim_regs_cr12;
	uint32_t	pim_regs_cr13;
	uint32_t	pim_regs_cr14;
	uint32_t	pim_regs_cr15;
	uint32_t	pim_regs_cr16;
	uint32_t	pim_regs_cr17;
	uint32_t	pim_regs_cr18;
	uint32_t	pim_regs_cr19;
	uint32_t	pim_regs_cr20;
	uint32_t	pim_regs_cr21;
	uint32_t	pim_regs_cr22;
	uint32_t	pim_regs_cr23;
	uint32_t	pim_regs_cr24;
	uint32_t	pim_regs_cr25;
	uint32_t	pim_regs_cr26;
	uint32_t	pim_regs_cr27;
	uint32_t	pim_regs_cr28;
	uint32_t	pim_regs_cr29;
	uint32_t	pim_regs_cr30;
	uint32_t	pim_regs_cr31;

	/* The space registers. */
	uint32_t	pim_regs_sr0;
	uint32_t	pim_regs_sr1;
	uint32_t	pim_regs_sr2;
	uint32_t	pim_regs_sr3;
	uint32_t	pim_regs_sr4;
	uint32_t	pim_regs_sr5;
	uint32_t	pim_regs_sr6;
	uint32_t	pim_regs_sr7;

	/* The back entries of the instruction address queues. */
	uint32_t	pim_regs_iisq_tail;
	uint32_t	pim_regs_iioq_tail;
};

/* The PIM data for HPMC and LPMC contains this check information. */
struct hppa_pim_checks {

	/* The Check Type. */
	uint32_t	pim_check_type;
#define	PIM_CHECK_CACHE		(1 << 31)
#define	PIM_CHECK_TLB		(1 << 30)
#define	PIM_CHECK_BUS		(1 << 29)
#define	PIM_CHECK_ASSISTS	(1 << 28)
#define	PIM_CHECK_BITS						\
	"\177\020"		/* New bitmask format */	\
	"b\040cache\0"		/* bit 31 */			\
	"b\037tlb\0"		/* bit 30 */			\
	"b\036bus\0"		/* bit 29 */			\
	"b\035assists\0"	/* bit 28 */

	/*
	 * The CPU State.  In addition to the common PIM_CPU_
	 * bits defined below, some fields are HPMC-specific.
	 */
	uint32_t	pim_check_cpu_state;
#define	PIM_CPU_IQV	(1 << 31)
#define	PIM_CPU_IQF	(1 << 30)
#define	PIM_CPU_IPV	(1 << 29)
#define	PIM_CPU_GRV	(1 << 28)
#define	PIM_CPU_CRV	(1 << 27)
#define	PIM_CPU_SRV	(1 << 26)
#define	PIM_CPU_TRV	(1 << 25)
#define	PIM_CPU_BITS						\
	"\177\020"		/* New bitmask format */	\
	"b\040iqv\0"		/* bit 31 */			\
	"b\037iqf\0"		/* bit 30 */			\
	"b\036ipv\0"		/* bit 29 */			\
	"b\035grv\0"		/* bit 28 */			\
	"b\034crv\0"		/* bit 27 */			\
	"b\033srv\0"		/* bit 26 */			\
	"b\032trv\0"		/* bit 25 */
#define	PIM_CPU_HPMC_TL(cs)	(((cs) >> 4) & 0x3)
#define	PIM_CPU_HPMC_HD		(1 << 3)
#define	PIM_CPU_HPMC_SIS	(1 << 2)
#define	PIM_CPU_HPMC_CS(cs)	((cs) & 0x3)
#define	PIM_CPU_HPMC_BITS					\
	PIM_CPU_BITS						\
	"f\004\002tl\0"         /* bit 4 .. 5 */		\
	"b\003hd\0"             /* bit 3 */			\
	"b\002sis\0"            /* bit 2 */			\
	"f\000\002cs\0"         /* bit 0 .. 1 */

	uint32_t	pim_check_reserved_0;

	/* The Cache Check word. */
	uint32_t	pim_check_cache;
#define	PIM_CACHE_ICC	(1 << 31)
#define	PIM_CACHE_DCC	(1 << 30)
#define	PIM_CACHE_TC	(1 << 29)
#define	PIM_CACHE_DC	(1 << 28)
#define	PIM_CACHE_CRG	(1 << 27)
#define	PIM_CACHE_LC	(1 << 26)
#define	PIM_CACHE_RCC	(1 << 25)
#define	PIM_CACHE_PADD(cc)	((cc) & 0x000fffff)
#define	PIM_CACHE_BITS						\
	"\177\020"		/* New bitmask format */	\
	"b\040icc\0"		/* bit 31 */			\
	"b\039dcc\0"		/* bit 30 */			\
	"b\038tc\0"		/* bit 29 */			\
	"b\037dc\0"		/* bit 28 */			\
	"b\036crg\0"		/* bit 27 */			\
	"b\035lc\0"		/* bit 26 */			\
	"b\034rcc\0"		/* bit 25 */			\
	"f\000\032paddr\0"	/* bit 0 .. 23 */

	/* The TLB Check word. */
	uint32_t	pim_check_tlb;
#define	PIM_TLB_ITC	(1 << 31)
#define	PIM_TLB_DTC	(1 << 30)
#define	PIM_TLB_TRG	(1 << 29)
#define	PIM_TLB_TUC	(1 << 28)
#define	PIM_TLB_TNF	(1 << 27)
#define	PIM_TLB_BITS						\
	"\177\020"		/* New bitmask format */	\
	"b\040itc\0"		/* bit 31 */			\
	"b\039dtc\0"		/* bit 30 */			\
	"b\038trg\0"		/* bit 29 */			\
	"b\037tuc\0"		/* bit 28 */			\
	"b\036tnf\0"		/* bit 27 */			\

	/* The Bus Check word. */
	uint32_t	pim_check_bus;
#define	PIM_BUS_RSV		(1 << 21)
#define	PIM_BUS_RQV		(1 << 20)
#define	PIM_BUS_VAR(bc)		(((bc) >> 16) & 0xf)
#define	PIM_BUS_TYPE(bc)	(((bc) >> 12) & 0xf)
#define	PIM_BUS_SIZE(bc)	(((bc) >> 8) & 0xf)
#define	PIM_BUS_PIV		(1 << 7)
#define	PIM_BUS_BSV		(1 << 6)
#define	PIM_BUS_STAT(bc)	((bc) & 0x3f)
#define	PIM_BUS_BITS \
	"\177\020"		/* New bitmask format */	\
	"b\025rsv\0" 		/* bit 21 */			\
	"b\024rqv\0"		/* bit 20 */			\
	"f\020\004var\0"	/* bit 16 .. 19 */		\
	"f\014\004type\0"	/* bit 12 .. 15 */		\
	"f\010\004size\0"	/* bit 8 .. 11 */		\
	"b\007piv\0"		/* bit 7 */			\
	"b\006bsv\0"		/* bit 6 */			\
	"f\000\006stat\0"	/* bit 0 .. 5 */

	/* The Assist Check word. */
	uint32_t	pim_check_assist;
#define	PIM_ASSIST_COC		(1 << 31)
#define	PIM_ASSIST_SC		(1 << 30)
#define	PIM_ASSIST_BITS		"\020\040COC\037SC"

	uint32_t	pim_check_reserved_1;

	/* Additional information about the check. */
	uint32_t	pim_check_assist_state;
	uint32_t	pim_check_responder;
	uint32_t	pim_check_requestor;
	uint32_t	pim_check_path_info;
};

/* The PIM data for HPMC and LPMC contains this register array. */
struct hppa_pim_fpregs {

	/* The FPU state. */
	uint64_t	pim_fpregs_fp0;
	uint64_t	pim_fpregs_fp1;
	uint64_t	pim_fpregs_fp2;
	uint64_t	pim_fpregs_fp3;
	uint64_t	pim_fpregs_fp4;
	uint64_t	pim_fpregs_fp5;
	uint64_t	pim_fpregs_fp6;
	uint64_t	pim_fpregs_fp7;
	uint64_t	pim_fpregs_fp8;
	uint64_t	pim_fpregs_fp9;
	uint64_t	pim_fpregs_fp10;
	uint64_t	pim_fpregs_fp11;
	uint64_t	pim_fpregs_fp12;
	uint64_t	pim_fpregs_fp13;
	uint64_t	pim_fpregs_fp14;
	uint64_t	pim_fpregs_fp15;
	uint64_t	pim_fpregs_fp16;
	uint64_t	pim_fpregs_fp17;
	uint64_t	pim_fpregs_fp18;
	uint64_t	pim_fpregs_fp19;
	uint64_t	pim_fpregs_fp20;
	uint64_t	pim_fpregs_fp21;
	uint64_t	pim_fpregs_fp22;
	uint64_t	pim_fpregs_fp23;
	uint64_t	pim_fpregs_fp24;
	uint64_t	pim_fpregs_fp25;
	uint64_t	pim_fpregs_fp26;
	uint64_t	pim_fpregs_fp27;
	uint64_t	pim_fpregs_fp28;
	uint64_t	pim_fpregs_fp29;
	uint64_t	pim_fpregs_fp30;
	uint64_t	pim_fpregs_fp31;
};

/* The HPMC PIM data. */
struct hppa_pim_hpmc {
	struct	hppa_pim_regs pim_hpmc_regs;
	struct	hppa_pim_checks pim_hpmc_checks;
	struct	hppa_pim_fpregs pim_hpmc_fpregs;
};

/* The LPMC PIM data. */
struct hppa_pim_lpmc {
	uint32_t	pim_lpmc_hversion_dep[74];
	struct	hppa_pim_checks pim_lpmc_checks;
	struct	hppa_pim_fpregs pim_lpmc_fpregs;
};

/* The TOC PIM data. */
struct hppa_pim_toc {
	struct	hppa_pim_regs pim_toc_regs;
	uint32_t	pim_toc_hversion_dep;
	uint32_t	pim_toc_cpu_state;
};

struct hppa_pim64_regs {

	/* The general registers. */
	uint64_t	pim_regs_r0;
	uint64_t	pim_regs_r1;
	uint64_t	pim_regs_r2;
	uint64_t	pim_regs_r3;
	uint64_t	pim_regs_r4;
	uint64_t	pim_regs_r5;
	uint64_t	pim_regs_r6;
	uint64_t	pim_regs_r7;
	uint64_t	pim_regs_r8;
	uint64_t	pim_regs_r9;
	uint64_t	pim_regs_r10;
	uint64_t	pim_regs_r11;
	uint64_t	pim_regs_r12;
	uint64_t	pim_regs_r13;
	uint64_t	pim_regs_r14;
	uint64_t	pim_regs_r15;
	uint64_t	pim_regs_r16;
	uint64_t	pim_regs_r17;
	uint64_t	pim_regs_r18;
	uint64_t	pim_regs_r19;
	uint64_t	pim_regs_r20;
	uint64_t	pim_regs_r21;
	uint64_t	pim_regs_r22;
	uint64_t	pim_regs_r23;
	uint64_t	pim_regs_r24;
	uint64_t	pim_regs_r25;
	uint64_t	pim_regs_r26;
	uint64_t	pim_regs_r27;
	uint64_t	pim_regs_r28;
	uint64_t	pim_regs_r29;
	uint64_t	pim_regs_r30;
	uint64_t	pim_regs_r31;

	/* The control registers. */
	uint64_t	pim_regs_cr0;
	uint64_t	pim_regs_cr1;
	uint64_t	pim_regs_cr2;
	uint64_t	pim_regs_cr3;
	uint64_t	pim_regs_cr4;
	uint64_t	pim_regs_cr5;
	uint64_t	pim_regs_cr6;
	uint64_t	pim_regs_cr7;
	uint64_t	pim_regs_cr8;
	uint64_t	pim_regs_cr9;
	uint64_t	pim_regs_cr10;
	uint64_t	pim_regs_cr11;
	uint64_t	pim_regs_cr12;
	uint64_t	pim_regs_cr13;
	uint64_t	pim_regs_cr14;
	uint64_t	pim_regs_cr15;
	uint64_t	pim_regs_cr16;
	uint64_t	pim_regs_cr17;
	uint64_t	pim_regs_cr18;
	uint64_t	pim_regs_cr19;
	uint64_t	pim_regs_cr20;
	uint64_t	pim_regs_cr21;
	uint64_t	pim_regs_cr22;
	uint64_t	pim_regs_cr23;
	uint64_t	pim_regs_cr24;
	uint64_t	pim_regs_cr25;
	uint64_t	pim_regs_cr26;
	uint64_t	pim_regs_cr27;
	uint64_t	pim_regs_cr28;
	uint64_t	pim_regs_cr29;
	uint64_t	pim_regs_cr30;
	uint64_t	pim_regs_cr31;

	/* The space registers. */
	uint64_t	pim_regs_sr0;
	uint64_t	pim_regs_sr1;
	uint64_t	pim_regs_sr2;
	uint64_t	pim_regs_sr3;
	uint64_t	pim_regs_sr4;
	uint64_t	pim_regs_sr5;
	uint64_t	pim_regs_sr6;
	uint64_t	pim_regs_sr7;

	/* The back entries of the instruction address queues. */
	uint64_t	pim_regs_iisq_tail;
	uint64_t	pim_regs_iioq_tail;
};

struct hppa_pim64_checks {
	/* The Check Type. */	
	uint32_t	pim_check_type;

	/*
	 * The CPU State.  In addition to the common PIM_CPU_
	 * bits defined below, some fields are HPMC-specific.
	 */
	uint32_t	pim_check_cpu_state;

	/* The Cache Check word. */
	uint32_t	pim_check_cache;

	/* The TLB Check word. */
	uint32_t	pim_check_tlb;

	/* The Bus Check word. */
	uint32_t	pim_check_bus;

	/* The Assist Check word. */
	uint32_t	pim_check_assist;

	/* Additional information about the check. */
	uint32_t	pim_check_assist_state;
	uint32_t	pim_check_path_info;
	uint64_t	pim_check_responder;
	uint64_t	pim_check_requestor;
};

/* The PARISC 2.0 HPMC PIM data. */
struct hppa_pim64_hpmc {
	struct hppa_pim64_regs pim_hpmc_regs;
	struct hppa_pim64_checks pim_hpmc_checks;
	struct hppa_pim_fpregs pim_hpmc_fpregs;
};

/* The PARISC 2.0 LPMC PIM data. */
struct hppa_pim64_lpmc {
	uint64_t pim_lmpc_hversion_dep[74];
	struct hppa_pim64_checks pim_lpmc_checks;
	struct hppa_pim_fpregs pim_lpmc_fpregs;
};

/* The PARISC 2.0 TOC PIM data. */
struct hppa_pim64_toc {
	struct	hppa_pim64_regs pim_toc_regs;
	uint32_t	pim_toc_hversion_dep;
	uint32_t	pim_toc_cpu_state;
};
