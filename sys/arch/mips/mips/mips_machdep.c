/*	$NetBSD: mips_machdep.c,v 1.262.2.3 2016/10/05 20:55:32 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mips_machdep.c,v 1.262.2.3 2016/10/05 20:55:32 skrll Exp $");

#define __INTR_PRIVATE
#include "opt_cputype.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/intr.h>
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
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/ucontext.h>
#include <sys/bitops.h>

#include <mips/kcore.h>

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32.h>
#endif

#include <uvm/uvm.h>

#include <dev/cons.h>
#include <dev/mm.h>

#include <mips/pcb.h>
#include <mips/cache.h>
#include <mips/frame.h>
#include <mips/regnum.h>
#include <mips/mips_opcode.h>

#include <mips/cpu.h>
#include <mips/locore.h>
#include <mips/psl.h>
#include <mips/pte.h>
#include <mips/userret.h>

#ifdef __HAVE_BOOTINFO_H
#include <machine/bootinfo.h>
#endif

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
#include <mips/mipsNN.h>		/* MIPS32/MIPS64 registers */

#define	_MKINSN(a,b,c,d,e) ((uint32_t)(((a) << 26)|((b) << 21)|((c) << 16)|((d) << 11)|(e)))

#ifdef _LP64
#define	_LOAD_V0_L_PRIVATE_A0	_MKINSN(OP_LD, _R_A0, _R_V0, 0, offsetof(lwp_t, l_private))
#define	_MTC0_V0_USERLOCAL	_MKINSN(OP_COP0, OP_DMT, _R_V0, MIPS_COP_0_TLB_CONTEXT, 2)
#else
#define	_LOAD_V0_L_PRIVATE_A0	_MKINSN(OP_LW, _R_A0, _R_V0, 0, offsetof(lwp_t, l_private))
#define	_MTC0_V0_USERLOCAL	_MKINSN(OP_COP0, OP_MT, _R_V0, MIPS_COP_0_TLB_CONTEXT, 2)
#endif
#define	JR_RA			_MKINSN(OP_SPECIAL, _R_RA, 0, 0, OP_JR)

#endif

/* Internal routines. */
int	cpu_dumpsize(void);
u_long	cpu_dump_mempagecnt(void);
int	cpu_dump(void);

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
static void mips_watchpoint_init(void);
#endif

#if defined(_LP64) && defined(ENABLE_MIPS_16KB_PAGE)
vaddr_t mips_vm_maxuser_address = MIPS_VM_MAXUSER_ADDRESS;
#endif

#if defined(MIPS3_PLUS)
uint32_t mips3_cp0_tlb_page_mask_probe(void);
uint64_t mips3_cp0_tlb_entry_hi_probe(void);
uint64_t mips3_cp0_tlb_entry_lo_probe(void);

static void mips3_tlb_probe(void);
#endif

#if defined(MIPS1)
static void	mips1_vector_init(const struct splsw *);
extern const struct locoresw mips1_locoresw;
extern const mips_locore_jumpvec_t mips1_locore_vec;
#endif

#if defined(MIPS3)
static void	mips3_vector_init(const struct splsw *);
extern const struct locoresw mips3_locoresw;
extern const mips_locore_jumpvec_t mips3_locore_vec;
#endif

#if defined(MIPS3_LOONGSON2)
static void	loongson2_vector_init(const struct splsw *);
extern const struct locoresw loongson2_locoresw;
extern const mips_locore_jumpvec_t loongson2_locore_vec;
#endif

#if defined(MIPS32)
static void	mips32_vector_init(const struct splsw *);
extern const struct locoresw mips32_locoresw;
extern const mips_locore_jumpvec_t mips32_locore_vec;
#endif

#if defined(MIPS32R2)
static void	mips32r2_vector_init(const struct splsw *);
extern const struct locoresw mips32r2_locoresw;
extern const mips_locore_jumpvec_t mips32r2_locore_vec;
#endif

#if defined(MIPS64)
static void	mips64_vector_init(const struct splsw *);
extern const struct locoresw mips64_locoresw;
extern const mips_locore_jumpvec_t mips64_locore_vec;
#endif

#if defined(MIPS64R2)
extern const struct locoresw mips64r2_locoresw;
extern const mips_locore_jumpvec_t mips64r2_locore_vec;
#endif

#if defined(PARANOIA)
void std_splsw_test(void);
#endif

mips_locore_jumpvec_t mips_locore_jumpvec;

struct locoresw mips_locoresw;

extern const struct splsw std_splsw;
struct splsw mips_splsw;

struct mips_options mips_options = {
	.mips_cpu_id = 0xffffffff,
	.mips_fpu_id = 0xffffffff,
};

void *	msgbufaddr;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */


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
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT | CPU_MIPS_LOONGSON2
	  | MIPS_LOONGSON2_CCA, 0, 0, "ICT Loongson 2E CPU"	},
	{ 0, MIPS_LOONGSON2, MIPS_REV_LOONGSON2F, -1, CPU_ARCH_MIPS3, 64,
	  CPU_MIPS_R4K_MMU | CPU_MIPS_DOUBLE_COUNT | CPU_MIPS_LOONGSON2
	  | MIPS_LOONGSON2_CCA, 0, 0, "ICT Loongson 2F CPU"	},

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
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_USERLOCAL | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 |
	  MIPS_CP0FL_CONFIG3 | MIPS_CP0FL_CONFIG7,
	  0, "24K" },
	{ MIPS_PRID_CID_MTI, MIPS_24KE, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_USERLOCAL | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 |
	  MIPS_CP0FL_CONFIG3 | MIPS_CP0FL_CONFIG7,
	  0, "24KE" },
	{ MIPS_PRID_CID_MTI, MIPS_34K, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_USERLOCAL | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 |
	  MIPS_CP0FL_CONFIG3 | MIPS_CP0FL_CONFIG7,
	  0, "34K" },
	{ MIPS_PRID_CID_MTI, MIPS_74K, -1, -1,	-1, 0,
	  CPU_MIPS_HAVE_SPECIAL_CCA | (0 << CPU_MIPS_CACHED_CCA_SHIFT) |
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_USERLOCAL | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 |
	  MIPS_CP0FL_CONFIG3 | MIPS_CP0FL_CONFIG6 | MIPS_CP0FL_CONFIG7,
	  0, "74K" },
	{ MIPS_PRID_CID_MTI, MIPS_1004K, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_USERLOCAL | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 |
	  MIPS_CP0FL_CONFIG3 | MIPS_CP0FL_CONFIG6 | MIPS_CP0FL_CONFIG7,
	  0, "1004K" },
	{ MIPS_PRID_CID_MTI, MIPS_1074K, -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_USERLOCAL | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 |
	  MIPS_CP0FL_CONFIG3 | MIPS_CP0FL_CONFIG6 | MIPS_CP0FL_CONFIG7,
	  0, "1074K" },

	{ MIPS_PRID_CID_BROADCOM, MIPS_BCM3302, -1, -1, -1, 0,
	  MIPS32_FLAGS | CPU_MIPS_DOUBLE_COUNT, 0, 0, "BCM3302"	},

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

	{ MIPS_PRID_CID_RMI, MIPS_XLR732B, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLR|MIPS_CIDFL_RMI_CPUS(8,4)|MIPS_CIDFL_RMI_L2(2MB),
	  "XLR732B"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLR732C, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLR|MIPS_CIDFL_RMI_CPUS(8,4)|MIPS_CIDFL_RMI_L2(2MB),
	  "XLR732C"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS616, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(4,4)|MIPS_CIDFL_RMI_L2(1MB),
	  "XLS616"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS416, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(4,4)|MIPS_CIDFL_RMI_L2(1MB),
	  "XLS416"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS408, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(2,4)|MIPS_CIDFL_RMI_L2(1MB),
	  "XLS408"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS408LITE, -1, -1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(2,4)|MIPS_CIDFL_RMI_L2(1MB),
	  "XLS408lite"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS404LITE, -1, -1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(1,4)|MIPS_CIDFL_RMI_L2(512KB),
	  "XLS404lite"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS208, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(2,4)|MIPS_CIDFL_RMI_L2(512KB),
	  "XLS208"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS204, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(1,4)|MIPS_CIDFL_RMI_L2(256KB),
	  "XLS204"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS108, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(2,4)|MIPS_CIDFL_RMI_L2(512KB),
	  "XLS108"		},

	{ MIPS_PRID_CID_RMI, MIPS_XLS104, -1,	-1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR |
	  CPU_MIPS_I_D_CACHE_COHERENT | CPU_MIPS_HAVE_MxCR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EIRR | MIPS_CP0FL_EIMR | MIPS_CP0FL_EBASE |
	  MIPS_CP0FL_CONFIG | MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG7,
	  CIDFL_RMI_TYPE_XLS|MIPS_CIDFL_RMI_CPUS(1,4)|MIPS_CIDFL_RMI_L2(256KB),
	  "XLS104"		},

	{ MIPS_PRID_CID_CAVIUM, MIPS_CN31XX, -1, -1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_CONFIG |
	  MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 | MIPS_CP0FL_CONFIG3,
	  0,
	  "CN31xx"		},

	{ MIPS_PRID_CID_CAVIUM, MIPS_CN30XX, -1, -1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_CONFIG |
	  MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 | MIPS_CP0FL_CONFIG3,
	  0,
	  "CN30xx"		},

	{ MIPS_PRID_CID_CAVIUM, MIPS_CN50XX, -1, -1, -1, 0,
	  MIPS64_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_NO_LLADDR,
	  MIPS_CP0FL_USE |
	  MIPS_CP0FL_EBASE | MIPS_CP0FL_CONFIG | MIPS_CP0FL_HWRENA |
	  MIPS_CP0FL_CONFIG1 | MIPS_CP0FL_CONFIG2 | MIPS_CP0FL_CONFIG3,
	  0,
	  "CN50xx"		},

	/* Microsoft Research' extensible MIPS */
	{ MIPS_PRID_CID_MICROSOFT, MIPS_eMIPS, 1, -1, CPU_ARCH_MIPS1, 64,
	  CPU_MIPS_NO_WAIT, 0, 0,		"eMIPS CPU"		},

	/* Ingenic XBurst */
	{ MIPS_PRID_CID_INGENIC, MIPS_XBURST,  -1, -1,	-1, 0,
	  MIPS32_FLAGS | CPU_MIPS_D_CACHE_COHERENT | CPU_MIPS_DOUBLE_COUNT,
	  0, 0, "XBurst"		},

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
	"Cavium",
};
#define	ncidnames __arraycount(cidnames)

#if defined(MIPS1)
/*
 * MIPS-I locore function vector
 */

static void
mips1_vector_init(const struct splsw *splsw)
{
	extern char mips1_utlb_miss[], mips1_utlb_miss_end[];
	extern char mips1_exception[], mips1_exception_end[];

	/*
	 * Copy down exception vector code.
	 */
	if (mips1_utlb_miss_end - mips1_utlb_miss > 0x80)
		panic("startup: UTLB vector code too large");
	if (mips1_exception_end - mips1_exception > 0x80)
		panic("startup: general exception vector code too large");
	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips1_utlb_miss,
		mips1_exception_end - mips1_utlb_miss);

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
static void
mips3_vector_init(const struct splsw *splsw)
{
	/* r4000 exception handler address and end */
	extern char mips3_exception[], mips3_exception_end[];

	/* TLB miss handler address and end */
	extern char mips3_tlb_miss[];
	extern char mips3_xtlb_miss[];

	/* Cache error handler */
	extern char mips3_cache[];
	/*
	 * Copy down exception vector code.
	 */

	if (mips3_xtlb_miss - mips3_tlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "UTLB");
	if (mips3_cache - mips3_xtlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "XTLB");
	if (mips3_exception - mips3_cache != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "Cache error");
	if (mips3_exception_end - mips3_exception > 0x80)
		panic("startup: %s vector code too large",
		    "General exception");

	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips3_tlb_miss,
	      mips3_exception_end - mips3_tlb_miss);

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips3_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);
}
#endif /* MIPS3 */

#if defined(MIPS3_LOONGSON2)
static void
loongson2_vector_init(const struct splsw *splsw)
{
	/* r4000 exception handler address and end */
	extern char loongson2_exception[], loongson2_exception_end[];

	/* TLB miss handler address and end */
	extern char loongson2_tlb_miss[];
	extern char loongson2_xtlb_miss[];

	/* Cache error handler */
	extern char loongson2_cache[];

	/*
	 * Copy down exception vector code.
	 */

	if (loongson2_xtlb_miss - loongson2_tlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "UTLB");
	if (loongson2_cache - loongson2_xtlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "XTLB");
	if (loongson2_exception - loongson2_cache != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "Cache error");
	if (loongson2_exception_end - loongson2_exception > 0x80)
		panic("startup: %s vector code too large",
		    "General exception");

	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, loongson2_tlb_miss,
	      loongson2_exception_end - loongson2_tlb_miss);

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = loongson2_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);
}
#endif /* MIPS3_LOONGSON2 */

#if defined(MIPS32)
static void
mips32_vector_init(const struct splsw *splsw)
{
	/* r4000 exception handler address */
	extern char mips32_exception[];

	/* TLB miss handler addresses */
	extern char mips32_tlb_miss[];

	/* Cache error handler */
	extern char mips32_cache[];

	/* MIPS32 interrupt exception handler */
	extern char mips32_intr[], mips32_intr_end[];

	/*
	 * Copy down exception vector code.
	 */

	if (mips32_cache - mips32_tlb_miss != 0x100)
		panic("startup: %s vector code not 128 bytes in length",
		    "UTLB");
	if (mips32_exception - mips32_cache != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "Cache error");
	if (mips32_intr - mips32_exception != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "General exception");
	if (mips32_intr_end - mips32_intr > 0x80)
		panic("startup: %s vector code too large",
		    "interrupt exception");

	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips32_tlb_miss,
	      mips32_intr_end - mips32_tlb_miss);

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips32_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);

	mips_watchpoint_init();
}
#endif /* MIPS32 */

#if defined(MIPS32R2)
static void
mips32r2_vector_init(const struct splsw *splsw)
{
	/* r4000 exception handler address */
	extern char mips32r2_exception[];

	/* TLB miss handler addresses */
	extern char mips32r2_tlb_miss[];

	/* Cache error handler */
	extern char mips32r2_cache[];

	/* MIPS32 interrupt exception handler */
	extern char mips32r2_intr[], mips32r2_intr_end[];

	/*
	 * Copy down exception vector code.
	 */
	if (mips32r2_cache - mips32r2_tlb_miss != 0x100)
		panic("startup: %s vector code not 128 bytes in length",
		    "UTLB");
	if (mips32r2_exception - mips32r2_cache != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "Cache error");
	if (mips32r2_intr - mips32r2_exception != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "General exception");
	if (mips32r2_intr_end - mips32r2_intr > 0x80)
		panic("startup: %s vector code too large",
		    "interrupt exception");

	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips32r2_tlb_miss,
	      mips32r2_intr_end - mips32r2_tlb_miss);

	/*
	 * Let see if this cpu has DSP V2 ASE...
	 */
	uint32_t cp0flags = mips_options.mips_cpu->cpu_cp0flags;
	if (mipsNN_cp0_config2_read() & MIPSNN_CFG2_M) {
		const uint32_t cfg3 = mipsNN_cp0_config3_read();
		if (cfg3 & MIPSNN_CFG3_ULRP) {
			cp0flags |= MIPS_CP0FL_USERLOCAL;
		}
		if (cfg3 & MIPSNN_CFG3_DSP2P) {
			mips_options.mips_cpu_flags |= CPU_MIPS_HAVE_DSP;
		}
	}
	/*
	 * If this CPU doesn't have a COP0 USERLOCAL register, at the end
	 * of cpu_switch resume overwrite the instructions which update it.
	 */
	if (!(cp0flags & MIPS_CP0FL_USERLOCAL)) {
		extern uint32_t mips32r2_cpu_switch_resume[];
		for (uint32_t *insnp = mips32r2_cpu_switch_resume;; insnp++) {
			KASSERT(insnp[0] != JR_RA);
			if (insnp[0] == _LOAD_V0_L_PRIVATE_A0
			    && insnp[1] == _MTC0_V0_USERLOCAL) {
				insnp[0] = JR_RA;
				insnp[1] = 0;		/* NOP */
				break;
			}
		}
	}

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips32r2_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);

	mips_watchpoint_init();
}
#endif /* MIPS32R2 */

#if defined(MIPS64)
static void
mips64_vector_init(const struct splsw *splsw)
{
	/* r4000 exception handler address */
	extern char mips64_exception[];

	/* TLB miss handler addresses */
	extern char mips64_tlb_miss[];
	extern char mips64_xtlb_miss[];

	/* Cache error handler */
	extern char mips64_cache[];

	/* MIPS64 interrupt exception handler */
	extern char mips64_intr[], mips64_intr_end[];

	/*
	 * Copy down exception vector code.
	 */

	if (mips64_xtlb_miss - mips64_tlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "UTLB");
	if (mips64_cache - mips64_xtlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "XTLB");
	if (mips64_exception - mips64_cache != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "Cache error");
	if (mips64_intr - mips64_exception != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "General exception");
	if (mips64_intr_end - mips64_intr > 0x80)
		panic("startup: %s vector code too large",
		    "interrupt exception");

	memcpy((void *)MIPS_UTLB_MISS_EXC_VEC, mips64_tlb_miss,
	      mips64_intr_end - mips64_tlb_miss);

	/*
	 * Copy locore-function vector.
	 */
	mips_locore_jumpvec = mips64_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);

	mips_watchpoint_init();
}
#endif /* MIPS64 */

#if defined(MIPS64R2)
void
mips64r2_vector_init(const struct splsw *splsw)
{
	/* r4000 exception handler address */
	extern char mips64r2_exception[];

	/* TLB miss handler addresses */
	extern char mips64r2_tlb_miss[];
	extern char mips64r2_xtlb_miss[];

	/* Cache error handler */
	extern char mips64r2_cache[];

	/* MIPS64 interrupt exception handler */
	extern char mips64r2_intr[], mips64r2_intr_end[];

	/*
	 * Copy down exception vector code.
	 */

	if (mips64r2_xtlb_miss - mips64r2_tlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "UTLB");
	if (mips64r2_cache - mips64r2_xtlb_miss != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "XTLB");
	if (mips64r2_exception - mips64r2_cache != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "Cache error");
	if (mips64r2_intr - mips64r2_exception != 0x80)
		panic("startup: %s vector code not 128 bytes in length",
		    "General exception");
	if (mips64r2_intr_end - mips64r2_intr > 0x80)
		panic("startup: %s vector code too large",
		    "interrupt exception");

	const intptr_t ebase = (intptr_t)mipsNN_cp0_ebase_read();
	const int cpunum = ebase & MIPS_EBASE_CPUNUM;

	// This may need to be on CPUs other CPU0 so use EBASE to fetch
	// the appropriate address for exception code.  EBASE also contains
	// the cpunum so remove that.
	memcpy((void *)(intptr_t)(ebase & ~MIPS_EBASE_CPUNUM), mips64r2_tlb_miss,
	      mips64r2_intr_end - mips64r2_tlb_miss);

	/*
	 * Let see if this cpu has DSP V2 ASE...
	 */
	uint32_t cp0flags = mips_options.mips_cpu->cpu_cp0flags;
	if (mipsNN_cp0_config2_read() & MIPSNN_CFG2_M) {
		const uint32_t cfg3 = mipsNN_cp0_config3_read();
		if (cfg3 & MIPSNN_CFG3_ULRP) {
			cp0flags |= MIPS_CP0FL_USERLOCAL;
		}
		if (cfg3 & MIPSNN_CFG3_DSP2P) {
			mips_options.mips_cpu_flags |= CPU_MIPS_HAVE_DSP;
		}
	}

	/*
	 * If this CPU doesn't have a COP0 USERLOCAL register, at the end
	 * of cpu_switch resume overwrite the instructions which update it.
	 */
	if (!(cp0flags & MIPS_CP0FL_USERLOCAL) && cpunum == 0) {
		extern uint32_t mips64r2_cpu_switch_resume[];
		for (uint32_t *insnp = mips64r2_cpu_switch_resume;; insnp++) {
			KASSERT(insnp[0] != JR_RA);
			if (insnp[0] == _LOAD_V0_L_PRIVATE_A0
			    && insnp[1] == _MTC0_V0_USERLOCAL) {
				insnp[0] = JR_RA;
				insnp[1] = 0;		/* NOP */
				break;
			}
		}
	}

	/*
	 * Copy locore-function vector.
	 */
	if (cpunum == 0)
		mips_locore_jumpvec = mips64r2_locore_vec;

	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Clear BEV in SR so we start handling our own exceptions */
	mips_cp0_status_write(mips_cp0_status_read() & ~MIPS_SR_BEV);

	mips_watchpoint_init();
}
#endif /* MIPS64R2 */

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
mips_vector_init(const struct splsw *splsw, bool multicpu_p)
{
	struct mips_options * const opts = &mips_options;
	const struct pridtab *ct;
	const mips_prid_t cpu_id = opts->mips_cpu_id;

	for (ct = cputab; ct->cpu_name != NULL; ct++) {
		if (MIPS_PRID_CID(cpu_id) != ct->cpu_cid ||
		    MIPS_PRID_IMPL(cpu_id) != ct->cpu_pid)
			continue;
		if (ct->cpu_rev >= 0 &&
		    MIPS_PRID_REV(cpu_id) != ct->cpu_rev)
			continue;
		if (ct->cpu_copts >= 0 &&
		    MIPS_PRID_COPTS(cpu_id) != ct->cpu_copts)
			continue;

		opts->mips_cpu = ct;
		opts->mips_cpu_arch = ct->cpu_isa;
		opts->mips_num_tlb_entries = ct->cpu_ntlb;
		break;
	}

	if (opts->mips_cpu == NULL)
		panic("CPU type (0x%x) not supported", cpu_id);

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	if (MIPS_PRID_CID(cpu_id) != 0) {
		/* MIPS32/MIPS64, use coprocessor 0 config registers */
		uint32_t cfg, cfg1;

		cfg = mips3_cp0_config_read();
		cfg1 = mipsNN_cp0_config1_read();

		/* pick CPU type */
		switch (MIPSNN_GET(CFG_AT, cfg)) {
		case MIPSNN_CFG_AT_MIPS32:
			opts->mips_cpu_arch = CPU_ARCH_MIPS32;
			break;
		case MIPSNN_CFG_AT_MIPS64:
			opts->mips_cpu_arch = CPU_ARCH_MIPS64;
			break;
		case MIPSNN_CFG_AT_MIPS64S:
		default:
			panic("MIPS32/64 architecture type %d not supported",
			    MIPSNN_GET(CFG_AT, cfg));
		}

		switch (MIPSNN_GET(CFG_AR, cfg)) {
		case MIPSNN_CFG_AR_REV1:
			break;
		case MIPSNN_CFG_AR_REV2:
			switch (opts->mips_cpu_arch) {
			case CPU_ARCH_MIPS32:
				opts->mips_cpu_arch = CPU_ARCH_MIPS32R2;
				break;
			case CPU_ARCH_MIPS64:
				opts->mips_cpu_arch = CPU_ARCH_MIPS64R2;
				break;
			default:
				printf("WARNING: MIPS32/64 arch %d revision %d "
				    "unknown!\n", opts->mips_cpu_arch,
				    MIPSNN_GET(CFG_AR, cfg));
				break;
			}
			break;
		default:
			printf("WARNING: MIPS32/64 arch revision %d "
			    "unknown!\n", MIPSNN_GET(CFG_AR, cfg));
			break;
		}

		/* figure out MMU type (and number of TLB entries) */
		switch (MIPSNN_GET(CFG_MT, cfg)) {
		case MIPSNN_CFG_MT_TLB:
			opts->mips_num_tlb_entries = MIPSNN_CFG1_MS(cfg1);
			break;
		case MIPSNN_CFG_MT_NONE:
		case MIPSNN_CFG_MT_BAT:
		case MIPSNN_CFG_MT_FIXED:
		default:
			panic("MIPS32/64 MMU type %d not supported",
			    MIPSNN_GET(CFG_MT, cfg));
		}
	}
#endif /* (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */

	if (opts->mips_cpu_arch < 1)
		panic("Unknown CPU ISA for CPU type 0x%x", cpu_id);
	if (opts->mips_num_tlb_entries < 1)
		panic("Unknown number of TLBs for CPU type 0x%x", cpu_id);

	/*
	 * Check CPU-specific flags.
	 */
	opts->mips_cpu_flags = opts->mips_cpu->cpu_flags;
	opts->mips_has_r4k_mmu = (opts->mips_cpu_flags & CPU_MIPS_R4K_MMU) != 0;
	opts->mips_has_llsc = (opts->mips_cpu_flags & CPU_MIPS_NO_LLSC) == 0;
#if defined(MIPS3_4100)
	if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4100)
		opts->mips3_pg_shift = MIPS3_4100_PG_SHIFT;
	else
#endif
		opts->mips3_pg_shift = MIPS3_DEFAULT_PG_SHIFT;

	opts->mips3_cca_devmem = CCA_UNCACHED;
	if (opts->mips_cpu_flags & CPU_MIPS_HAVE_SPECIAL_CCA) {
		uint32_t cca;

		cca = (opts->mips_cpu_flags & CPU_MIPS_CACHED_CCA_MASK) >>
		    CPU_MIPS_CACHED_CCA_SHIFT;
		opts->mips3_pg_cached = MIPS3_CCA_TO_PG(cca);
#ifndef __mips_o32
		opts->mips3_xkphys_cached = MIPS_PHYS_TO_XKPHYS(cca, 0);
#endif
	} else {
		opts->mips3_pg_cached = MIPS3_DEFAULT_PG_CACHED;
#ifndef __mips_o32
		opts->mips3_xkphys_cached = MIPS3_DEFAULT_XKPHYS_CACHED;
#endif
	}

#ifdef __HAVE_MIPS_MACHDEP_CACHE_CONFIG
	mips_machdep_cache_config();
#endif

	/*
	 * if 'splsw' is NULL, use standard SPL with COP0 status/cause
	 * otherwise use chip-specific splsw
	 */
	if (splsw == NULL) {
		mips_splsw = std_splsw;
#ifdef PARANOIA
		std_splsw_test();	/* only works with std_splsw */
#endif
	} else {
		mips_splsw = *splsw;
	}

	/*
	 * Determine cache configuration and initialize our cache
	 * frobbing routine function pointers.
	 */
	mips_config_cache();

	/*
	 * We default to RAS atomic ops since they are the lowest overhead.
	 */
#ifdef MULTIPROCESSOR
	if (multicpu_p) {
		/*
		 * If we could have multiple CPUs active,
		 * use the ll/sc variants.
		 */
		mips_locore_atomicvec = mips_llsc_locore_atomicvec;
	}
#endif
	/*
	 * Now initialize our ISA-dependent function vector.
	 */
	switch (opts->mips_cpu_arch) {
#if defined(MIPS1)
	case CPU_ARCH_MIPS1:
		(*mips1_locore_vec.ljv_tlb_invalidate_all)();
		mips1_vector_init(splsw);
		mips_locoresw = mips1_locoresw;
		break;
#endif
#if defined(MIPS3)
	case CPU_ARCH_MIPS3:
	case CPU_ARCH_MIPS4:
		mips3_tlb_probe();
#if defined(MIPS3_4100)
		if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4100)
			mips3_cp0_pg_mask_write(MIPS4100_PG_SIZE_TO_MASK(PAGE_SIZE));
		else
#endif
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
#if defined(MIPS3_LOONGSON2)
		if (opts->mips_cpu_flags & CPU_MIPS_LOONGSON2) {
			(*loongson2_locore_vec.ljv_tlb_invalidate_all)();
			mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);
			loongson2_vector_init(splsw);
			mips_locoresw = loongson2_locoresw;
			opts->mips3_cca_devmem = CCA_ACCEL;
			break;
		}
#endif /* MIPS3_LOONGSON2 */
		(*mips3_locore_vec.ljv_tlb_invalidate_all)();
		mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);
		mips3_vector_init(splsw);
		mips_locoresw = mips3_locoresw;
		break;

#endif /* MIPS3 */
#if defined(MIPS32)
	case CPU_ARCH_MIPS32:
		mips3_tlb_probe();
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		(*mips32_locore_vec.ljv_tlb_invalidate_all)();
		mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);
		mips32_vector_init(splsw);
		mips_locoresw = mips32_locoresw;
		break;
#endif
#if defined(MIPS32R2)
	case CPU_ARCH_MIPS32R2:
		mips3_tlb_probe();
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		(*mips32r2_locore_vec.ljv_tlb_invalidate_all)();
		mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);
		mips32r2_vector_init(splsw);
		mips_locoresw = mips32r2_locoresw;
		break;
#endif
#if defined(MIPS64)
	case CPU_ARCH_MIPS64: {
		mips3_tlb_probe();
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		(*mips64_locore_vec.ljv_tlb_invalidate_all)();
		mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);
		mips64_vector_init(splsw);
		mips_locoresw = mips64_locoresw;
		break;
	}
#endif
#if defined(MIPS64R2)
	case CPU_ARCH_MIPS64R2: {
		mips3_tlb_probe();
		mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
		mips3_cp0_wired_write(0);
		(*mips64r2_locore_vec.ljv_tlb_invalidate_all)();
		mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);
		mips64r2_vector_init(splsw);
		mips_locoresw = mips64r2_locoresw;
		break;
	}
#endif
	default:
		printf("cpu_arch 0x%x: not supported\n", opts->mips_cpu_arch);
		cpu_reboot(RB_HALT, NULL);
	}

	/*
	 * Now that the splsw and locoresw have been filled in, fixup the
	 * jumps to any stubs to actually jump to the real routines.
	 */
	extern uint32_t _ftext[];
	extern uint32_t _etext[];
	mips_fixup_stubs(_ftext, _etext);

#if (MIPS3 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	/*
	 * Install power-saving idle routines.
	 */
	if ((opts->mips_cpu_flags & CPU_MIPS_USE_WAIT) &&
	    !(opts->mips_cpu_flags & CPU_MIPS_NO_WAIT))
		mips_locoresw.lsw_cpu_idle = mips_wait_idle;
#endif /* (MIPS3 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */
}

void
mips_set_wbflush(void (*flush_fn)(void))
{
	mips_locoresw.lsw_wbflush = flush_fn;
	(*flush_fn)();
}

#if defined(MIPS3_PLUS)
static void
mips3_tlb_probe(void)
{
	struct mips_options * const opts = &mips_options;
	opts->mips3_tlb_pg_mask = mips3_cp0_tlb_page_mask_probe();
	if (CPUIS64BITS) {
		opts->mips3_tlb_vpn_mask = mips3_cp0_tlb_entry_hi_probe();
		opts->mips3_tlb_vpn_mask |= PAGE_MASK;
		opts->mips3_tlb_vpn_mask <<= 2;
		opts->mips3_tlb_vpn_mask >>= 2;
		opts->mips3_tlb_pfn_mask = mips3_cp0_tlb_entry_lo_probe();
#if defined(_LP64) && defined(ENABLE_MIPS_16KB_PAGE)
		/*
		 * 16KB pages could cause our page table being able to address
		 * a larger address space than the actual chip supports.  So
		 * we need to limit the address space to what it can really
		 * address.
		 */
		if (mips_vm_maxuser_address > opts->mips3_tlb_vpn_mask + 1)
			mips_vm_maxuser_address = opts->mips3_tlb_vpn_mask + 1;
#endif
	}
}
#endif

/*
 * Identify product revision IDs of CPU and FPU.
 */
void
cpu_identify(device_t dev)
{
	const struct mips_options * const opts = &mips_options;
	const struct mips_cache_info * const mci = &mips_cache_info;
	const mips_prid_t cpu_id = opts->mips_cpu_id;
	const mips_prid_t fpu_id = opts->mips_fpu_id;
	static const char * const waynames[] = {
		[0] = "fully set-associative",
		[1] = "direct-mapped",
		[2] = "2-way set-associative",
		[3] = NULL,
		[4] = "4-way set-associative",
		[5] = "5-way set-associative",
		[6] = "6-way set-associative",
		[7] = "7-way set-associative",
		[8] = "8-way set-associative",
#ifdef MIPS64_OCTEON
		[64] = "64-way set-associative",
#endif
	};
#define	nwaynames (sizeof(waynames) / sizeof(waynames[0]))
	static const char * const wtnames[] = {
		"write-back",
		"write-through",
	};
	const char *cpuname, *fpuname;
	int i;

	cpuname = opts->mips_cpu->cpu_name;

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

	if (opts->mips_cpu->cpu_cid != 0) {
		if (opts->mips_cpu->cpu_cid <= ncidnames)
			aprint_normal("%s ", cidnames[opts->mips_cpu->cpu_cid]);
		else if (opts->mips_cpu->cpu_cid == MIPS_PRID_CID_INGENIC) {
			aprint_normal("Ingenic ");
		} else {
			aprint_normal("Unknown Company ID - 0x%x", opts->mips_cpu->cpu_cid);
			aprint_normal_dev(dev, "");
		}
	}
	if (cpuname != NULL)
		aprint_normal("%s (0x%x)", cpuname, cpu_id);
	else
		aprint_normal("unknown CPU type (0x%x)", cpu_id);
	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC)
		aprint_normal(" Rev. %d.%d", MIPS_PRID_REV_MAJ(cpu_id),
		    MIPS_PRID_REV_MIN(cpu_id));
	else
		aprint_normal(" Rev. %d", MIPS_PRID_REV(cpu_id));

	if (fpuname != NULL)
		aprint_normal(" with %s", fpuname);
	else
		aprint_normal(" with unknown FPC type (0x%x)", fpu_id);
	if (opts->mips_fpu_id != 0) {
		if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC)
			aprint_normal(" Rev. %d.%d", MIPS_PRID_REV_MAJ(fpu_id),
			    MIPS_PRID_REV_MIN(fpu_id));
		else
			aprint_normal(" Rev. %d", MIPS_PRID_REV(fpu_id));
	}
	if (opts->mips_cpu_flags & MIPS_HAS_DSP) {
		aprint_normal(" and DSPv2");
	}
	aprint_normal("\n");

	if (MIPS_PRID_CID(cpu_id) == MIPS_PRID_CID_PREHISTORIC &&
	    MIPS_PRID_RSVD(cpu_id) != 0) {
		aprint_normal_dev(dev, "NOTE: top 8 bits of prehistoric PRID not 0!\n");
		aprint_normal_dev(dev, "Please mail port-mips@NetBSD.org with %s "
		    "dmesg lines.\n", device_xname(dev));
	}

	KASSERT(mci->mci_picache_ways < nwaynames);
	KASSERT(mci->mci_pdcache_ways < nwaynames);
	KASSERT(mci->mci_sicache_ways < nwaynames);
	KASSERT(mci->mci_sdcache_ways < nwaynames);

	switch (opts->mips_cpu_arch) {
#if defined(MIPS1)
	case CPU_ARCH_MIPS1:
		if (mci->mci_picache_size)
			aprint_normal_dev(dev, "%dKB/%dB %s Instruction cache, "
			    "%d TLB entries\n", mci->mci_picache_size / 1024,
			    mci->mci_picache_line_size, waynames[mci->mci_picache_ways],
			    opts->mips_num_tlb_entries);
		else
			aprint_normal_dev(dev, "%d TLB entries\n",
			    opts->mips_num_tlb_entries);
		if (mci->mci_pdcache_size)
			aprint_normal_dev(dev, "%dKB/%dB %s %s Data cache\n",
			    mci->mci_pdcache_size / 1024, mci->mci_pdcache_line_size,
			    waynames[mci->mci_pdcache_ways],
			    wtnames[mci->mci_pdcache_write_through]);
		break;
#endif /* MIPS1 */
#if (MIPS3 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	case CPU_ARCH_MIPS3:
	case CPU_ARCH_MIPS4:
	case CPU_ARCH_MIPS32:
	case CPU_ARCH_MIPS32R2:
	case CPU_ARCH_MIPS64:
	case CPU_ARCH_MIPS64R2: {
		const char *sufx = "KMGTPE";
		uint32_t pg_mask;
		aprint_normal_dev(dev, "%d TLB entries", opts->mips_num_tlb_entries);
#if !defined(__mips_o32)
		if (CPUIS64BITS) {
			int64_t pfn_mask;
			i = ffs(~(opts->mips3_tlb_vpn_mask >> 31)) + 30;
			aprint_normal(", %d%cB (%d-bit) VAs",
			    1 << (i % 10), sufx[(i / 10) - 1], i);
			for (i = 64, pfn_mask = opts->mips3_tlb_pfn_mask << 6;
			     pfn_mask > 0; i--, pfn_mask <<= 1)
				;
			aprint_normal(", %d%cB (%d-bit) PAs",
			      1 << (i % 10), sufx[(i / 10) - 1], i);
		}
#endif
		for (i = 4, pg_mask = opts->mips3_tlb_pg_mask >> 13;
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
		aprint_normal(", %d%cB max page size\n", i, sufx[0]);
		if (mci->mci_picache_size)
			aprint_normal_dev(dev,
			    "%dKB/%dB %s L1 instruction cache\n",
			    mci->mci_picache_size / 1024,
			    mci->mci_picache_line_size, waynames[mci->mci_picache_ways]);
		if (mci->mci_pdcache_size)
			aprint_normal_dev(dev,
			    "%dKB/%dB %s %s %sL1 data cache\n",
			    mci->mci_pdcache_size / 1024, mci->mci_pdcache_line_size,
			    waynames[mci->mci_pdcache_ways],
			    wtnames[mci->mci_pdcache_write_through],
			    ((opts->mips_cpu_flags & CPU_MIPS_D_CACHE_COHERENT)
				? "coherent " : ""));
		if (mci->mci_sdcache_line_size)
			aprint_normal_dev(dev,
			    "%dKB/%dB %s %s L2 %s cache\n",
			    mci->mci_sdcache_size / 1024, mci->mci_sdcache_line_size,
			    waynames[mci->mci_sdcache_ways],
			    wtnames[mci->mci_sdcache_write_through],
			    mci->mci_scache_unified ? "unified" : "data");
		break;
	}
#endif /* (MIPS3 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */
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
	struct trapframe * const tf = l->l_md.md_utf;
	struct proc * const p = l->l_proc;

	memset(tf, 0, sizeof(struct trapframe));
	tf->tf_regs[_R_SP] = (intptr_t)stack;
	tf->tf_regs[_R_PC] = (intptr_t)pack->ep_entry & ~3;
	tf->tf_regs[_R_T9] = (intptr_t)pack->ep_entry & ~3; /* abicall requirement */
	tf->tf_regs[_R_SR] = PSL_USERSET;
#if !defined(__mips_o32)
	/*
	 * allow 64bit ops in userland for non-O32 ABIs
	 */
	if (p->p_md.md_abi == _MIPS_BSD_API_N32
	    && (CPUISMIPS64 || CPUISMIPS64R2)) {
		tf->tf_regs[_R_SR] |= MIPS_SR_PX;
	} else if (p->p_md.md_abi != _MIPS_BSD_API_O32) {
		tf->tf_regs[_R_SR] |= MIPS_SR_UX;
	}
	if (_MIPS_SIM_NEWABI_P(p->p_md.md_abi))
		tf->tf_regs[_R_SR] |= MIPS3_SR_FR;
#endif
#ifdef _LP64
	/*
	 * If we are using a 32-bit ABI on a 64-bit kernel, mark the process
	 * that way.  If we aren't, clear it.
	 */
	if (p->p_md.md_abi == _MIPS_BSD_API_N32
	    || p->p_md.md_abi == _MIPS_BSD_API_O32) {
		p->p_flag |= PK_32;
	} else {
		p->p_flag &= ~PK_32;
	}
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
	tf->tf_regs[_R_A0] = (intptr_t)stack;
	tf->tf_regs[_R_A1] = 0;
	tf->tf_regs[_R_A2] = 0;
	tf->tf_regs[_R_A3] = p->p_psstrp;

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
#ifdef MIPS3_LOONGSON2
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "loongson-mmi", NULL,
		       NULL, MIPS_HAS_LMMI, NULL, 0,
		       CTL_MACHDEP, CPU_LMMI, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
                       CTLTYPE_INT, "fpu_present", NULL,
                       NULL,
#ifdef NOFPU
		       0,
#else
		       1,
#endif
		       NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);
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

#if 0
struct pcb dumppcb;
#endif

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
	cpuhdrp->sysmappa   = MIPS_KSEG0_TO_PHYS(curcpu()->ci_pmap_kern_segtab);
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
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	nblks = bdev_size(dumpdev);
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

#if 0
	/* Save registers. */
	savectx(&dumppcb);
#endif

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

	psize = bdev_size(dumpdev);
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
	u_int bank = vm_nphysseg - 1;
	struct vm_physseg *vps = VM_PHYSMEM_PTR(bank);
#ifndef _LP64
	/*
	 * Fist the physical segment that can be mapped to KSEG0
	 */
	for (; vps >= vm_physmem; vps--, bank--) {
		if (vps->avail_start + atop(sz) <= atop(MIPS_PHYS_MASK))
			break;
	}
#endif

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

	/* Remove the [last] segment if it now has no pages. */
	if (vps->start == vps->end) {
		for (vm_nphysseg--; bank < vm_nphysseg - 1; bank++) {
			VM_PHYSMEM_PTR_SWAP(bank, bank + 1);
		}
	}

	/* warn if the message buffer had to be shrunk */
	if (sz != reqsz)
		printf("WARNING: %"PRIdVSIZE" bytes not available for msgbuf "
		    "in last cluster (%"PRIdVSIZE" used)\n", reqsz, sz);
}

void
mips_init_lwp0_uarea(void)
{
	struct lwp * const l = &lwp0;
	vaddr_t v;

	if (l->l_addr == NULL) {
		v = uvm_pageboot_alloc(USPACE);
		uvm_lwp_setuarea(&lwp0, v);
	} else {
		v = (vaddr_t)l->l_addr;
	}

	l->l_md.md_utf = (struct trapframe *)(v + USPACE) - 1;
	struct pcb * const pcb = lwp_getpcb(l);
	/*
	 * Now zero out the only two areas of the uarea that we care about.
	 */
	memset(l->l_md.md_utf, 0, sizeof(*l->l_md.md_utf));
	memset(pcb, 0, sizeof(*pcb));

	pcb->pcb_context.val[_L_SR] = MIPS_SR_INT_IE
	    | (ipl_sr_map.sr_bits[IPL_SCHED] ^ MIPS_INT_MASK);
#ifdef __mips_n32
	pcb->pcb_context.val[_L_SR] |= MIPS_SR_KX;
	l->l_md.md_utf->tf_regs[_R_SR] = MIPS_SR_KX;
#endif
#ifdef _LP64
	pcb->pcb_context.val[_L_SR] |= MIPS_SR_KX | MIPS_SR_UX;
	l->l_md.md_utf->tf_regs[_R_SR] = MIPS_SR_KX | MIPS_SR_UX;
#endif
}

int mips_poolpage_vmfreelist = VM_FREELIST_DEFAULT;

#define	HALFGIG		((paddr_t)512 * 1024 * 1024)
#define	FOURGIG		((paddr_t)4 * 1024 * 1024 * 1024)

void
mips_page_physload(vaddr_t vkernstart, vaddr_t vkernend,
	const phys_ram_seg_t *segs, size_t nseg,
	const struct mips_vmfreelist *flp, size_t nfl)
{
	const paddr_t kernstart = MIPS_KSEG0_TO_PHYS(trunc_page(vkernstart));
	const paddr_t kernend = MIPS_KSEG0_TO_PHYS(round_page(vkernend));
#if defined(VM_FREELIST_FIRST4G) || defined(VM_FREELIST_FIRST512M)
#ifdef VM_FREELIST_FIRST512M
	bool need512m = false;
#endif
#ifdef VM_FREELIST_FIRST4G
	bool need4g = false;
#endif

	/*
	 * Do a first pass and see what ranges memory we have to deal with.
	 */
	for (size_t i = 0; i < nseg; i++) {
#ifdef VM_FREELIST_FIRST4G
		if (round_page(segs[i].start + segs[i].size) > FOURGIG) {
			need4g = true;
			mips_poolpage_vmfreelist = VM_FREELIST_FIRST4G;
		}
#endif
#ifdef VM_FREELIST_FIRST512M
		if (round_page(segs[i].start + segs[i].size) > HALFGIG) {
			need512m = true;
			mips_poolpage_vmfreelist = VM_FREELIST_FIRST512M;
		}
#endif
	}
#endif /* VM_FREELIST_FIRST512M || VM_FREELIST_FIRST4G */

	for (; nseg-- > 0; segs++) {
		/*
		 * Make sure everything is in page units.
		 */
		paddr_t segstart = round_page(segs->start);
		const paddr_t segfinish = trunc_page(segs->start + segs->size);

		printf("phys segment: %#"PRIxPADDR" @ %#"PRIxPADDR"\n",
		    segfinish - segstart, segstart);

		/*
		 * Page 0 is reserved for exception vectors.
		 */
		if (segstart == 0) {
			segstart = PAGE_SIZE;
		}
		while (segstart < segfinish) {
			int freelist = -1;	/* unknown freelist */
			paddr_t segend = segfinish;
			for (size_t i = 0; i < nfl; i++) {
				/*
				 * If this segment doesn't overlap the freelist
				 * at all, skip it.
				 */
				if (segstart >= flp[i].fl_end
				    || segend <= flp[i].fl_start)
					continue;
				/*
				 * If the start of this segment starts before
				 * the start of the freelist, then limit the
				 * segment to loaded to the part that doesn't
				 * match this freelist and fall back to normal
				 * freelist matching.
				 */
				if (segstart < flp[i].fl_start) {
					segstart = flp[i].fl_start;
					break;
				}

				/*
				 * We've matched this freelist so remember it.
				 */
				freelist = flp->fl_freelist;

				/*
				 * If this segment extends past the end of this
				 * freelist, bound to segment to the freelist.
				 */
				if (segend > flp[i].fl_end)
					segend = flp[i].fl_end;
				break;
			}
			/*
			 * If we didn't match one of the port dependent
			 * freelists, let's try the common ones.
			 */
			if (freelist == -1) {
#ifdef VM_FREELIST_FIRST512M
				if (need512m && segstart < HALFGIG) {
					freelist = VM_FREELIST_FIRST512M;
					if (segend > HALFGIG)
						segend = HALFGIG;
				} else
#endif
#ifdef VM_FREELIST_FIRST4G
				if (need4g && segstart < FOURGIG) {
					freelist = VM_FREELIST_FIRST4G;
					if (segend > FOURGIG)
						segend = FOURGIG;
				} else
#endif
					freelist = VM_FREELIST_DEFAULT;
			}

			/*
			 * Make sure the memory we provide to uvm doesn't
			 * include the kernel.
			 */
			if (segstart < kernend && segend > kernstart) {
				if (segstart < kernstart) {
					/*
					 * Only add the memory before the
					 * kernel.
					 */
					segend = kernstart;
				} else if (segend > kernend) {
					/*
					 * Only add the memory after the
					 * kernel.
					 */
					segstart = kernend;
				} else {
					/*
					 * Just skip the segment entirely since
					 * it's completely inside the kernel.
					 */
					printf("skipping %#"PRIxPADDR" @ %#"PRIxPADDR" (kernel)\n",
					    segend - segstart, segstart);
					break;
				}
			}

			/*
			 * Now we give this segment to uvm.
			 */
			printf("adding %#"PRIxPADDR" @ %#"PRIxPADDR" to freelist %d\n",

			    segend - segstart, segstart, freelist);
			paddr_t first = atop(segstart);
			paddr_t last = atop(segend);
			uvm_page_physload(first, last, first, last, freelist);

			/*
			 * Start where we finished.
			 */
			segstart = segend;
		}
	}
}

/*
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	ucontext_t * const uc = arg;
	lwp_t * const l = curlwp;
	int error __diagused;

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
	ucontext32_t * const uc = arg;
	lwp_t * const l = curlwp;
	int error __diagused;

	error = cpu_setmcontext32(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	/* Note: we are freeing ucontext_t, not ucontext32_t. */
	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}
#endif /* COMPAT_NETBSD32 */

#ifdef PARANOIA
void
std_splsw_test(void)
{
	struct cpu_info * const ci = curcpu();
	const uint32_t * const sr_map = ipl_sr_map.sr_bits;
	uint32_t status = mips_cp0_status_read();
	uint32_t sr_bits;
	int s;

	KASSERT((status & MIPS_SR_INT_IE) == 0);

	sr_bits = sr_map[IPL_NONE];

	splx(IPL_NONE);
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT(status == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_NONE);

	s = splsoftclock();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_SOFTCLOCK]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_SOFTCLOCK);
	KASSERT(s == IPL_NONE);

	s = splsoftbio();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_SOFTBIO]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_SOFTBIO);
	KASSERT(s == IPL_SOFTCLOCK);

	s = splsoftnet();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_SOFTNET]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_SOFTNET);
	KASSERT(s == IPL_SOFTBIO);

	s = splsoftserial();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_SOFTSERIAL]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_SOFTSERIAL);
	KASSERT(s == IPL_SOFTNET);

	s = splvm();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_VM]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_VM);
	KASSERT(s == IPL_SOFTSERIAL);

	s = splsched();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_SCHED]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_SCHED);
	KASSERT(s == IPL_VM);

	s = splhigh();
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT((status ^ sr_map[IPL_HIGH]) == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_HIGH);
	KASSERT(s == IPL_SCHED);

	splx(IPL_NONE);
	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT(status == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_NONE);

	for (int r = IPL_SOFTCLOCK; r <= IPL_HIGH; r++) {
		/*
		 * As IPL increases, more intrs may be masked but no intrs
		 * may become unmasked.
		 */
		KASSERT((sr_map[r] & sr_bits) == sr_bits);
		sr_bits |= sr_map[r];
		s = splraise(r);
		KASSERT(s == IPL_NONE);

		for (int t = r; t <= IPL_HIGH; t++) {
			int o = splraise(t);
			status = mips_cp0_status_read() & MIPS_INT_MASK;
			KASSERT((status ^ sr_map[t]) == MIPS_INT_MASK);
			KASSERT(ci->ci_cpl == t);
			KASSERT(o == r);

			splx(o);
			status = mips_cp0_status_read() & MIPS_INT_MASK;
			KASSERT((status ^ sr_map[r]) == MIPS_INT_MASK);
			KASSERT(ci->ci_cpl == r);
		}

		splx(s);
		status = mips_cp0_status_read() & MIPS_INT_MASK;
		KASSERT((status ^ sr_map[s]) == MIPS_INT_MASK);
		KASSERT(ci->ci_cpl == s);
	}

	status = mips_cp0_status_read() & MIPS_INT_MASK;
	KASSERT(status == MIPS_INT_MASK);
	KASSERT(ci->ci_cpl == IPL_NONE);
}

#endif /* PARANOIA */

bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
#ifdef _LP64
	if (MIPS_XKSEG_P(pa)) {
		*vap = MIPS_PHYS_TO_XKPHYS_CACHED(pa);
		return true;
	}
#endif
	if (MIPS_KSEG0_P(pa)) {
		*vap = MIPS_PHYS_TO_KSEG0(pa);
		return true;
	}
	return false;
}

bool
mm_md_page_color(paddr_t pa, int *colorp)
{
	if (MIPS_CACHE_VIRTUAL_ALIAS) {
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg != NULL);
		struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);
		*colorp = atop(mdpg->mdpg_first.pv_va);
		return !mips_cache_badalias(pa, mdpg->mdpg_first.pv_va);
	}
	*colorp = 0;
	return true;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa < ctob(physmem)) ? 0 : EFAULT;
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{
	const vaddr_t v = (vaddr_t)ptr;

#ifdef _LP64
	if (v < MIPS_XKPHYS_START) {
		return EFAULT;
	}
	if (MIPS_XKPHYS_P(v) && v > MIPS_PHYS_TO_XKPHYS_CACHED(pmap_limits.avail_end +
	    mips_round_page(MSGBUFSIZE))) {
		return EFAULT;
	}
	if (MIPS_XKSEG_P(v) && v < MIPS_KSEG0_START) {
		*handled = true;
		return 0;
	}
	if (MIPS_KSEG1_P(v) || MIPS_KSEG2_P(v)) {
		return EFAULT;
	}
#else
	if (v < MIPS_KSEG0_START) {
		return EFAULT;
	}
	if (v < MIPS_PHYS_TO_KSEG0(pmap_limits.avail_end +
	    mips_round_page(MSGBUFSIZE))) {
		*handled = true;
		return 0;
	}
	if (v < MIPS_KSEG2_START) {
		return EFAULT;
	}
#endif
	*handled = false;
	return 0;
}

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
static void
mips_watchpoint_init(void)
{
	/*
	 * determine number of CPU watchpoints
	 */
	curcpu()->ci_cpuwatch_count = cpuwatch_discover();
}
#endif


/*
 * Process the tail end of a posix_spawn() for the child.
 */
void
cpu_spawn_return(struct lwp *l)
{
	userret(l);
}

