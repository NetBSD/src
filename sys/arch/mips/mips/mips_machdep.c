/*	$NetBSD: mips_machdep.c,v 1.120.2.10 2002/04/01 07:41:10 nathanw Exp $	*/

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
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

__KERNEL_RCSID(0, "$NetBSD: mips_machdep.c,v 1.120.2.10 2002/04/01 07:41:10 nathanw Exp $");

#include "opt_cputype.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/reboot.h>
#include <sys/mount.h>			/* fsid_t for syscallargs */
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/user.h>
#include <sys/msgbuf.h>
#include <sys/conf.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ucontext.h>
#include <machine/kcore.h>

#include <sys/sa.h>
#include <sys/savar.h>
#include <uvm/uvm_extern.h>

#include <mips/cache.h>
#include <mips/frame.h>
#include <mips/regnum.h>
#include <mips/locore.h>
#include <mips/psl.h>
#include <mips/pte.h>
#include <machine/cpu.h>

#if defined(MIPS32) || defined(MIPS64)
#include <mips/mipsNN.h>		/* MIPS32/MIPS64 registers */
#endif

/* Internal routines. */
int	cpu_dumpsize(void);
u_long	cpu_dump_mempagecnt(void);
int	cpu_dump(void);

#if defined(MIPS1)
static void	mips1_vector_init(void);
extern long	*mips1_locoresw[];
#endif

#if defined(MIPS3)
#if defined(MIPS3_5900)
static void	r5900_vector_init(void);
extern long	*mips5900_locoresw[];
#else
static void	mips3_vector_init(void);
extern long	*mips3_locoresw[];
#endif
#endif

#if defined(MIPS32)
static void	mips32_vector_init(void);
extern long	*mips32_locoresw[];
#endif

#if defined(MIPS64)
static void	mips64_vector_init(void);
extern long	*mips64_locoresw[];
#endif

mips_locore_jumpvec_t mips_locore_jumpvec;

long *mips_locoresw[3];

int cpu_arch;
int cpu_mhz;
int mips_num_tlb_entries;
int mips_cpu_flags;
int mips_has_llsc;
int mips_has_r4k_mmu;
int mips3_pg_cached;

struct	user *proc0paddr;
struct	lwp  *fpcurproc;
struct	pcb  *curpcb;
struct	segtab *segbase;

caddr_t	msgbufaddr;

#ifdef MIPS3_4100			/* VR4100 core */
int	default_pg_mask = 0x00001800;
#endif

struct pridtab {
	int	cpu_cid;
	int	cpu_pid;
	int	cpu_rev;	/* -1 == wildcard */
	int	cpu_isa;	/* -1 == probed (mips32/mips64) */
	int	cpu_ntlb;	/* -1 == unknown, 0 == probed */
	int	cpu_flags;
	char	*cpu_name;
};

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
#define	MIPS32_FLAGS	CPU_MIPS_R4K_MMU | CPU_MIPS_CAUSE_IV
#define	MIPS64_FLAGS	MIPS32_FLAGS	/* same as MIPS32 flags (for now) */

static const struct pridtab *mycpu;

static const struct pridtab cputab[] = {
	{ 0, MIPS_R2000, -1,			CPU_ARCH_MIPS1, 64,
	  0,					"MIPS R2000 CPU"	},
	{ 0, MIPS_R3000, MIPS_REV_R3000,	CPU_ARCH_MIPS1, 64,
	  0,					"MIPS R3000 CPU"	},
	{ 0, MIPS_R3000, MIPS_REV_R3000A,	CPU_ARCH_MIPS1, 64,
	  0,					"MIPS R3000A CPU"	},
	{ 0, MIPS_R6000, -1,			CPU_ARCH_MIPS2, 32,
	  MIPS_NOT_SUPP,			"MIPS R6000 CPU"	},

	/*
	 * rev 0x00 and 0x30 are R4000, 0x40 and 0x50 are R4400.
	 * should we allow ranges and use 0x00 - 0x3f for R4000 and
	 * 0x40 - 0xff for R4400?
	 */
	{ 0, MIPS_R4000, MIPS_REV_R4000_A,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"MIPS R4000 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4000_B,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"MIPS R4000 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4400_A,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"MIPS R4400 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4400_B,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"MIPS R4400 CPU"	},
	{ 0, MIPS_R4000, MIPS_REV_R4400_C,	CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"MIPS R4400 CPU"	},

	{ 0, MIPS_R3LSI, -1,			CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP,			"LSI Logic R3000 derivative" },
	{ 0, MIPS_R6000A, -1,			CPU_ARCH_MIPS2, 32,
	  MIPS_NOT_SUPP,			"MIPS R6000A CPU"	},
	{ 0, MIPS_R3IDT, -1,			CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP,			"IDT R3041 or RC36100 CPU" },
	{ 0, MIPS_R4100, -1,			CPU_ARCH_MIPS3, 32,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_NO_LLSC,	"NEC VR4100 CPU"	},
	{ 0, MIPS_R4200, -1,			CPU_ARCH_MIPS3, -1,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"NEC VR4200 CPU"	},
	{ 0, MIPS_R4300, -1,			CPU_ARCH_MIPS3, 32,
	  CPU_MIPS_R4K_MMU,			"NEC VR4300 CPU"	},
	{ 0, MIPS_R4600, -1,			CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"QED R4600 Orion CPU"	},
	{ 0, MIPS_R4700, -1,			CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"QED R4700 Orion CPU"	},

	{ 0, MIPS_R8000, -1,			CPU_ARCH_MIPS4, 384,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"MIPS R8000 Blackbird/TFP CPU" },
	{ 0, MIPS_R10000, -1,			CPU_ARCH_MIPS4, 64,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"MIPS R10000 CPU"	},
	{ 0, MIPS_R12000, -1,			CPU_ARCH_MIPS4, 64,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"MIPS R12000 CPU"	},
	{ 0, MIPS_R14000, -1,			CPU_ARCH_MIPS4, 64,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"MIPS R14000 CPU"	},

	/* XXX
	 * If the Processor Revision ID of the 4650 isn't 0, the following
	 * entry needs to be adjusted.  Can't use a wildcard match because
	 * the TX39 series processors share the same Processor ID value.
	 * Or maybe put TX39 CPUs first if the revid doesn't overlap with
	 * the 4650...
	 */
	{ 0, MIPS_R4650, 0,			CPU_ARCH_MIPS3, -1,
	  MIPS_NOT_SUPP /* no MMU! */,		"QED R4650 CPU"	},
	{ 0, MIPS_TX3900, MIPS_REV_TX3912,	CPU_ARCH_MIPS1, 32,
	  0,					"Toshiba TX3912 CPU"	},
	{ 0, MIPS_TX3900, MIPS_REV_TX3922,	CPU_ARCH_MIPS1, 64,
	  0,					"Toshiba TX3922 CPU"	},
	{ 0, MIPS_TX3900, MIPS_REV_TX3927,	CPU_ARCH_MIPS1, 64,
	  0,					"Toshiba TX3927 CPU"	},
	{ 0, MIPS_R5000, -1,			CPU_ARCH_MIPS4, 48,
	  CPU_MIPS_R4K_MMU,			"MIPS R5000 CPU"	},
	{ 0, MIPS_RM5200, -1,			CPU_ARCH_MIPS4, 48,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_CAUSE_IV,	"QED RM5200 CPU"	},

	/* XXX
	 * The rm7000 rev 2.0 can have 64 tlbs, and has 6 extra interrupts.  See
	 *    "Migrating to the RM7000 from other MIPS Microprocessors"
	 * for more details.
	 */
	{ 0, MIPS_RM7000, -1,			CPU_ARCH_MIPS4, 48,
	  MIPS_NOT_SUPP | CPU_MIPS_CAUSE_IV,	"QED RM7000 CPU"	},

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
	{ 0, MIPS_RC32300, -1,			CPU_ARCH_MIPS3, 16,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"IDT RC32300 CPU"	},
	{ 0, MIPS_RC32364, -1,			CPU_ARCH_MIPS3, 16,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"IDT RC32364 CPU"	},
	{ 0, MIPS_RC64470, -1,			CPU_ARCH_MIPSx, -1,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"IDT RC64474/RC64475 CPU" },

	{ 0, MIPS_R5400, -1,			CPU_ARCH_MIPSx, -1,
	  MIPS_NOT_SUPP | CPU_MIPS_R4K_MMU,	"NEC VR5400 CPU"	},
	{ 0, MIPS_R5900, -1,			CPU_ARCH_MIPS3, 48,
	  CPU_MIPS_R4K_MMU,			"Toshiba R5900 CPU"	},

#if 0 /* ID collisions : can we use a CU1 test or similar? */
	{ 0, MIPS_R3SONY, -1,			CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP,			"SONY R3000 derivative"	},	/* 0x21; crash R4700? */
	{ 0, MIPS_R3NKK, -1,			CPU_ARCH_MIPS1, -1,
	  MIPS_NOT_SUPP,			"NKK R3000 derivative"	},	/* 0x23; crash R5000? */
#endif

	{ MIPS_PRID_CID_MTI, MIPS_4Kc, -1,	-1, 0,
	  MIPS32_FLAGS,				"4Kc"			},
	{ MIPS_PRID_CID_MTI, MIPS_4KEc, -1,	-1, 0,
	  MIPS32_FLAGS,				"4KEc"			},
	{ MIPS_PRID_CID_MTI, MIPS_4KSc, -1,	-1, 0,
	  MIPS32_FLAGS,				"4KSc"			},
	{ MIPS_PRID_CID_MTI, MIPS_5Kc, -1,	-1, 0,
	  MIPS64_FLAGS,				"5Kc"			},

	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU1000_R1, -1, -1, 0,
	  MIPS32_FLAGS,				"Au1000 (Rev 1)"	},
	{ MIPS_PRID_CID_ALCHEMY, MIPS_AU1000_R2, -1, -1, 0,
	  MIPS32_FLAGS,				"Au1000 (Rev 2)" 	},

	/* The SB1 CPUs use a CCA of 5 - "Cacheable Coherent Shareable" */
	{ MIPS_PRID_CID_SIBYTE, MIPS_SB1, -1,	-1, 0,
	  MIPS64_FLAGS | CPU_MIPS_HAVE_SPECIAL_CCA | \
	  (5 << CPU_MIPS_CACHED_CCA_SHIFT),	"SB1"			},

	{ 0, 0, 0,				0, 64,
	  0,					NULL			}
};

static const struct pridtab fputab[] = {
	{ 0, MIPS_SOFT,	-1, 0, 0, 0,	"software emulated floating point" },
	{ 0, MIPS_R2360, -1, 0, 0, 0,	"MIPS R2360 Floating Point Board" },
	{ 0, MIPS_R2010, -1, 0, 0, 0,	"MIPS R2010 FPC" },
	{ 0, MIPS_R3010, -1, 0, 0, 0,	"MIPS R3010 FPC" },
	{ 0, MIPS_R6010, -1, 0, 0, 0,	"MIPS R6010 FPC" },
	{ 0, MIPS_R4010, -1, 0, 0, 0,	"MIPS R4010 FPC" },
	{ 0, MIPS_R10000, -1, 0, 0, 0,	"built-in FPU" },
};

/*
 * Company ID's are not sparse (yet), this array is indexed directly
 * by pridtab->cpu_cid.
 */
static const char *cidnames[] = {
	"Prehistoric",
	"MIPS",		/* or "MIPS Technologies, Inc.	*/
	"Broadcom",	/* or "Broadcom Corp."		*/
	"Alchemy",	/* or "Alchemy Semiconductor"	*/
	"SiByte",	/* or "Broadcom Corp. (SiByte)"	*/
	"SandCraft",
};
#define	ncidnames (sizeof(cidnames) / sizeof(cidnames[0]))

#ifdef MIPS1
/*
 * MIPS-I locore function vector
 */
static const mips_locore_jumpvec_t mips1_locore_vec =
{
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
	memcpy(&mips_locore_jumpvec, &mips1_locore_vec,
		sizeof(mips_locore_jumpvec_t));

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
static const mips_locore_jumpvec_t mips3_locore_vec =
{
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
	memcpy(&mips_locore_jumpvec, &mips3_locore_vec,
	      sizeof(mips_locore_jumpvec_t));

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS3_SR_DIAG_BEV);
}
#endif /* !MIPS3_5900 */

#ifdef MIPS3_5900	/* XXX */
/*
 * MIPS R5900 locore function vector.
 * Same as MIPS32 - all MMU registers are 32bit.
 */
static const mips_locore_jumpvec_t r5900_locore_vec =
{
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

	memcpy(&mips_locore_jumpvec, &r5900_locore_vec,
	    sizeof(mips_locore_jumpvec_t));

	mips_config_cache();

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS3_SR_DIAG_BEV);
}
#endif /* MIPS3_5900 */
#endif /* MIPS3 */

#ifdef MIPS32
/*
 * MIPS32 locore function vector
 */
static const mips_locore_jumpvec_t mips32_locore_vec =
{
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
	memcpy(&mips_locore_jumpvec, &mips32_locore_vec,
		      sizeof(mips_locore_jumpvec_t));

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS3_SR_DIAG_BEV);
}
#endif /* MIPS32 */

#ifdef MIPS64
/*
 * MIPS64 locore function vector
 */
const mips_locore_jumpvec_t mips64_locore_vec =
{
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
	memcpy(&mips_locore_jumpvec, &mips64_locore_vec,
	      sizeof(mips_locore_jumpvec_t));

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS3_SR_DIAG_BEV);
}
#endif /* MIPS64 */

/*
 * Do all the stuff that locore normally does before calling main(),
 * that is common to all mips-CPU NetBSD ports.
 *
 * The principal purpose of this function is to examine the
 * variable cpu_id, into which the kernel locore start code
 * writes the cpu ID register, and to then copy appropriate
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

	mycpu = NULL;
	for (ct = cputab; ct->cpu_name != NULL; ct++) {
//printf("test 0x%x 0x%x 0x%x with 0x%x 0x%x 0x%x (%s - isa %d)\n", MIPS_PRID_CID(cpu_id), MIPS_PRID_IMPL(cpu_id), MIPS_PRID_REV(cpu_id), ct->cpu_cid, ct->cpu_pid, ct->cpu_rev, ct->cpu_name, ct->cpu_isa);
		if (MIPS_PRID_CID(cpu_id) != ct->cpu_cid ||
		    MIPS_PRID_IMPL(cpu_id) != ct->cpu_pid)
			continue;
		if (ct->cpu_rev >= 0 &&
		    MIPS_PRID_REV(cpu_id) != ct->cpu_rev)
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

		if (MIPSNN_GET(CFG_AR, cfg) != MIPSNN_CFG_AR_REV1)
			printf("WARNING: MIPS32/64 arch revision != revision 1!\n");

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
		panic("Unknown CPU ISA for CPU type 0x%x\n", cpu_id);
	if (mips_num_tlb_entries < 1)
		panic("Unknown number of TLBs for CPU type 0x%x\n", cpu_id);

	/*
	 * Check cpu-specific flags.
	 */
	mips_has_r4k_mmu = mycpu->cpu_flags & CPU_MIPS_R4K_MMU;
	mips_has_llsc = !(mycpu->cpu_flags & CPU_MIPS_NO_LLSC);

	if (mycpu->cpu_flags & CPU_MIPS_HAVE_SPECIAL_CCA) {
		uint32_t cca;

		cca = (ct->cpu_flags & CPU_MIPS_CACHED_CCA_MASK) >>
		    CPU_MIPS_CACHED_CCA_SHIFT;
		mips3_pg_cached = MIPS3_CCA_TO_PG(cca);
	} else
		mips3_pg_cached = MIPS3_DEFAULT_PG_CACHED;

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
#ifdef MIPS1
	case CPU_ARCH_MIPS1:
		mips1_TBIA(mips_num_tlb_entries);
		mips1_vector_init();
		memcpy(mips_locoresw, mips1_locoresw, sizeof(mips_locoresw));
		break;
#endif
#if defined(MIPS3)
	case CPU_ARCH_MIPS3:
	case CPU_ARCH_MIPS4:
#ifdef MIPS3_5900	/* XXX */
		mips3_cp0_wired_write(0);
		mips5900_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		r5900_vector_init();
		memcpy(mips_locoresw, mips5900_locoresw, sizeof(mips_locoresw));
#else /* MIPS3_5900 */
		mips3_cp0_wired_write(0);
		mips3_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		mips3_vector_init();
		memcpy(mips_locoresw, mips3_locoresw, sizeof(mips_locoresw));
#endif /* MIPS3_5900 */
		break;
#endif
#ifdef MIPS32
	case CPU_ARCH_MIPS32:
		mips3_cp0_wired_write(0);
		mips32_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		mips32_vector_init();
		memcpy(mips_locoresw, mips32_locoresw, sizeof(mips_locoresw));
		break;
#endif
#if defined(MIPS64)
	case CPU_ARCH_MIPS64:
		mips3_cp0_wired_write(0);
		mips64_TBIA(mips_num_tlb_entries);
		mips3_cp0_wired_write(MIPS3_TLB_WIRED_UPAGES);
		mips64_vector_init();
		memcpy(mips_locoresw, mips64_locoresw, sizeof(mips_locoresw));
		break;
#endif
	default:
		printf("cpu_arch 0x%x: not supported\n", cpu_arch);
		cpu_reboot(RB_HALT, NULL);
	}

	/*
	 * Install power-saving idle routines.
	 */
	switch (MIPS_PRID_CID(cpu_id)) {
	case MIPS_PRID_CID_PREHISTORIC:
		switch (MIPS_PRID_IMPL(cpu_id)) {
#if defined(MIPS3) && !defined(MIPS3_5900)
		case MIPS_RM5200:
		case MIPS_RM7000:
		    {
			void rm52xx_idle(void);

			CPU_IDLE = (long *) rm52xx_idle;
			break;
		    }
#endif /* MIPS3 && !MIPS3_5900 */
		default:
			/* Nothing. */
			break;
		}
#if defined(MIPS32) || defined(MIPS64)
	default:
	    {
		/*
		 * XXX: wait is valid on all mips32/64, but do we
		 *	always want to use it?
		 */
		void mipsNN_idle(void);

		CPU_IDLE = (long *) mipsNN_idle;
	    }
#endif
	}
}

void
mips_set_wbflush(flush_fn)
	void (*flush_fn)(void);
{
#undef wbflush
	mips_locore_jumpvec.wbflush = flush_fn;
	(*flush_fn)();
}

/*
 * Identify product revision IDs of cpu and fpu.
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
	};
#define	nwaynames (sizeof(waynames) / sizeof(waynames[0]))
	static const char * const wtnames[] = {
		"write-back",
		"write-through",
	};
	static const char * const label = "cpu0";	/* XXX */
	char *cpuname, *fpuname;
	int i;

	cpuname = mycpu->cpu_name;

	fpuname = NULL;
	for (i = 0; i < sizeof(fputab)/sizeof(fputab[0]); i++) {
		if (MIPS_PRID_CID(fpu_id) == fputab[i].cpu_cid &&
		    MIPS_PRID_IMPL(fpu_id) == fputab[i].cpu_pid) {
			fpuname = fputab[i].cpu_name;
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
		printf("%s: Please mail port-mips@netbsd.org with cpu0 "
		    "dmesg lines.\n", label);
	}

	KASSERT(mips_picache_ways < nwaynames);
	KASSERT(mips_pdcache_ways < nwaynames);
	KASSERT(mips_sicache_ways < nwaynames);
	KASSERT(mips_sdcache_ways < nwaynames);

	switch (cpu_arch) {
#ifdef MIPS1
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
	case CPU_ARCH_MIPS64:
		if (mips_picache_size)
			printf("%s: %dKB/%dB %s L1 Instruction cache, "
			    "%d TLB entries\n", label, mips_picache_size / 1024,
			    mips_picache_line_size, waynames[mips_picache_ways],
			    mips_num_tlb_entries);
		else
			printf("%s: %d TLB entries\n", label, mips_num_tlb_entries);
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
setregs(l, pack, stack)
	struct lwp *l;
	struct exec_package *pack;
	u_long stack;
{
	struct frame *f = (struct frame *)l->l_md.md_regs;

	memset(f, 0, sizeof(struct frame));
	f->f_regs[SP] = (int) stack;
	f->f_regs[PC] = (int) pack->ep_entry & ~3;
	f->f_regs[T9] = (int) pack->ep_entry & ~3; /* abicall requirement */
	f->f_regs[SR] = PSL_USERSET;
	/*
	 * Set up arguments for the rtld-capable crt0:
	 *	a0	stack pointer
	 *	a1	rtld cleanup (filled in by dynamic loader)
	 *	a2	rtld object (filled in by dynamic loader)
	 *	a3	ps_strings
	 */
	f->f_regs[A0] = (int) stack;
	f->f_regs[A1] = 0;
	f->f_regs[A2] = 0;
	f->f_regs[A3] = (int)p->p_psstr;

	if ((l->l_md.md_flags & MDP_FPUSED) && l == fpcurproc)
		fpcurproc = (struct lwp *)0;
	memset(&l->l_addr->u_pcb.pcb_fpregs, 0, sizeof(struct fpreg));
	l->l_md.md_flags &= ~MDP_FPUSED;
	l->l_md.md_ss_addr = 0;
}

#ifdef DEBUG
int sigdebug = 0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send an interrupt to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct sigframe *fp;
	struct frame *f;
	int onstack;
	struct sigcontext ksc;

	f = (struct frame *)l->l_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (p->p_sigctx.ps_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		fp = (struct sigframe *)((caddr_t)p->p_sigctx.ps_sigstk.ss_sp +
						p->p_sigctx.ps_sigstk.ss_size);
	else
		/* cast for _MIPS_BSD_API == _MIPS_BSD_API_LP32_64CLEAN case */
		fp = (struct sigframe *)(u_int32_t)f->f_regs[SP];
	fp--;

#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sendsig(%d): sig %d ssp %p usp %p scp %p\n",
		       p->p_pid, sig, &onstack, fp, &fp->sf_sc);
#endif

	/* Build stack frame for signal trampoline. */
	ksc.sc_pc = f->f_regs[PC];
	ksc.mullo = f->f_regs[MULLO];
	ksc.mulhi = f->f_regs[MULHI];

	/* Save register context. */
	ksc.sc_regs[ZERO] = 0xACEDBADE;		/* magic number */
	memcpy(&ksc.sc_regs[1], &f->f_regs[1],
	    sizeof(ksc.sc_regs) - sizeof(ksc.sc_regs[0]));

	/* Save the floating-pointstate, if necessary, then copy it. */
#ifndef SOFTFLOAT
	ksc.sc_fpused = l->l_md.md_flags & MDP_FPUSED;
	if (ksc.sc_fpused) {
		/* if FPU has current state, save it first */
		if (l == fpcurproc)
			savefpregs(l);
		*(struct fpreg *)ksc.sc_fpregs = l->l_addr->u_pcb.pcb_fpregs;
	}
#else
	*(struct fpreg *)ksc.sc_fpregs = p->p_addr->u_pcb.pcb_fpregs;
#endif

	/* Save signal stack. */
	ksc.sc_onstack = p->p_sigctx.ps_sigstk.ss_flags & SS_ONSTACK;

	/* Save signal mask. */
	ksc.sc_mask = *mask;

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX)
	/*
	 * XXX We always have to save an old style signal mask because
	 * XXX we might be delivering a signal to a process which will
	 * XXX escape from the signal in a non-standard way and invoke
	 * XXX sigreturn() directly.
	 */
	native_sigset_to_sigset13(mask, &ksc.__sc_mask13);
#endif

	if (copyout(&ksc, &fp->sf_sc, sizeof(ksc))) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
#ifdef DEBUG
		if ((sigdebug & SDB_FOLLOW) ||
		    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
			printf("sendsig(%d): copyout failed on sig %d\n",
			    p->p_pid, sig);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* Set up the registers to return to sigcode. */
	f->f_regs[A0] = sig;
	f->f_regs[A1] = code;
	f->f_regs[A2] = (int)&fp->sf_sc;
	f->f_regs[A3] = (int)catcher;

	f->f_regs[PC] = (int)catcher;
	f->f_regs[T9] = (int)catcher;
	f->f_regs[SP] = (int)fp;

	/* Signal trampoline code is at base of user stack. */
	f->f_regs[RA] = (int)p->p_sigctx.ps_sigcode;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;

#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
sys___sigreturn14(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys___sigreturn14_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, ksc;
	struct frame *f;
	struct proc *p = l->l_proc;
	int error;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
#endif
	if ((error = copyin(scp, &ksc, sizeof(ksc))) != 0)
		return (error);

	if ((int) ksc.sc_regs[ZERO] != 0xACEDBADE)	/* magic number */
		return (EINVAL);

	/* Restore the register context. */
	f = (struct frame *)l->l_md.md_regs;
	f->f_regs[PC] = ksc.sc_pc;
	f->f_regs[MULLO] = ksc.mullo;
	f->f_regs[MULHI] = ksc.mulhi;
	memcpy(&f->f_regs[1], &scp->sc_regs[1],
	    sizeof(scp->sc_regs) - sizeof(scp->sc_regs[0]));
#ifndef	SOFTFLOAT
	if (scp->sc_fpused) {
		/* Disable the FPU to fault in FP registers. */
		f->f_regs[SR] &= ~MIPS_SR_COP_1_BIT;
		if (l == fpcurproc) {
			fpcurproc = (struct lwp *)0;
		}
		l->l_addr->u_pcb.pcb_fpregs = *(struct fpreg *)scp->sc_fpregs;
	}
#else
	l->l_addr->u_pcb.pcb_fpregs = *(struct fpreg *)scp->sc_fpregs;
#endif

	/* Restore signal stack. */
	if (ksc.sc_onstack & SS_ONSTACK)
		p->p_sigctx.ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigctx.ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	(void) sigprocmask1(p, SIG_SETMASK, &ksc.sc_mask, 0);

	return (EJUSTRETURN);
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

struct user dumppcb;		/* Actually, struct pcb would do. */

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
	int (*dump)(dev_t, daddr_t, caddr_t, size_t);
	char buf[dbtob(1)];
	kcore_seg_t *segp;
	cpu_kcore_hdr_t *cpuhdrp;
	phys_ram_seg_t *memsegp;
	int i;

	dump = bdevsw[major(dumpdev)].d_dump;

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

	return (dump(dumpdev, dumplo, (caddr_t)buf, dbtob(1)));
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
	int nblks, dumpblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		goto bad;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		goto bad;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
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
#define	BYTES_PER_DUMP	NBPG

void
dumpsys(void)
{
	u_long totalbytesleft, bytes, i, n, memcl;
	u_long maddr;
	int psize;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, caddr_t, size_t);
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	if (dumpdev == NODEV)
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

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
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
	dump = bdevsw[major(dumpdev)].d_dump;
	error = 0;

	for (memcl = 0; memcl < mem_cluster_cnt; memcl++) {
		maddr = mem_clusters[memcl].start;
		bytes = mem_clusters[memcl].size;

		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {
			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0)
				printf("%ld ", totalbytesleft / (1024 * 1024));

			/* Limit size for next transfer. */
			n = bytes - i;
			if (n > BYTES_PER_DUMP)
				n = BYTES_PER_DUMP;

			error = (*dump)(dumpdev, blkno,
			    (caddr_t)MIPS_PHYS_TO_KSEG0(maddr), n);
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

	vps = &vm_physmem[vm_nphysseg - 1];

	/* shrink so that it'll fit in the last segment */
	if ((vps->avail_end - vps->avail_start) < atop(sz))
		sz = ptoa(vps->avail_end - vps->avail_start);

	vps->end -= atop(sz);
	vps->avail_end -= atop(sz);
	msgbufaddr = (caddr_t) MIPS_PHYS_TO_KSEG0(ptoa(vps->end));
	initmsgbuf(msgbufaddr, sz);

	/* Remove the last segment if it now has no pages. */
	if (vps->start == vps->end)
		vm_nphysseg--;

	/* warn if the message buffer had to be shrunk */
	if (sz != reqsz)
		printf("WARNING: %ld bytes not available for msgbuf "
		    "in last cluster (%ld used)\n", reqsz, sz);
}

void
savefpregs(l)
	struct lwp *l;
{
#ifndef NOFPU
	u_int32_t status, fpcsr, *fp;
	struct frame *f;

	if (l == NULL)
		return;
	/*
	 * turnoff interrupts enabling CP1 to read FPCSR register.
	 */
	__asm __volatile (
		".set noreorder		;"
		".set noat		;"
		"mfc0	%0, $12		;"
		"li	$1, %2		;"
		"mtc0	$1, $12		;"
		"nop; nop; nop; nop	;"
		"cfc1	%1, $31		;"
		"cfc1	%1, $31		;"
		".set reorder		;"
		".set at" 
		: "=r" (status), "=r"(fpcsr) : "i"(MIPS_SR_COP_1_BIT));
	/*
	 * this process yielded FPA.
	 */
	f = (struct frame *)l->l_md.md_regs;
	f->f_regs[SR] &= ~MIPS_SR_COP_1_BIT;

	/*
	 * save FPCSR and 32bit FP register values.
	 */
	fp = (int *)l->l_addr->u_pcb.pcb_fpregs.r_regs;
	fp[32] = fpcsr;
	__asm __volatile (
		".set noreorder		;"
		"swc1	$f0, 0(%0)	;"
		"swc1	$f1, 4(%0)	;"
		"swc1	$f2, 8(%0)	;"
		"swc1	$f3, 12(%0)	;"
		"swc1	$f4, 16(%0)	;"
		"swc1	$f5, 20(%0)	;"
		"swc1	$f6, 24(%0)	;"
		"swc1	$f7, 28(%0)	;"
		"swc1	$f8, 32(%0)	;"
		"swc1	$f9, 36(%0)	;"
		"swc1	$f10, 40(%0)	;"
		"swc1	$f11, 44(%0)	;"
		"swc1	$f12, 48(%0)	;"
		"swc1	$f13, 52(%0)	;"
		"swc1	$f14, 56(%0)	;"
		"swc1	$f15, 60(%0)	;"
		"swc1	$f16, 64(%0)	;"
		"swc1	$f17, 68(%0)	;"
		"swc1	$f18, 72(%0)	;"
		"swc1	$f19, 76(%0)	;"
		"swc1	$f20, 80(%0)	;"
		"swc1	$f21, 84(%0)	;"
		"swc1	$f22, 88(%0)	;"
		"swc1	$f23, 92(%0)	;"
		"swc1	$f24, 96(%0)	;"
		"swc1	$f25, 100(%0)	;"
		"swc1	$f26, 104(%0)	;"
		"swc1	$f27, 108(%0)	;"
		"swc1	$f28, 112(%0)	;"
		"swc1	$f29, 116(%0)	;"
		"swc1	$f30, 120(%0)	;"
		"swc1	$f31, 124(%0)	;"
		".set reorder" :: "r"(fp));
	/*
	 * stop CP1, enable interrupts.
	 */
	__asm __volatile ("mtc0 %0, $12" :: "r"(status));
	return;
#endif
}

void
loadfpregs(l)
	struct lwp *l;
{
#ifndef NOFPU
	u_int32_t status, *fp;
	struct frame *f;

	if (l == NULL)
		panic("loading fpregs for NULL proc");

	/*
	 * turnoff interrupts enabling CP1 to load FP registers.
	 */
	__asm __volatile(
		".set noreorder		;"
		".set noat		;"
		"mfc0	%0, $12		;"
		"li	$1, %1		;"
		"mtc0	$1, $12		;"
		"nop; nop; nop; nop	;"
		".set reorder		;"
		".set at" : "=r"(status) : "i"(MIPS_SR_COP_1_BIT));

	f = (struct frame *)l->l_md.md_regs;
	fp = (int *)l->l_addr->u_pcb.pcb_fpregs.r_regs;
	/*
	 * load 32bit FP registers and establish processes' FP context.
	 */
	__asm __volatile(
		".set noreorder		;"
		"lwc1	$f0, 0(%0)	;"
		"lwc1	$f1, 4(%0)	;"
		"lwc1	$f2, 8(%0)	;"
		"lwc1	$f3, 12(%0)	;"
		"lwc1	$f4, 16(%0)	;"
		"lwc1	$f5, 20(%0)	;"
		"lwc1	$f6, 24(%0)	;"
		"lwc1	$f7, 28(%0)	;"
		"lwc1	$f8, 32(%0)	;"
		"lwc1	$f9, 36(%0)	;"
		"lwc1	$f10, 40(%0)	;"
		"lwc1	$f11, 44(%0)	;"
		"lwc1	$f12, 48(%0)	;"
		"lwc1	$f13, 52(%0)	;"
		"lwc1	$f14, 56(%0)	;"
		"lwc1	$f15, 60(%0)	;"
		"lwc1	$f16, 64(%0)	;"
		"lwc1	$f17, 68(%0)	;"
		"lwc1	$f18, 72(%0)	;"
		"lwc1	$f19, 76(%0)	;"
		"lwc1	$f20, 80(%0)	;"
		"lwc1	$f21, 84(%0)	;"
		"lwc1	$f22, 88(%0)	;"
		"lwc1	$f23, 92(%0)	;"
		"lwc1	$f24, 96(%0)	;"
		"lwc1	$f25, 100(%0)	;"
		"lwc1	$f26, 104(%0)	;"
		"lwc1	$f27, 108(%0)	;"
		"lwc1	$f28, 112(%0)	;"
		"lwc1	$f29, 116(%0)	;"
		"lwc1	$f30, 120(%0)	;"
		"lwc1	$f31, 124(%0)	;"
		".set reorder" :: "r"(fp));
	/*
	 * load FPCSR and stop CP1 again while enabling interrupts.
	 */
	__asm __volatile(
		".set noreorder		;"
		".set noat		;"
		"ctc1	%0, $31		;"
		"mtc0	%1, $12		;"
		".set reorder		;"
		".set at"
		:: "r"(fp[32] &~ MIPS_FPU_EXCEPTION_BITS), "r"(status));
	return;
#endif
}

/* 
 * Start a new LWP
 */
void
startlwp(arg)
	void *arg;
{
	int err;
	ucontext_t *uc = arg;
	struct lwp *l = curproc;

	err = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
#if DIAGNOSTIC
	if (err) {
		printf("Error %d from cpu_setmcontext.", err);
	}
#endif
	pool_put(&lwp_uc_pool, uc);

	userret(l);
}

/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	userret(l);
}

void 
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted, void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct proc *p = l->l_proc;
	
	struct saframe *sf, frame;
	struct frame *f;

	extern char sigcode[], upcallcode[];

	f = (struct frame *)l->l_md.md_regs;


#if 0 /* First 4 args in regs (see below). */
	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
#endif
	frame.sa_arg = ap;
	frame.sa_upcall = upcall;

	sf = (struct saframe *)sp - 1;
	if (copyout(&frame, sf, sizeof(frame)) != 0) {
		/* Copying onto the stack didn't work. Die. */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	f->f_regs[PC] = ((u_int32_t)p->p_sigctx.ps_sigcode) +
	    ((u_int32_t)upcallcode - (u_int32_t)sigcode);
	f->f_regs[SP] = (u_int32_t)sf;
	f->f_regs[A0] = type;
	f->f_regs[A1] = (u_int32_t)sas;
	f->f_regs[A2] = nevents;
	f->f_regs[A3] = ninterrupted;
	f->f_regs[S8] = 0;
	f->f_regs[T9] = (u_int32_t)upcall;  /* t9=Upcall function*/

}


void
cpu_getmcontext(l, mcp, flags)
	struct lwp *l;
	mcontext_t *mcp;
	unsigned int *flags;
{
	const struct frame *f = (struct frame *)l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;

	/* Save register context. Dont copy R0 - it is always 0 */
	memcpy(&gr[_REG_AT], &f->f_regs[AST], sizeof(mips_reg_t) * 31);

	gr[_REG_MDLO]  = f->f_regs[MULLO];
	gr[_REG_MDHI]  = f->f_regs[MULHI];
	gr[_REG_CAUSE] = f->f_regs[CAUSE];
	gr[_REG_EPC]   = f->f_regs[PC];

	*flags |= _UC_CPU;

	/* Save floating point register context, if any. */
	if (l->l_md.md_flags & MDP_FPUSED) {
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 * XXX:  FP regs may be written to wrong location XXX
		 */
		if (l == fpcurproc)
			savefpregs(l);
		memcpy(&mcp->__fpregs, &l->l_addr->u_pcb.pcb_fpregs.r_regs,
		    sizeof (mcp->__fpregs));
		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(l, mcp, flags)
	struct lwp *l;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct frame *f = (struct frame *)l->l_md.md_regs;
	__greg_t *gr = mcp->__gregs;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		/* Save register context. */
		/* XXX:  Do we validate the addresses?? */
		memcpy(&f->f_regs[AST], &gr[_REG_AT], sizeof(mips_reg_t) * 31);

		f->f_regs[MULLO] = gr[_REG_MDLO];
		f->f_regs[MULHI] = gr[_REG_MDHI];
		f->f_regs[CAUSE] = gr[_REG_CAUSE];
		f->f_regs[PC] = gr[_REG_EPC];
	}

	/* Restore floating point register context, if any. */
	if (flags & _UC_FPU) {
		/* XXX:  FP regs may be read from wrong location XXX */
		memcpy(&l->l_addr->u_pcb.pcb_fpregs.r_regs, &mcp->__fpregs,
		    sizeof (mcp->__fpregs));
		/* XXX:  Do we restore here?? */
	}

	return (0);
}
