/*	$NetBSD: mips_machdep.c,v 1.231.4.1 2011/02/08 16:19:28 bouyer Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris Demetriou.
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
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mips_machdep.c,v 1.231.4.1 2011/02/08 16:19:28 bouyer Exp $");

#include "opt_cputype.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/reboot.h>
#include <sys/mount.h>			/* fsid_t for syscallargs */
#include <sys/lwp.h>
#include <sys/sysctl.h>
#include <sys/msgbuf.h>
#include <sys/conf.h>
#include <sys/core.h>
#include <sys/device.h>
#include <sys/kcore.h>
#include <sys/kmem.h>
#include <sys/ras.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/cpu.h>
#include <sys/ucontext.h>

#include <mips/kcore.h>
#include <mips/cpu.h>

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32.h>
#endif

#include <uvm/uvm.h>

#include <dev/cons.h>

#include <mips/pcb.h>
#include <mips/cache.h>
#include <mips/frame.h>
#include <mips/regnum.h>

#include <mips/cpu.h>
#include <mips/locore.h>
#include <mips/psl.h>
#include <mips/pte.h>
#include <mips/userret.h>

#ifdef __HAVE_BOOTINFO_H
#include <machine/bootinfo.h>
#endif

#if defined(MIPS32) || defined(MIPS64)
#include <mips/mipsNN.h>		/* MIPS32/MIPS64 registers */
#endif

/* Internal routines. */
int	cpu_dumpsize(void);
u_long	cpu_dump_mempagecnt(void);
int	cpu_dump(void);

#if defined(MIPS3_PLUS)
uint32_t mips_cp0_tlb_page_mask_probe(void);
uint64_t mips_cp0_tlb_entry_hi_probe(void);
uint64_t mips_cp0_tlb_entry_lo_probe(void);

static void mips3_tlb_probe(void);
#endif

#if defined(MIPS1)
static void	mips1_vector_init(void);
extern const struct locoresw mips1_locoresw;
#endif

#if defined(MIPS3)
#if defined(MIPS3_5900)
static void	r5900_vector_init(void);
extern const struct locoresw mips5900_locoresw;
#else
static void	mips3_vector_init(void);
extern const struct locoresw mips3_locoresw;
#endif
#endif

#if defined(MIPS32)
static void	mips32_vector_init(void);
extern const struct locoresw mips32_locoresw;
#endif

#if defined(MIPS64)
static void	mips64_vector_init(void);
extern const struct locoresw mips64_locoresw;
#endif

mips_locore_jumpvec_t mips_locore_jumpvec;

struct locoresw mips_locoresw;

int cpu_arch;
int cpu_mhz;
int mips_num_tlb_entries;
int mips_cpu_flags;
int mips_has_llsc;
int mips_has_r4k_mmu;
int mips3_pg_cached;
#ifdef _LP64
uint64_t mips3_xkphys_cached;
#endif
u_int mips3_pg_shift;
#ifdef MIPS3_PLUS
uint64_t mips3_tlb_vpn_mask;
uint64_t mips3_tlb_pfn_mask;
uint32_t mips3_tlb_pg_mask;
#endif

struct	segtab *segbase;

void *	msgbufaddr;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[128];


/*
 * Assumptions:
 *  - All MIPS3+ have an r4k-style MMU.  _Many_ assumptions throughout
 *    much of the mips code about this.  Includes overloaded usage of
 *    MIPS3_PLUS.
 *  - All MIPS3+ use the same exception model (cp0 status, cause bits,
 *    etc).  _Many_ assumptions throughout much of the mips code about
 *    this.  Includes overloaded usage of MIPS3_PLUS.
 *  - All MIPS3+ have a count register.  MIPS_HAS_CLOCK in <mips/cpu.h>
 *    will need to be revised if this is false.
 */
#define	MIPS32_FLAGS	CPU_MIPS_R4K_MMU | CPU_MIPS_CAUSE_IV | CPU_MIPS_USE_WAIT
#define	MIPS64_FLAGS	MIPS32_FLAGS	/* same as MIPS32 flags (for now) */

const struct pridtab *mycpu;

static const struct pridtab cputab[] = {
	{ 0, MIPS_R2000, -1, -1,		CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_LLSC, 0, 0,		"MIPS R2000 CPU"	},
	{ 0, MIPS_R3000, MIPS_REV_R2000A, -1,	CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_LLSC, 0, 0,		"MIPS R2000A CPU"	},
	{ 0, MIPS_R3000, MIPS_REV_R3000, -1,	CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_LLSC, 0, 0,		"MIPS R3000 CPU"	},
	{ 0, MIPS_R3000, MIPS_REV_R3000A, -1,	CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_LLSC, 0, 0,		"MIPS R3000A CPU"	},
	{ 0, MIPS_R6000, -1, -1,		CPU_ARCH_MIPS2, 32,
	  MIPS_NOT_SUPP, 0, 0,			"MIPS R6000 CPU"	},

	/*
	 * rev 0x00, 0x22 and 0x30 are R4000, 0x40, 0x50 and 0x60 are R4400.
	 * should we allow ranges and use 0x00 - 0x3f for R4000 and
	 * 0x40 - 0xff for R4400?
	 */
	{ 0, MIPS_R4000, MIPS_REV_R4000_A, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R4000 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4000_B, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R4000 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4000_C, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R4000 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4400_A, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R4400 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4400_B, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R4400 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4400_C, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R4400 CPU"	},

	{ 0, MIPS_R3LSI, -1, -1,		CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP, 0, 0,			"LSI Logic R3000 derivative" },
	{ 0, MIPS_R6000A, -1, -1,		CPU_ARCH_MIPS2, 32,
	  MIPS_NOT_SUPP, 0, 0,			"MIPS R6000A CPU"	},
	{ 0, MIPS_R3IDT, -1, -1,		CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP, 0, 0,			"IDT R3041 or RC36100 CPU" },
	{ 0, MIPS_R4100, -1, -1,		CPU_ARCH_MIPS3, 32,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_NO_LLSC, 0, 0,
						"NEC VR4100 CPU"	},
	{ 0, MIPS_R4200, -1, -1,		CPU_ARCH_MIPS3, -1,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU, 0, 0,
						"NEC VR4200 CPU"	},
	{ 0, MIPS_R4300, -1, -1,		CPU_ARCH_MIPS3, 32,
	  CPU_MIPS_R4K_MMU, 0, 0,		"NEC VR4300 CPU"	},
	{ 0, MIPS_R4600, -1, -1,		CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"QED R4600 Orion CPU"	},
	{ 0, MIPS_R4700, -1, -1,		CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU, 0, 0,		"QED R4700 Orion CPU"	},

	{ 0, MIPS_R8000, -1, -1,		CPU_ARCH_MIPS4, 384,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU, 0, 0,
					 "MIPS R8000 Blackbird/TFP CPU" },
	{ 0, MIPS_R10000, -1, -1,		CPU_ARCH_MIPS4, 64,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R10000 CPU"	},
	{ 0, MIPS_R12000, -1, -1,		CPU_ARCH_MIPS4, 64,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R12000 CPU"	},
	{ 0, MIPS_R14000, -1, -1,		CPU_ARCH_MIPS4, 64,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"MIPS R14000 CPU"	},

	/* XXX
	 * If the Processor Revision ID of the 4650 isn't 0, the following
	 * entry needs to be adjusted.  Can't use a wildcard match because
	 * the TX39 series processors share the same Processor ID value.
	 * Or maybe put TX39 CPUs first if the revid doesn't overlap with
	 * the 4650...
	 */
	{ 0, MIPS_R4650, 0, -1,			CPU_ARCH_MIPS3, -1,
	  MIPS_NOT_SUPP /* no MMU! */, 0, 0,	"QED R4650 CPU"	},
	{ 0, MIPS_TX3900, MIPS_REV_TX3912, -1,	CPU_ARCH_MIPS1, 32,
	  CPU_MIPS_NO_LLSC, 0, 0,		"Toshiba TX3912 CPU"	},
	{ 0, MIPS_TX3900, MIPS_REV_TX3922, -1,	CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_LLSC, 0, 0,		"Toshiba TX3922 CPU"	},
	{ 0, MIPS_TX3900, MIPS_REV_TX3927, -1,	CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_LLSC, 0, 0,		"Toshiba TX3927 CPU"	},
	{ 0, MIPS_R5000, -1, -1,		CPU_ARCH_MIPS4, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,			
						"MIPS R5000 CPU"	},
	{ 0, MIPS_RM5200, -1, -1,		CPU_ARCH_MIPS4, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_CAUSE_IV | CPU_MIPS_DOUBLE_COUNT |
	  CPU_MIPS_USE_WAIT, 0, 0,		"QED RM5200 CPU"	},

	/* XXX
	 * The rm7000 rev 2.0 can have 64 tlbs, and has 6 extra interrupts.  See
	 *    "Migrating to the RM7000 from other MIPS Microprocessors"
	 * for more details.
	 */
	{ 0, MIPS_RM7000, -1, -1,		CPU_ARCH_MIPS4, 48,
	  MIPS_NOT_SUPP | CPU_MIPS_CAUSE_IV | CPU_MIPS_DOUBLE_COUNT |
	  CPU_MIPS_USE_WAIT, 0, 0,		"QED RM7000 CPU"	},

	/* 
	 * IDT RC32300 core is a 32 bit MIPS2 processor with
	 * MIPS3/MIPS4 extensions. It has an R4000-style TLB,
	 * while all registers are 32 bits and any 64 bit
	 * instructions like ld/sd/dmfc0/dmtc0 are not allowed.
	 *
	 * note that the Config register has a non-standard base
	 * for IC and DC (2^9 instead of 2^12).
	 *
	 */
	{ 0, MIPS_RC32300, -1, -1,		CPU_ARCH_MIPS3, 16,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU, 0, 0,
						"IDT RC32300 CPU"	},
	{ 0, MIPS_RC32364, -1, -1,		CPU_ARCH_MIPS3, 16,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU, 0, 0,
						"IDT RC32364 CPU"	},
	{ 0, MIPS_RC64470, -1, -1,		CPU_ARCH_MIPSx, -1,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU, 0, 0,
						"IDT RC64474/RC64475 CPU" },

	{ 0, MIPS_R5400, -1, -1,		CPU_ARCH_MIPSx, -1,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU, 0, 0,
						"NEC VR5400 CPU"	},
	{ 0, MIPS_R5900, -1, -1,		CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_NO_LLSC | CPU_MIPS_R4K_MMU, 0, 0,
						"Toshiba R5900 CPU"	},

	{ 0, MIPS_TX4900, MIPS_REV_TX4927, -1,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"Toshiba TX4927 CPU"	},

	{ 0, MIPS_TX4900, -1, -1,		CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT, 0, 0,
						"Toshiba TX4900 CPU"	},

	/* 
	 * ICT Loongson2 is a MIPS64 CPU with a few quirks.  For some reason
	 * the virtual aliases present with 4KB pages make the caches misbehave
	 * so we make all accesses uncached.  With 16KB pages, no virtual
	 * aliases are possible so we can use caching.
	 */
#ifdef ENABLE_MIPS_16KB_PAGE
#define	MIPS_LOONGSON2_CCA	0
#else
#define	MIPS_LOONGSON2_CCA	(CPU_MIPS_HAVE_SPECIAL_CCA | \
				(2 << CPU_MIPS_CACHED_CCA_SHIFT))
#endif
	{ 0, MIPS_LOONGSON2, MIPS_REV_LOONGSON2E, -1, CPU_ARCH_MIPS3, 64,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT | MIPS_LOONGSON2_CCA, 0, 0,
						"ICT Loongson 2E CPU"	},
	{ 0, MIPS_LOONGSON2, MIPS_REV_LOONGSON2F, -1, CPU_ARCH_MIPS3, 64,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT | MIPS_LOONGSON2_CCA, 0, 0,
						"ICT Loongson 2F CPU"	},

#if 0 /* ID collisions : can we use a CU1 test or similar? */
	{ 0, MIPS_R3SONY, -1, -1,		CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP, 0, 0,			"SONY R3000 derivative"	},	/* 0x21; crash R4700? */
	{ 0, MIPS_R3NKK, -1, -1,		CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP, 0, 0,			"NKK R3000 derivative"	},	/* 0x23; crash R5000? */
#endif

	{ MIPS_PRID_CID_MTI, MIPS_4Kc, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "4Kc"		},
	{ MIPS_PRID_CID_MTI, MIPS_4KEc, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "4KEc"		},
	{ MIPS_PRID_CID_MTI, MIPS_4KEc_R2, -1, -1, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "4KEc (Rev 2)"	},
	{ MIPS_PRID_CID_MTI, MIPS_4KSc, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "4KSc"		},
	{ MIPS_PRID_CID_MTI, MIPS_5Kc, -1, -1,	-1, 0,
	  MIPS64_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "5Kc"		},
	{ MIPS_PRID_CID_MTI, MIPS_20Kc, -1, -1,	-1, 0,
	  MIPS64_FLAGS,				0, 0, "20Kc"		},
	{ MIPS_PRID_CID_MTI, MIPS_24K, -1, -1,	-1, 0,
	  MIPS64_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "24K"		},
	{ MIPS_PRID_CID_MTI, MIPS_24KE, -1, -1,	-1, 0,
	  MIPS64_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "24KE"		},
	{ MIPS_PRID_CID_MTI, MIPS_34K, -1, -1,	-1, 0,
	  MIPS64_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "34K"		},
	{ MIPS_PRID_CID_MTI, MIPS_74K, -1, -1,	-1, 0,
	  MIPS64_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "74K"		},

	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV1, -1, MIPS_AU1000, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1000 (Rev 1 core)"	},
	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV2, -1, MIPS_AU1000, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1000 (Rev 2 core)" 	},

	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV1, -1, MIPS_AU1100, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1100 (Rev 1 core)"	},
	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV2, -1, MIPS_AU1100, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1100 (Rev 2 core)" 	},

	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV1, -1, MIPS_AU1500, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1500 (Rev 1 core)"	},
	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV2, -1, MIPS_AU1500, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1500 (Rev 2 core)" 	},

	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU_REV2, -1, MIPS_AU1550, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_NO_WAIT | CPU_MIPS_I_D_CACHE_COHERENT, 0, 0,
						"Au1550 (Rev 2 core)" 	},

	/* The SB-1 CPU uses a CCA of 5 - "Cacheable Coherent Shareable" */
	{ MIPS_PRID_CID_SIBYTE, MIPS_SB1, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT |
	  CPU_MIPS_HAVE_SPECIAL_CCA | (5 << CPU_MIPS_CACHED_CCA_SHIFT), 0, 0,
						"SB-1"			},

	{ MIPS_PRID_CID_RMI, MIPS_XLS616, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIGn(0) | MIPS_CP0FL_CONFIGn(1) | MIPS_CP0FL_CONFIGn(7),
	  CIDFL_RMI_TYPE_XLS,			"XLS616"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS416, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIGn(0) | MIPS_CP0FL_CONFIGn(1) | MIPS_CP0FL_CONFIGn(7),
	  CIDFL_RMI_TYPE_XLS,			"XLS416"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS408, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIGn(0) | MIPS_CP0FL_CONFIGn(1) | MIPS_CP0FL_CONFIGn(7),
	  CIDFL_RMI_TYPE_XLS,			"XLS408"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS408LITE, -1, -1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIGn(0) | MIPS_CP0FL_CONFIGn(1) | MIPS_CP0FL_CONFIGn(7),
	  CIDFL_RMI_TYPE_XLS,			"XLS408LITE"		},

	/* Microsoft Research' extensible MIPS */
	{ MIPS_PRID_CID_MICROSOFT, MIPS_eMIPS, 1, -1, CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_WAIT, 0, 0,		"eMIPS CPU"		},

	{ 0, 0, 0,				0, 0, 0,
	  0, 0, 0,				NULL			}
};

static const struct pridtab fputab[] = {
    { 0, MIPS_SOFT,  -1, 0, 0, 0, 0, 0, 0, "software emulated floating point" },
    { 0, MIPS_R2360, -1, 0, 0, 0, 0, 0, 0, "MIPS R2360 Floating Point Board" },
    { 0, MIPS_R2010, -1, 0, 0, 0, 0, 0, 0, "MIPS R2010 FPC" },
    { 0, MIPS_R3010, -1, 0, 0, 0, 0, 0, 0, "MIPS R3010 FPC" },
    { 0, MIPS_R6010, -1, 0, 0, 0, 0, 0, 0, "MIPS R6010 FPC" },
    { 0, MIPS_R4010, -1, 0, 0, 0, 0, 0, 0, "MIPS R4010 FPC" },
};

/*
 * Company ID's are not sparse (yet), this array is indexed directly
 * by pridtab->cpu_cid.
 */
static const char * const cidnames[] = {
	"Prehistoric",
	"MIPS",		/* or "MIPS Technologies, Inc.	*/
	"Broadcom",	/* or "Broadcom Corp."		*/
	"Alchemy",	/* or "Alchemy Semiconductor"	*/
	"SiByte",	/* or "Broadcom Corp. (SiByte)"	*/
	"SandCraft",
	"Phillips",
	"Toshiba or Microsoft",
	"LSI",
	"(unannounced)",
	"(unannounced)",
	"Lexra",
	"RMI",
};
#define	ncidnames (sizeof(cidnames) / sizeof(cidnames[0]))

#if defined(MIPS1)
/*
 * MIPS-I locore function vector
 */
static const mips_locore_jumpvec_t mips1_locore_vec = {
	mips1_SetPID,
	mips1_TBIAP,
	mips1_TBIS,
	mips1_TLBUpdate,
	mips1_wbflush,
};

static void
mips1_vector_init(void)
{
	extern char mips1_UTLBMiss[], mips1_UTLBMissEnd[];
	extern char mips1_exception[], mips1_exceptionEnd[];

	/*
	 * Copy down exception vector code.
	 */
	if (mips1_UTLBMissEnd - mips1_UTLBMiss > 0x80)
		panic("startup: UTLB vector code too large");
	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips1_UTLBMiss,
		mips1_UTLBMissEnd - mips1_UTLBMiss);

	if (mips1_exceptionEnd - mips1_exception > 0x80)
		panic("startup: general exception vector code too large");
	memcpy((void *)MIPS1_GEN_EXC_VEC, mips1_exception,
		mips1_exceptionEnd - mips1_exception);

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips1_locore_vec;

	/*
	 * Clear out the I and D caches.
	 */
	mips_icache_sync_all();
	mips_dcache_wbinv_all();
}
#endif /* MIPS1 */

#if defined(MIPS3)
#ifndef MIPS3_5900	/* XXX */
/*
 * MIPS III locore function vector
 */
static const mips_locore_jumpvec_t mips3_locore_vec = {
	mips3_SetPID,
	mips3_TBIAP,
	mips3_TBIS,
	mips3_TLBUpdate,
	mips3_wbflush,
};

static void
mips3_vector_init(void)
{
	/* r4000 exception handler address and end */
	extern char mips3_exception[], mips3_exceptionEnd[];

	/* TLB miss handler address and end */
	extern char mips3_TLBMiss[], mips3_TLBMissEnd[];
	extern char mips3_XTLBMiss[], mips3_XTLBMissEnd[];

	/* Cache error handler */
	extern char mips3_cache[], mips3_cacheEnd[];

	/*
	 * Copy down exception vector code.
	 */

	if (mips3_TLBMissEnd - mips3_TLBMiss > 0x80)
		panic("startup: UTLB vector code too large");
	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips3_TLBMiss,
	      mips3_TLBMissEnd - mips3_TLBMiss);

	if (mips3_XTLBMissEnd - mips3_XTLBMiss > 0x80)
		panic("startup: XTLB vector code too large");
	memcpy((void *)MIPS3_XTLB_MISS_EXC_VEC, mips3_XTLBMiss,
	      mips3_XTLBMissEnd - mips3_XTLBMiss);

	if (mips3_cacheEnd - mips3_cache > 0x80)
		panic("startup: Cache error vector code too large");
	memcpy((void *)MIPS3_CACHE_ERR_EXC_VEC, mips3_cache,
	      mips3_cacheEnd - mips3_cache);

	if (mips3_exceptionEnd - mips3_exception > 0x80)
		panic("startup: General exception vector code too large");
	memcpy((void *)MIPS3_GEN_EXC_VEC, mips3_exception,
	      mips3_exceptionEnd - mips3_exception);

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips3_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);
}
#endif /* !MIPS3_5900 */

#if defined(MIPS3_5900)	/* XXX */
/*
 * MIPS R5900 locore function vector.
 * Same as MIPS32 - all MMU registers are 32bit.
 */
static const mips_locore_jumpvec_t r5900_locore_vec = {
	mips5900_SetPID,
	mips5900_TBIAP,
	mips5900_TBIS,
	mips5900_TLBUpdate,
	mips5900_wbflush,
};

static void
r5900_vector_init(void)
{
	extern char mips5900_exception[], mips5900_exceptionEnd[];
	extern char mips5900_TLBMiss[], mips5900_TLBMissEnd[];
	size_t esz = mips5900_exceptionEnd - mips5900_exception;
	size_t tsz = mips5900_TLBMissEnd - mips5900_TLBMiss;

	KDASSERT(tsz <= 0x80);
	KDASSERT(esz <= 0x80);

	if (tsz > 0x80)
		panic("startup: UTLB vector code too large");
	if (esz > 0x80)
		panic("startup: General exception vector code too large");

	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips5900_TLBMiss, tsz);
	memcpy((void *)MIPS_R5900_COUNTER_EXC_VEC, mips5900_exception, esz);
	memcpy((void *)MIPS_R5900_DEBUG_EXC_VEC, mips5900_exception, esz);
	memcpy((void *)MIPS3_GEN_EXC_VEC, mips5900_exception, esz);
	memcpy((void *)MIPS3_INTR_EXC_VEC, mips5900_exception, esz);

	mips_locore_jumpvec = r5900_locore_vec;

	mips_config_cache();

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);
}
#endif /* MIPS3_5900 */
#endif /* MIPS3 */

#if defined(MIPS32)
/*
 * MIPS32 locore function vector
 */
static const mips_locore_jumpvec_t mips32_locore_vec = {
	mips32_SetPID,
	mips32_TBIAP,
	mips32_TBIS,
	mips32_TLBUpdate,
	mips32_wbflush,
};

static void
mips32_vector_init(void)
{
	/* r4000 exception handler address and end */
	extern char mips32_exception[], mips32_exceptionEnd[];

	/* TLB miss handler address and end */
	extern char mips32_TLBMiss[], mips32_TLBMissEnd[];

	/* Cache error handler */
	extern char mips32_cache[], mips32_cacheEnd[];

	/* MIPS32/MIPS64 interrupt exception handler */
	extern char mips32_intr[], mips32_intrEnd[];

	/*
	 * Copy down exception vector code.
	 */

	if (mips32_TLBMissEnd - mips32_TLBMiss > 0x80)
		panic("startup: UTLB vector code too large");
	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips32_TLBMiss,
		      mips32_TLBMissEnd - mips32_TLBMiss);

	if (mips32_cacheEnd - mips32_cache > 0x80)
		panic("startup: Cache error vector code too large");
	memcpy((void *)MIPS3_CACHE_ERR_EXC_VEC, mips32_cache,
	      mips32_cacheEnd - mips32_cache);

	if (mips32_exceptionEnd - mips32_exception > 0x80)
		panic("startup: General exception vector code too large");
	memcpy((void *)MIPS3_GEN_EXC_VEC, mips32_exception,
	      mips32_exceptionEnd - mips32_exception);

	if (mips32_intrEnd - mips32_intr > 0x80)
		panic("startup: interrupt exception vector code too large");
#if 0	/* XXX - why doesn't mipsNN_intr() work? */
	memcpy((void *)MIPS3_INTR_EXC_VEC, mips32_intr,
	      mips32_intrEnd - mips32_intr);
#else
	memcpy((void *)MIPS3_INTR_EXC_VEC, mips32_exception,
	      mips32_exceptionEnd - mips32_exception);
#endif

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips32_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);
}
#endif /* MIPS32 */

#if defined(MIPS64)
/*
 * MIPS64 locore function vector
 */
const mips_locore_jumpvec_t mips64_locore_vec = {
	mips64_SetPID,
	mips64_TBIAP,
	mips64_TBIS,
	mips64_TLBUpdate,
	mips64_wbflush,
};

static void
mips64_vector_init(void)
{
	/* r4000 exception handler address and end */
	extern char mips64_exception[], mips64_exceptionEnd[];

	/* TLB miss handler address and end */
	extern char mips64_TLBMiss[], mips64_TLBMissEnd[];
	extern char mips64_XTLBMiss[], mips64_XTLBMissEnd[];

	/* Cache error handler */
	extern char mips64_cache[], mips64_cacheEnd[];

	/* MIPS32/MIPS64 interrupt exception handler */
	extern char mips64_intr[], mips64_intrEnd[];

	/*
	 * Copy down exception vector code.
	 */

	if (mips64_TLBMissEnd - mips64_TLBMiss > 0x80)
		panic("startup: UTLB vector code too large");
	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips64_TLBMiss,
	      mips64_TLBMissEnd - mips64_TLBMiss);

	if (mips64_XTLBMissEnd - mips64_XTLBMiss > 0x80)
		panic("startup: XTLB vector code too large");
	memcpy((void *)MIPS3_XTLB_MISS_EXC_VEC, mips64_XTLBMiss,
	      mips64_XTLBMissEnd - mips64_XTLBMiss);

	if (mips64_cacheEnd - mips64_cache > 0x80)
		panic("startup: Cache error vector code too large");
	memcpy((void *)MIPS3_CACHE_ERR_EXC_VEC, mips64_cache,
	      mips64_cacheEnd - mips64_cache);

	if (mips64_exceptionEnd - mips64_exception > 0x80)
		panic("startup: General exception vector code too large");
	memcpy((void *)MIPS3_GEN_EXC_VEC, mips64_exception,
	      mips64_exceptionEnd - mips64_exception);

	if (mips64_intrEnd - mips64_intr > 0x80)
		panic("startup: interrupt exception vector code too large");
#if 0	/* XXX - why doesn't mipsNN_intr() work? */
	memcpy((void *)MIPS3_INTR_EXC_VEC, mips64_intr,
	      mips64_intrEnd - mips64_intr);
#else
	memcpy((void *)MIPS3_INTR_EXC_VEC, mips64_exception,
	      mips64_exceptionEnd - mips64_exception);
#endif

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips64_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);
}
#endif /* MIPS64 */

/*
 * Do all the stuff that locore normally does before calling main(),
 * that is common to all mips-CPU NetBSD ports.
 *
 * The principal purpose of this function is to examine the
 * variable cpu_id, into which the kernel locore start code
 * writes the CPU ID register, and to then copy appropriate
 * code into the CPU exception-vector entries and the jump tables
 * used to hide the differences in cache and TLB handling in
 * different MIPS CPUs.
 *
 * This should be the very first thing called by each port's
 * init_main() function.
 */

/*
 * Initialize the hardware exception vectors, and the jump table used to
 * call locore cache and TLB management functions, based on the kind
 * of CPU the kernel is running on.
 */
void
mips_vector_init(void)
{
	const struct pridtab *ct;

	/*
	 * XXX Set-up curlwp/curcpu again.  They may have been clobbered
	 * beween verylocore and here.
	 */
	lwp0.l_cpu = &cpu_info_store;
	cpu_info_store.ci_curlwp = &lwp0;
	cpu_info_store.ci_fpcurlwp = &lwp0;
	curlwp = &lwp0;

	mycpu = NULL;
	for (ct = cputab; ct->cpu_displayname != NULL; ct++) {
		if (MIPS_PRID_CID(cpu_id) != ct->cpu_cid ||
		    MIPS_PRID_IMPL(cpu_id) != ct->cpu_pid)
			continue;
		if (ct->cpu_rev >= 0 &&
		    MIPS_PRID_REV(cpu_id) != ct->cpu_rev)
			continue;
		if (ct->cpu_copts >= 0 &&
		    MIPS_PRID_COPTS(cpu_id) != ct->cpu_copts)
			continue;

		mycpu = ct;
		cpu_arch = ct->cpu_isa;
		mips_num_tlb_entries = ct->cpu_ntlb;
		break;
	}

	if (mycpu == NULL)
		panic("CPU type (0x%x) not supported", cpu_id);

#if defined(MIPS32) || defined(MIPS64)
	if (MIPS_PRID_CID(cpu_id) != 0) {
		/* MIPS32/MIPS64, use coprocessor 0 config registers */
		uint32_t cfg, cfg1;

		cfg = mips3_cp0_config_read();
		cfg1 = mipsNN_cp0_config1_read();

		/* pick CPU type */
		switch (MIPSNN_GET(CFG_AT, cfg)) {
		case MIPSNN_CFG_AT_MIPS32:
			cpu_arch = CPU_ARCH_MIPS32;
			break;
		case MIPSNN_CFG_AT_MIPS64:
			cpu_arch = CPU_ARCH_MIPS64;
			break;
		case MIPSNN_CFG_AT_MIPS64S:
		default:
			panic("MIPS32/64 architecture type %d not supported",
			    MIPSNN_GET(CFG_AT, cfg));
		}

		switch (MIPSNN_GET(CFG_AR, cfg)) {
		case MIPSNN_CFG_AR_REV1:
		case MIPSNN_CFG_AR_REV2:
			break;
		default:
			printf("WARNING: MIPS32/64 arch revision %d "
			    "unknown!\n", MIPSNN_GET(CFG_AR, cfg));
			break;
		}

		/* figure out MMU type (and number of TLB entries) */
		switch (MIPSNN_GET(CFG_MT, cfg)) {
		case MIPSNN_CFG_MT_TLB:
			mips_num_tlb_entries = MIPSNN_CFG1_MS(cfg1);
			break;
		case MIPSNN_CFG_MT_NONE:
		case MIPSNN_CFG_MT_BAT:
		case MIPSNN_CFG_MT_FIXED:
		default:
			panic("MIPS32/64 MMU type %d not supported",
			    MIPSNN_GET(CFG_MT, cfg));
		}
	}
#endif /* defined(MIPS32) || defined(MIPS64) */

	if (cpu_arch < 1)
		panic("Unknown CPU ISA for CPU type 0x%x", cpu_id);
	if (mips_num_tlb_entries < 1)
		panic("Unknown number of TLBs for CPU type 0x%x", cpu_id);

	/*
	 * Check CPU-specific flags.
	 */
	mips_cpu_flags = mycpu->cpu_flags;
	mips_has_r4k_mmu = mips_cpu_flags & CPU_MIPS_R4K_MMU;
	mips_has_llsc = (mips_cpu_flags & CPU_MIPS_NO_LLSC) == 0;
#if defined(MIPS3_4100)
	if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4100)
		mips3_pg_shift = MIPS3_4100_PG_SHIFT;
	else
#endif
		mips3_pg_shift = MIPS3_DEFAULT_PG_SHIFT;

	if (mycpu->cpu_flags & CPU_MIPS_HAVE_SPECIAL_CCA) {
		uint32_t cca;

		cca = (ct->cpu_flags & CPU_MIPS_CACHED_CCA_MASK) >>
		    CPU_MIPS_CACHED_CCA_SHIFT;
		mips3_pg_cached = MIPS3_CCA_TO_PG(cca);
#ifdef _LP64
		mips3_xkphys_cached = MIPS_PHYS_TO_XKPHYS(cca, 0);
#endif
	} else {
		mips3_pg_cached = MIPS3_DEFAULT_PG_CACHED;
#ifdef _LP64
		mips3_xkphys_cached = MIPS3_DEFAULT_XKPHYS_CACHED;
#endif
	}

#ifdef __HAVE_MIPS_MACHDEP_CACHE_CONFIG
	mips_machdep_cache_config();
#endif

	/*
	 * Determine cache configuration and initialize our cache
	 * frobbing routine function pointers.
	 */
	mips_config_cache();

	/*
	 * Now initialize our ISA-dependent function vector.
	 */
	switch (cpu_arch) {
#if defined(MIPS1)
	case CPU_ARCH_MIPS1:
		mips1_TBIA(mips_num_tlb_entries);
		mips1_vector_init();
		mips_locoresw = mips1_locoresw;
		break;
#endif
#if defined(MIPS3)
	case CPU_ARCH_MIPS3:
	case CPU_ARCH_MIPS4:
		mips3_tlb_probe();
#if defined(MIPS3_5900)	/* XXX */
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		mips5900_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		r5900_vector_init();
		mips_locoresw = mips5900_locoresw;
#else /* MIPS3_5900 */
#if defined(MIPS3_4100)
		if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4100)
			mips3_cp0_pg_mask_write(MIPS4100_PG_SIZE_TO_MASK(PAGE_SIZE));
		else
#endif
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		mips3_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		mips3_vector_init();
		mips_locoresw = mips3_locoresw;
#endif /* MIPS3_5900 */
		break;
#endif
#if defined(MIPS32)
	case CPU_ARCH_MIPS32:
		mips3_tlb_probe();
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		mips32_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		mips32_vector_init();
		mips_locoresw = mips32_locoresw;
		break;
#endif
#if defined(MIPS64)
	case CPU_ARCH_MIPS64: {
		mips3_tlb_probe();
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		mips64_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		mips64_vector_init();
		mips_locoresw = mips64_locoresw;
		break;
	}
#endif
	default:
		printf("cpu_arch 0x%x: not supported\n", cpu_arch);
		cpu_reboot(RB_HALT, NULL);
	}

/* XXX simonb: ugg, another ugly #ifdef check... */
#if (defined(MIPS3) && !defined(MIPS3_5900)) || defined(MIPS32) || defined(MIPS64)
	/*
	 * Install power-saving idle routines.
	 */
	if ((mips_cpu_flags & CPU_MIPS_USE_WAIT) &&
	    !(mips_cpu_flags & CPU_MIPS_NO_WAIT))
		mips_locoresw.lsw_cpu_idle = mips_wait_idle;
#endif /* (MIPS3 && !MIPS3_5900) || MIPS32 || MIPS64 */
}

void
mips_set_wbflush(void (*flush_fn)(void))
{
#undef wbflush
	mips_locore_jumpvec.wbflush = flush_fn;
	(*flush_fn)();
}

#if defined(MIPS3_PLUS)
static void
mips3_tlb_probe(void)
{
	mips3_tlb_pg_mask = mips_cp0_tlb_page_mask_probe();
	if (CPUIS64BITS) {
		mips3_tlb_vpn_mask = mips_cp0_tlb_entry_hi_probe();
		mips3_tlb_vpn_mask <<= 2;
		mips3_tlb_vpn_mask >>= 2;
		mips3_tlb_pfn_mask = mips_cp0_tlb_entry_lo_probe();
	}
}
#endif

/*
 * Identify product revision IDs of CPU and FPU.
 */
void
cpu_identify(void)
{
	static const char * const waynames[] = {
		"fully set-associative",	/* 0 */
		"direct-mapped",		/* 1 */
		"2-way set-associative",	/* 2 */
		NULL,				/* 3 */
		"4-way set-associative",	/* 4 */
		"5-way set-associative",	/* 5 */
		"6-way set-associative",	/* 6 */
		"7-way set-associative",	/* 7 */
		"8-way set-associative",	/* 8 */
	};
#define	nwaynames (sizeof(waynames) / sizeof(waynames[0]))
	static const char * const wtnames[] = {
		"write-back",
		"write-through",
	};
	static const char * const label = "cpu0";	/* XXX */
	const char *cpuname, *fpuname;
	int i;

	cpuname = mycpu->cpu_displayname;

	fpuname = NULL;
	for (i = 0; i < sizeof(fputab)/sizeof(fputab[0]); i++) {
		if (MIPS_PRID_CID(fpu_id) == fputab[i].cpu_cid &&
		    MIPS_PRID_IMPL(fpu_id) == fputab[i].cpu_pid) {
			fpuname = fputab[i].cpu_displayname;
			break;
		}
	}
	if (fpuname == NULL && MIPS_PRID_IMPL(fpu_id) == MIPS_PRID_IMPL(cpu_id))
		fpuname = "built-in FPU";
	if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4700)	/* FPU PRid is 0x20 */
		fpuname = "built-in FPU";
	if (MIPS_PRID_IMPL(cpu_id) == MIPS_RC64470)	/* FPU PRid is 0x21 */
		fpuname = "built-in FPU";

	if (mycpu->cpu_cid != 0) {
		if (mycpu->cpu_cid <= ncidnames)
			printf("%s ", cidnames[mycpu->cpu_cid]);
		else {
			printf("Unknown Company ID - 0x%x", mycpu->cpu_cid);
			printf("%s: ", label);
		}
	}
	if (cpuname != NULL)
		printf("%s (0x%x)", cpuname, cpu_id);
	else
		printf("unknown CPU type (0x%x)", cpu_id);
	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC)
		printf(" Rev. %d.%d", MIPS_PRID_REV_MAJ(cpu_id),
		    MIPS_PRID_REV_MIN(cpu_id));
	else
		printf(" Rev. %d", MIPS_PRID_REV(cpu_id));

	if (fpuname != NULL)
		printf(" with %s", fpuname);
	else
		printf(" with unknown FPC type (0x%x)", fpu_id);
	if (fpu_id != 0) {
		if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC)
			printf(" Rev. %d.%d", MIPS_PRID_REV_MAJ(fpu_id),
			    MIPS_PRID_REV_MIN(fpu_id));
		else
			printf(" Rev. %d", MIPS_PRID_REV(fpu_id));
	}
	printf("\n");

	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC &&
	    MIPS_PRID_RSVD(cpu_id) != 0) {
		printf("%s: NOTE: top 8 bits of prehistoric PRID not 0!\n",
		    label);
		printf("%s: Please mail port-mips@NetBSD.org with cpu0 "
		    "dmesg lines.\n", label);
	}

	KASSERT(mips_picache_ways < nwaynames);
	KASSERT(mips_pdcache_ways < nwaynames);
	KASSERT(mips_sicache_ways < nwaynames);
	KASSERT(mips_sdcache_ways < nwaynames);

	switch (cpu_arch) {
#if defined(MIPS1)
	case CPU_ARCH_MIPS1:
		if (mips_picache_size)
			printf("%s: %dKB/%dB %s Instruction cache, "
			    "%d TLB entries\n", label, mips_picache_size / 1024,
			    mips_picache_line_size, waynames[mips_picache_ways],
			    mips_num_tlb_entries);
		else
			printf("%s: %d TLB entries\n", label,
			    mips_num_tlb_entries);
		if (mips_pdcache_size)
			printf("%s: %dKB/%dB %s %s Data cache\n", label,
			    mips_pdcache_size / 1024, mips_pdcache_line_size,
			    waynames[mips_pdcache_ways],
			    wtnames[mips_pdcache_write_through]);
		break;
#endif /* MIPS1 */
#if defined(MIPS3) || defined(MIPS32) || defined(MIPS64)
	case CPU_ARCH_MIPS3:
	case CPU_ARCH_MIPS4:
	case CPU_ARCH_MIPS32:
	case CPU_ARCH_MIPS64: {
		const char *sufx = "KMGTPE";
		uint32_t pg_mask;
		printf("%s: %d TLB entries", label, mips_num_tlb_entries);
#if !defined(__mips_o32)
		if (CPUIS64BITS) {
			int64_t pfn_mask;
			i = ffs(~(mips3_tlb_vpn_mask >> 31)) + 30;
			printf(", %d%cB (%d-bit) VAs",
			    1 << (i % 10), sufx[(i / 10) - 1], i);
			for (i = 64, pfn_mask = mips3_tlb_pfn_mask << 6;
			     pfn_mask > 0; i--, pfn_mask <<= 1)
				;
			printf(", %d%cB (%d-bit) PAs",
			      1 << (i % 10), sufx[(i / 10) - 1], i);
		}
#endif
		for (i = 4, pg_mask = mips3_tlb_pg_mask >> 13;
		     pg_mask != 0; ) {
			if ((pg_mask & 3) != 3)
				break;
			pg_mask >>= 2;
			i *= 4;
			if (i == 1024) {
				i = 1;
				sufx++;
			}
		}
		printf(", %d%cB max page size\n", i, sufx[0]);
		if (mips_picache_size)
			printf("%s: %dKB/%dB %s L1 Instruction cache\n",
			    label, mips_picache_size / 1024,
			    mips_picache_line_size, waynames[mips_picache_ways]);
		if (mips_pdcache_size)
			printf("%s: %dKB/%dB %s %s L1 Data cache\n", label,
			    mips_pdcache_size / 1024, mips_pdcache_line_size,
			    waynames[mips_pdcache_ways],
			    wtnames[mips_pdcache_write_through]);
		if (mips_sdcache_line_size)
			printf("%s: %dKB/%dB %s %s L2 %s cache\n", label,
			    mips_sdcache_size / 1024, mips_sdcache_line_size,
			    waynames[mips_sdcache_ways],
			    wtnames[mips_sdcache_write_through],
			    mips_scache_unified ? "Unified" : "Data");
		break;
	}
#endif /* MIPS3 */
	default:
		panic("cpu_identify: impossible");
	}
}

/*
 * Set registers on exec.
 * Clear all registers except sp, pc, and t9.
 * $sp is set to the stack pointer passed in.  $pc is set to the entry
 * point given by the exec_package passed in, as is $t9 (used for PIC
 * code by the MIPS elf abi).
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct frame *f = l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);

	memset(f, 0, sizeof(struct frame));
	f->f_regs[_R_SP] = (int)stack;
	f->f_regs[_R_PC] = (int)pack->ep_entry & ~3;
	f->f_regs[_R_T9] = (int)pack->ep_entry & ~3; /* abicall requirement */
	f->f_regs[_R_SR] = PSL_USERSET;
#if !defined(__mips_o32)
	/*
	 * allow 64bit ops in userland for non-O32 ABIs
	 */
	if (l->l_proc->p_md.md_abi != _MIPS_BSD_API_O32)
		f->f_regs[_R_SR] |= MIPS_SR_UX;
	if (_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi))
		f->f_regs[_R_SR] |= MIPS3_SR_FR;
#endif
	/*
	 * Set up arguments for _start():
	 *	_start(stack, obj, cleanup, ps_strings);
	 *
	 * Notes:
	 *	- obj and cleanup are the auxiliary and termination
	 *	  vectors.  They are fixed up by ld.elf_so.
	 *	- ps_strings is a NetBSD extension.
	 */
	f->f_regs[_R_A0] = (intptr_t)stack;
	f->f_regs[_R_A1] = 0;
	f->f_regs[_R_A2] = 0;
	f->f_regs[_R_A3] = (intptr_t)l->l_proc->p_psstr;

	if ((l->l_md.md_flags & MDP_FPUSED) && l == fpcurlwp)
		fpcurlwp = &lwp0;
	memset(&pcb->pcb_fpregs, 0, sizeof(struct fpreg));
	l->l_md.md_flags &= ~MDP_FPUSED;
	l->l_md.md_ss_addr = 0;
}

#ifdef __HAVE_BOOTINFO_H
/*
 * Machine dependent system variables.
 */
static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct btinfo_bootpath *bibp;
	struct sysctlnode node;

	bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	if(!bibp)
		return(ENOENT); /* ??? */

	node = *rnode;
	node.sysctl_data = bibp->bootpath;
	node.sysctl_size = sizeof(bibp->bootpath);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}
#endif

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
#ifdef __HAVE_BOOTINFO_H
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_booted_kernel, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "root_device", NULL,
		       sysctl_root_device, 0, NULL, 0,
		       CTL_MACHDEP, CPU_ROOT_DEVICE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
                       CTLTYPE_INT, "llsc", NULL,
                       NULL, MIPS_HAS_LLSC, NULL, 0,
                       CTL_MACHDEP, CPU_LLSC, CTL_EOL);
}

/*
 * These are imported from platform-specific code.
 * XXX Should be declared in a header file.
 */
extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;

/*
 * These variables are needed by /sbin/savecore.
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

struct pcb dumppcb;

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize(void)
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)) +
	    ALIGN(mem_cluster_cnt * sizeof(phys_ram_seg_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return (-1);

	return (1);
}

/*
 * cpu_dump_mempagecnt: calculate size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;

	n = 0;
	for (i = 0; i < mem_cluster_cnt; i++)
		n += atop(mem_clusters[i].size);
	return (n);
}

/*
 * cpu_dump: dump machine-dependent kernel core dump headers.
 */
int
cpu_dump(void)
{
	int (*dump)(dev_t, daddr_t, void *, size_t);
	char buf[dbtob(1)];
	kcore_seg_t *segp;
	cpu_kcore_hdr_t *cpuhdrp;
	phys_ram_seg_t *memsegp;
	const struct bdevsw *bdev;
	int i;

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return (ENXIO);

	dump = bdev->d_dump;

	memset(buf, 0, sizeof buf);
	segp = (kcore_seg_t *)buf;
	cpuhdrp = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(*segp))];
	memsegp = (phys_ram_seg_t *)&buf[ ALIGN(sizeof(*segp)) +
	    ALIGN(sizeof(*cpuhdrp))];

	/*
	 * Generate a segment header.
	 */
	CORE_SETMAGIC(*segp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	segp->c_size = dbtob(1) - ALIGN(sizeof(*segp));

	/*
	 * Add the machine-dependent header info.
	 */
	if (MIPS_HAS_R4K_MMU) {
		cpuhdrp->archlevel = 3;
		cpuhdrp->pg_shift  = MIPS3_PG_SHIFT;
		cpuhdrp->pg_frame  = MIPS3_PG_FRAME;
		cpuhdrp->pg_v      = MIPS3_PG_V;
	} else {
		cpuhdrp->archlevel = 1;
		cpuhdrp->pg_shift  = MIPS1_PG_SHIFT;
		cpuhdrp->pg_frame  = MIPS1_PG_FRAME;
		cpuhdrp->pg_v      = MIPS1_PG_V;
	}
	cpuhdrp->sysmappa   = MIPS_KSEG0_TO_PHYS(Sysmap);
	cpuhdrp->sysmapsize = Sysmapsize;
	cpuhdrp->nmemsegs   = mem_cluster_cnt;

	/*
	 * Fill in the memory segment descriptors.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		memsegp[i].start = mem_clusters[i].start;
		memsegp[i].size = mem_clusters[i].size;
	}

	return (dump(dumpdev, dumplo, (void *)buf, dbtob(1)));
}

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	const struct bdevsw *bdev;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		goto bad;
	}
	if (bdev->d_psize == NULL)
		goto bad;
	nblks = (*bdev->d_psize)(dumpdev);
	if (nblks <= ctod(1))
		goto bad;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(cpu_dump_mempagecnt());

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;

 bad:
	dumpsize = 0;
}

/*
 * Dump the kernel's image to the swap partition.
 */
#define	BYTES_PER_DUMP	PAGE_SIZE

void
dumpsys(void)
{
	u_long totalbytesleft, bytes, i, n, memcl;
	u_long maddr;
	int psize;
	daddr_t blkno;
	const struct bdevsw *bdev;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	/* XXX should purge all outstanding keystrokes. */

	if ((error = cpu_dump()) != 0)
		goto err;

	totalbytesleft = ptoa(cpu_dump_mempagecnt());
	blkno = dumplo + cpu_dumpsize();
	dump = bdev->d_dump;
	error = 0;

	for (memcl = 0; memcl < mem_cluster_cnt; memcl++) {
		maddr = mem_clusters[memcl].start;
		bytes = mem_clusters[memcl].size;

		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {
			void *maddr_va;

			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0)
				printf_nolog("%ld ",
				    totalbytesleft / (1024 * 1024));

			/* Limit size for next transfer. */
			n = bytes - i;
			if (n > BYTES_PER_DUMP)
				n = BYTES_PER_DUMP;

#ifdef _LP64
			maddr_va = (void *)MIPS_PHYS_TO_XKPHYS_CACHED(maddr);
#else
			maddr_va = (void *)MIPS_PHYS_TO_KSEG0(maddr);
#endif
			error = (*dump)(dumpdev, blkno, maddr_va, n);
			if (error)
				goto err;
			maddr += n;
			blkno += btodb(n);		/* XXX? */

			/* XXX should look for keystrokes, to cancel. */
		}
	}

 err:
	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case EINTR:
		printf("aborted from console\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
	printf("\n\n");
	delay(5000000);		/* 5 seconds */
}

void
mips_init_msgbuf(void)
{
	vsize_t sz = (vsize_t)round_page(MSGBUFSIZE);
	vsize_t reqsz = sz;
	struct vm_physseg *vps;

	vps = VM_PHYSMEM_PTR(vm_nphysseg - 1);

	/* shrink so that it'll fit in the last segment */
	if ((vps->avail_end - vps->avail_start) < atop(sz))
		sz = ptoa(vps->avail_end - vps->avail_start);

	vps->end -= atop(sz);
	vps->avail_end -= atop(sz);
#ifdef _LP64
	msgbufaddr = (void *) MIPS_PHYS_TO_XKPHYS_CACHED(ptoa(vps->end));
#else
	msgbufaddr = (void *) MIPS_PHYS_TO_KSEG0(ptoa(vps->end));
#endif
	initmsgbuf(msgbufaddr, sz);

	/* Remove the last segment if it now has no pages. */
	if (vps->start == vps->end)
		vm_nphysseg--;

	/* warn if the message buffer had to be shrunk */
	if (sz != reqsz)
		printf("WARNING: %"PRIdVSIZE" bytes not available for msgbuf "
		    "in last cluster (%"PRIdVSIZE" used)\n", reqsz, sz);
}

void
mips_init_lwp0_uarea(void)
{
	vaddr_t v;
	struct pcb *pcb0;

	v = uvm_pageboot_alloc(USPACE);
	uvm_lwp_setuarea(&lwp0, v);

	pcb0 = lwp_getpcb(&lwp0);
	lwp0.l_md.md_regs = (struct frame *)(v + USPACE) - 1;
	/*
	 * Now zero out the only two areas of the uarea that we care about.
	 */
	memset(lwp0.l_md.md_regs, 0, sizeof(*lwp0.l_md.md_regs));
	memset(pcb0, 0, sizeof(*pcb0));
	pcb0->pcb_context.val[_L_SR] =
#if !defined(__mips_o32)
	    MIPS_SR_KX |
#endif
	    MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */
}

void
savefpregs(struct lwp *l)
{
#ifndef NOFPU
	struct frame * const f = l->l_md.md_regs;
	uint32_t status, fpcsr;
	
	/*
	 * Don't do anything if the FPU is already off.
	 */
	if ((f->f_regs[_R_SR] & MIPS_SR_COP_1_BIT) == 0)
		return;

	/*
	 * this process yielded FPA.
	 */
	KASSERT(f->f_regs[_R_SR] & MIPS_SR_COP_1_BIT);	/* it should be on */

	/*
	 * turnoff interrupts enabling CP1 to read FPCSR register.
	 */
	__asm volatile (
		".set noreorder"				"\n\t"
		".set noat"					"\n\t"
		"mfc0	%0, $" ___STRING(MIPS_COP_0_STATUS) 	"\n\t"
		"mtc0	%2, $" ___STRING(MIPS_COP_0_STATUS)	"\n\t"
		___STRING(COP0_HAZARD_FPUENABLE)
		"cfc1	%1, $31"				"\n\t"
		"cfc1	%1, $31"				"\n\t"
		".set reorder"					"\n\t"
		".set at" 
		: "=&r" (status), "=r"(fpcsr)
		: "r"(f->f_regs[_R_SR] & (MIPS_SR_COP_1_BIT|MIPS3_SR_FR|MIPS_SR_KX)));

	/*
	 * Make sure we don't reenable FP when we return to userspace.
	 */
	f->f_regs[_R_SR] ^= MIPS_SR_COP_1_BIT;

	/*
	 * save FPCSR and 32bit FP register values.
	 */
	struct pcb * const pcb = lwp_getpcb(l);
	mips_fpreg_t * const fp = pcb->pcb_fpregs.r_regs;
#if !defined(__mips_o32)
	if (f->f_regs[_R_SR] & MIPS3_SR_FR) {
		KASSERT(_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		fp[32] = fpcsr;
		__asm volatile (
			".set noreorder			;"
			"sdc1	$f0, (0*%d1)(%0)	;"
			"sdc1	$f1, (1*%d1)(%0)	;"
			"sdc1	$f2, (2*%d1)(%0)	;"
			"sdc1	$f3, (3*%d1)(%0)	;"
			"sdc1	$f4, (4*%d1)(%0)	;"
			"sdc1	$f5, (5*%d1)(%0)	;"
			"sdc1	$f6, (6*%d1)(%0)	;"
			"sdc1	$f7, (7*%d1)(%0)	;"
			"sdc1	$f8, (8*%d1)(%0)	;"
			"sdc1	$f9, (9*%d1)(%0)	;"
			"sdc1	$f10, (10*%d1)(%0)	;"
			"sdc1	$f11, (11*%d1)(%0)	;"
			"sdc1	$f12, (12*%d1)(%0)	;"
			"sdc1	$f13, (13*%d1)(%0)	;"
			"sdc1	$f14, (14*%d1)(%0)	;"
			"sdc1	$f15, (15*%d1)(%0)	;"
			"sdc1	$f16, (16*%d1)(%0)	;"
			"sdc1	$f17, (17*%d1)(%0)	;"
			"sdc1	$f18, (18*%d1)(%0)	;"
			"sdc1	$f19, (19*%d1)(%0)	;"
			"sdc1	$f20, (20*%d1)(%0)	;"
			"sdc1	$f21, (21*%d1)(%0)	;"
			"sdc1	$f22, (22*%d1)(%0)	;"
			"sdc1	$f23, (23*%d1)(%0)	;"
			"sdc1	$f24, (24*%d1)(%0)	;"
			"sdc1	$f25, (25*%d1)(%0)	;"
			"sdc1	$f26, (26*%d1)(%0)	;"
			"sdc1	$f27, (27*%d1)(%0)	;"
			"sdc1	$f28, (28*%d1)(%0)	;"
			"sdc1	$f29, (29*%d1)(%0)	;"
			"sdc1	$f30, (30*%d1)(%0)	;"
			"sdc1	$f31, (31*%d1)(%0)	;"
			".set reorder" :: "r"(fp), "i"(sizeof(fp[0])));
	} else
#endif /* !defined(__mips_o32) */
	{
		KASSERT(!_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		((int *)fp)[32] = fpcsr;
		__asm volatile (
			".set noreorder			;"
			"swc1	$f0, (0*%d1)(%0)	;"
			"swc1	$f1, (1*%d1)(%0)	;"
			"swc1	$f2, (2*%d1)(%0)	;"
			"swc1	$f3, (3*%d1)(%0)	;"
			"swc1	$f4, (4*%d1)(%0)	;"
			"swc1	$f5, (5*%d1)(%0)	;"
			"swc1	$f6, (6*%d1)(%0)	;"
			"swc1	$f7, (7*%d1)(%0)	;"
			"swc1	$f8, (8*%d1)(%0)	;"
			"swc1	$f9, (9*%d1)(%0)	;"
			"swc1	$f10, (10*%d1)(%0)	;"
			"swc1	$f11, (11*%d1)(%0)	;"
			"swc1	$f12, (12*%d1)(%0)	;"
			"swc1	$f13, (13*%d1)(%0)	;"
			"swc1	$f14, (14*%d1)(%0)	;"
			"swc1	$f15, (15*%d1)(%0)	;"
			"swc1	$f16, (16*%d1)(%0)	;"
			"swc1	$f17, (17*%d1)(%0)	;"
			"swc1	$f18, (18*%d1)(%0)	;"
			"swc1	$f19, (19*%d1)(%0)	;"
			"swc1	$f20, (20*%d1)(%0)	;"
			"swc1	$f21, (21*%d1)(%0)	;"
			"swc1	$f22, (22*%d1)(%0)	;"
			"swc1	$f23, (23*%d1)(%0)	;"
			"swc1	$f24, (24*%d1)(%0)	;"
			"swc1	$f25, (25*%d1)(%0)	;"
			"swc1	$f26, (26*%d1)(%0)	;"
			"swc1	$f27, (27*%d1)(%0)	;"
			"swc1	$f28, (28*%d1)(%0)	;"
			"swc1	$f29, (29*%d1)(%0)	;"
			"swc1	$f30, (30*%d1)(%0)	;"
			"swc1	$f31, (31*%d1)(%0)	;"
		".set reorder" :: "r"(fp), "i"(4));
	}
	/*
	 * stop CP1, enable interrupts.
	 */
	__asm volatile ("mtc0 %0, $" ___STRING(MIPS_COP_0_STATUS)
	    :: "r"(status));
#endif /* !defined(NOFPU) */
}

void
loadfpregs(struct lwp *l)
{
#ifndef NOFPU
	struct frame * const f = l->l_md.md_regs;
	uint32_t status;
	uint32_t fpcsr;

	/*
	 * Got turned, maybe due to savefpregs.
	 */
	if (fpcurlwp == l) {
		f->f_regs[_R_SR] |= MIPS_SR_COP_1_BIT;
		return;
	} else {
		savefpregs(fpcurlwp);
		fpcurlwp = l;
	}

	/*
	 * Enable the FP when this lwp return to userspace.
	 */
	f->f_regs[_R_SR] |= MIPS_SR_COP_1_BIT;

	/*
	 * turnoff interrupts enabling CP1 to load FP registers.
	 */
	__asm volatile(
		".set noreorder"				"\n\t"
		".set noat"					"\n\t"
		"mfc0	%0, $" ___STRING(MIPS_COP_0_STATUS)	"\n\t"
		"mtc0	%1, $" ___STRING(MIPS_COP_0_STATUS)	"\n\t"
		___STRING(COP0_HAZARD_FPUENABLE)
		".set reorder"					"\n\t"
		".set at"
	    : "=&r"(status)
	    : "r"(f->f_regs[_R_SR] & (MIPS_SR_COP_1_BIT|MIPS3_SR_FR|MIPS_SR_KX)));

	struct pcb * const pcb = lwp_getpcb(l);
	mips_fpreg_t * const fp = pcb->pcb_fpregs.r_regs;
	/*
	 * load FP registers and establish processes' FP context.
	 */
#if !defined(__mips_o32)
	if (f->f_regs[_R_SR] & MIPS3_SR_FR) {
		KASSERT(_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		__asm volatile (
			".set noreorder			;"
			"ldc1	$f0, (0*%d1)(%0)	;"
			"ldc1	$f1, (1*%d1)(%0)	;"
			"ldc1	$f2, (2*%d1)(%0)	;"
			"ldc1	$f3, (3*%d1)(%0)	;"
			"ldc1	$f4, (4*%d1)(%0)	;"
			"ldc1	$f5, (5*%d1)(%0)	;"
			"ldc1	$f6, (6*%d1)(%0)	;"
			"ldc1	$f7, (7*%d1)(%0)	;"
			"ldc1	$f8, (8*%d1)(%0)	;"
			"ldc1	$f9, (9*%d1)(%0)	;"
			"ldc1	$f10, (10*%d1)(%0)	;"
			"ldc1	$f11, (11*%d1)(%0)	;"
			"ldc1	$f12, (12*%d1)(%0)	;"
			"ldc1	$f13, (13*%d1)(%0)	;"
			"ldc1	$f14, (14*%d1)(%0)	;"
			"ldc1	$f15, (15*%d1)(%0)	;"
			"ldc1	$f16, (16*%d1)(%0)	;"
			"ldc1	$f17, (17*%d1)(%0)	;"
			"ldc1	$f18, (18*%d1)(%0)	;"
			"ldc1	$f19, (19*%d1)(%0)	;"
			"ldc1	$f20, (20*%d1)(%0)	;"
			"ldc1	$f21, (21*%d1)(%0)	;"
			"ldc1	$f22, (22*%d1)(%0)	;"
			"ldc1	$f23, (23*%d1)(%0)	;"
			"ldc1	$f24, (24*%d1)(%0)	;"
			"ldc1	$f25, (25*%d1)(%0)	;"
			"ldc1	$f26, (26*%d1)(%0)	;"
			"ldc1	$f27, (27*%d1)(%0)	;"
			"ldc1	$f28, (28*%d1)(%0)	;"
			"ldc1	$f29, (29*%d1)(%0)	;"
			"ldc1	$f30, (30*%d1)(%0)	;"
			"ldc1	$f31, (31*%d1)(%0)	;"
			".set reorder" :: "r"(fp), "i"(sizeof(fp[0])));
		fpcsr = fp[32];
	} else
#endif
	{
		KASSERT(!_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi));
		__asm volatile (
			".set noreorder			;"
			"lwc1	$f0, (0*%d1)(%0)	;"
			"lwc1	$f1, (1*%d1)(%0)	;"
			"lwc1	$f2, (2*%d1)(%0)	;"
			"lwc1	$f3, (3*%d1)(%0)	;"
			"lwc1	$f4, (4*%d1)(%0)	;"
			"lwc1	$f5, (5*%d1)(%0)	;"
			"lwc1	$f6, (6*%d1)(%0)	;"
			"lwc1	$f7, (7*%d1)(%0)	;"
			"lwc1	$f8, (8*%d1)(%0)	;"
			"lwc1	$f9, (9*%d1)(%0)	;"
			"lwc1	$f10, (10*%d1)(%0)	;"
			"lwc1	$f11, (11*%d1)(%0)	;"
			"lwc1	$f12, (12*%d1)(%0)	;"
			"lwc1	$f13, (13*%d1)(%0)	;"
			"lwc1	$f14, (14*%d1)(%0)	;"
			"lwc1	$f15, (15*%d1)(%0)	;"
			"lwc1	$f16, (16*%d1)(%0)	;"
			"lwc1	$f17, (17*%d1)(%0)	;"
			"lwc1	$f18, (18*%d1)(%0)	;"
			"lwc1	$f19, (19*%d1)(%0)	;"
			"lwc1	$f20, (20*%d1)(%0)	;"
			"lwc1	$f21, (21*%d1)(%0)	;"
			"lwc1	$f22, (22*%d1)(%0)	;"
			"lwc1	$f23, (23*%d1)(%0)	;"
			"lwc1	$f24, (24*%d1)(%0)	;"
			"lwc1	$f25, (25*%d1)(%0)	;"
			"lwc1	$f26, (26*%d1)(%0)	;"
			"lwc1	$f27, (27*%d1)(%0)	;"
			"lwc1	$f28, (28*%d1)(%0)	;"
			"lwc1	$f29, (29*%d1)(%0)	;"
			"lwc1	$f30, (30*%d1)(%0)	;"
			"lwc1	$f31, (31*%d1)(%0)	;"
			".set reorder"
		    :
		    : "r"(fp), "i"(4));
		fpcsr = ((int *)fp)[32];
	}

	/*
	 * load FPCSR and stop CP1 again while enabling interrupts.
	 */
	__asm volatile(
		".set noreorder"				"\n\t"
		".set noat"					"\n\t"
		"ctc1	%0, $31"				"\n\t"
		"mtc0	%1, $" ___STRING(MIPS_COP_0_STATUS)	"\n\t"
		".set reorder"					"\n\t"
		".set at"
		:: "r"(fpcsr &~ MIPS_FPU_EXCEPTION_BITS), "r"(status));
#endif /* !defined(NOFPU) */
}

/* 
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = arg;
	lwp_t *l = curlwp;
	int error;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}

#ifdef COMPAT_NETBSD32
/* 
 * Start a new LWP
 */
void
startlwp32(void *arg)
{
	ucontext32_t *uc = arg;
	lwp_t *l = curlwp;
	int error;

	error = cpu_setmcontext32(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	/* Note: we are freeing ucontext_t, not ucontext32_t. */
	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}
#endif /* COMPAT_NETBSD32 */

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	userret(l);
}

void 
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
    void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct frame *f = l->l_md.md_regs;
	struct saframe frame;
#ifdef COMPAT_NETBSD32
	struct saframe32 frame32;
#endif
	void *ksf, *usf;
	size_t sfsz;

#if 0 /* First 4 args in regs (see below). */
	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
#endif
#ifdef COMPAT_NETBSD32
	switch (l->l_proc->p_md.md_abi) {
	case _MIPS_BSD_API_O32:
	case _MIPS_BSD_API_N32:
		NETBSD32PTR32(frame32.sa_arg, ap);
		NETBSD32PTR32(frame32.sa_upcall, upcall);
		ksf = &frame32;
		usf = (struct saframe32 *)sp - 1;
		sfsz = sizeof(frame32);
		break;
	default:
#endif
		frame.sa_arg = ap;
		frame.sa_upcall = upcall;
		ksf = &frame;
		usf = (struct saframe *)sp - 1;
		sfsz = sizeof(frame);
#ifdef COMPAT_NETBSD32
		break;
	}
#endif

	if (copyout(ksf, usf, sfsz) != 0) {
		/* Copying onto the stack didn't work. Die. */
		mutex_enter(l->l_proc->p_lock);
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	f->f_regs[_R_PC] = (intptr_t)upcall;
	f->f_regs[_R_SP] = (intptr_t)usf;
	f->f_regs[_R_A0] = type;
	f->f_regs[_R_A1] = (intptr_t)sas;
	f->f_regs[_R_A2] = nevents;
	f->f_regs[_R_A3] = ninterrupted;
	f->f_regs[_R_S8] = 0;
	f->f_regs[_R_RA] = 0;
	f->f_regs[_R_T9] = (intptr_t)upcall;  /* t9=Upcall function*/
}


void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct frame *f = l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;

	/* Save register context. Dont copy R0 - it is always 0 */
	memcpy(&gr[_REG_AT], &f->f_regs[_R_AST], sizeof(mips_reg_t) * 31);

	gr[_REG_MDLO]  = f->f_regs[_R_MULLO];
	gr[_REG_MDHI]  = f->f_regs[_R_MULHI];
	gr[_REG_CAUSE] = f->f_regs[_R_CAUSE];
	gr[_REG_EPC]   = f->f_regs[_R_PC];
	gr[_REG_SR]    = f->f_regs[_R_SR];

	if ((ras_pc = (intptr_t)ras_lookup(l->l_proc,
	    (void *) (intptr_t)gr[_REG_EPC])) != -1)
		gr[_REG_EPC] = ras_pc;

	*flags |= _UC_CPU;

	/* Save floating point register context, if any. */
	if (l->l_md.md_flags & MDP_FPUSED) {
		struct pcb *pcb;
		size_t fplen;

		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 */
		if (l == fpcurlwp)
			savefpregs(l);

		/*
		 * The PCB FP regs struct includes the FP CSR, so use the
		 * size of __fpregs.__fp_r when copying.
		 */
		pcb = lwp_getpcb(l);
#if !defined(__mips_o32)
		if (_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi)) {
#endif
			fplen = sizeof(struct fpreg);
#if !defined(__mips_o32)
		} else {
			fplen = sizeof(struct fpreg_oabi);
		}
#endif
		memcpy(&mcp->__fpregs, &pcb->pcb_fpregs, fplen);
		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct frame *f = l->l_md.md_regs;
	struct proc *p = l->l_proc;
	const __greg_t *gr = mcp->__gregs;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		/* Save register context. */
		/* XXX:  Do we validate the addresses?? */
		memcpy(&f->f_regs[_R_AST], &gr[_REG_AT],
		       sizeof(mips_reg_t) * 31);

		f->f_regs[_R_MULLO] = gr[_REG_MDLO];
		f->f_regs[_R_MULHI] = gr[_REG_MDHI];
		f->f_regs[_R_CAUSE] = gr[_REG_CAUSE];
		f->f_regs[_R_PC]    = gr[_REG_EPC];
		/* Do not restore SR. */
	}

	/* Restore floating point register context, if any. */
	if (flags & _UC_FPU) {
		struct pcb *pcb;
		size_t fplen;

		/* Disable the FPU to fault in FP registers. */
		f->f_regs[_R_SR] &= ~MIPS_SR_COP_1_BIT;
		fpcurlwp = &lwp0;

#if !defined(__mips_o32)
		if (_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi)) {
#endif
			fplen = sizeof(struct fpreg);
#if !defined(__mips_o32)
		} else {
			fplen = sizeof(struct fpreg_oabi);
		}
#endif
		/*
		 * The PCB FP regs struct includes the FP CSR, so use the
		 * proper size of fpreg when copying.
		 */
		pcb = lwp_getpcb(l);
		memcpy(&pcb->pcb_fpregs, &mcp->__fpregs, fplen);
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return (0);
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{

	aston(ci->ci_data.cpu_onproc);
	ci->ci_want_resched = 1;
}

void
cpu_idle(void)
{
	void (*mach_idle)(void) = mips_locoresw.lsw_cpu_idle;

	while (!curcpu()->ci_want_resched)
		(*mach_idle)();
}

bool
cpu_intr_p(void)
{

	return curcpu()->ci_idepth != 0;
}
