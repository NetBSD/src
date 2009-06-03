/*	$NetBSD: cpu.h,v 1.34 2009/06/03 21:08:51 skrll Exp $	*/

/*	$OpenBSD: cpu.h,v 1.55 2008/07/23 17:39:35 kettenis Exp $	*/

/*
 * Copyright (c) 2000-2004 Michael Shalayeff
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1988-1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: cpu.h 1.19 94/12/16$
 */

#ifndef	_MACHINE_CPU_H_
#define	_MACHINE_CPU_H_

#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/reg.h>


#ifndef _LOCORE

/* types */
enum hppa_cpu_type {
	hpc_unknown,
	hpcx,	/* PA7000 (x)		PA 1.0 */
	hpcxs,	/* PA7000 (s)		PA 1.1a */
	hpcxt,	/* PA7100 (t)		PA 1.1b */
	hpcxl,	/* PA7100LC (l)		PA 1.1c */
	hpcxtp,	/* PA7200 (t')		PA 1.1d */
	hpcxl2,	/* PA7300LC (l2)	PA 1.1e */
	hpcxu,	/* PA8000 (u)		PA 2.0 */
	hpcxup,	/* PA8200 (u+)		PA 2.0 */	
	hpcxw,	/* PA8500 (w)		PA 2.0 */
	hpcxwp,	/* PA8600 (w+)		PA 2.0 */
	hpcxw2, /* PA8700 (piranha)	PA 2.0 */
	mako	/* PA8800 (mako)	PA 2.0 */
};

/*
 * A CPU description.
 */
struct hppa_cpu_info {
	/* The official name of the chip. */
	const char *hci_chip_name;

	/* The nickname for the chip. */
	const char *hci_chip_nickname;

	/* The type and PA-RISC specification of the chip. */
	const char hci_chip_type[8];
	enum hppa_cpu_type hci_cputype;
	int  hci_cpuversion;
	int  hci_features;		/* CPU types and features */
#define	HPPA_FTRS_TLBU		0x00000001
#define	HPPA_FTRS_BTLBU		0x00000002
#define	HPPA_FTRS_HVT		0x00000004
#define	HPPA_FTRS_W32B		0x00000008

	const char *hci_chip_spec;

	int (*desidhash)(void);
	const u_int *itlbh, *dtlbh, *itlbnah, *dtlbnah, *tlbdh;
	int (*dbtlbins)(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
	int (*ibtlbins)(int, pa_space_t, vaddr_t, paddr_t, vsize_t, u_int);
	int (*btlbprg)(int);
	int (*hptinit)(vaddr_t, vsize_t);
};

#ifdef _KERNEL
extern const struct hppa_cpu_info *hppa_cpu_info;
extern int cpu_hvers;
extern int cpu_revision;
extern register_t kpsw;
#endif
#endif

/*
 * COPR/SFUs
 */
#define	HPPA_FPUS	0xc0
#define	HPPA_PMSFUS	0x20	/* ??? */

/*
 * Exported definitions unique to hp700/PA-RISC cpu support.
 */

/*
 * COPR/SFUs
 */
#define	HPPA_FPUVER(w)	(((w) & 0x003ff800) >> 11)
#define	HPPA_FPU_OP(w)	((w) >> 26)
#define	HPPA_FPU_UNMPL	0x01	/* exception reg, the rest is << 1 */
#define	HPPA_FPU_ILL	0x80	/* software-only */
#define	HPPA_FPU_I	0x01
#define	HPPA_FPU_U	0x02
#define	HPPA_FPU_O	0x04
#define	HPPA_FPU_Z	0x08
#define	HPPA_FPU_V	0x10
#define	HPPA_FPU_D	0x20
#define	HPPA_FPU_T	0x40
#define	HPPA_FPU_XMASK	0x7f
#define	HPPA_FPU_T_POS	25
#define	HPPA_FPU_RM	0x00000600
#define	HPPA_FPU_CQ	0x00fff800
#define	HPPA_FPU_C	0x04000000
#define	HPPA_FPU_FLSH	27
#define	HPPA_FPU_INIT	(0)
#define	HPPA_FPU_FORK(s) ((s) & ~((uint64_t)(HPPA_FPU_XMASK) << 32))

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

#define	HPPA_PGALIAS	0x00100000
#define	HPPA_PGAMASK	0xfff00000
#define	HPPA_PGAOFF	0x000fffff
#define	HPPA_SPAMASK	0xf0f0f000

#define	HPPA_IOSPACE	0xf0000000
#define	HPPA_IOLEN      0x10000000
#define	HPPA_PDC_LOW	0xef000000
#define	HPPA_PDC_HIGH	0xf1000000
#define	HPPA_IOBCAST	0xfffc0000
#define	HPPA_LBCAST	0xfffc0000
#define	HPPA_GBCAST	0xfffe0000
#define	HPPA_FPA	0xfff80000
#define	HPPA_FLEX_DATA	0xfff80001
#define	HPPA_DMA_ENABLE	0x00000001
#define	HPPA_SPA_ENABLE	0x00000020
#define	HPPA_NMODSPBUS	64

#ifndef _LOCORE
#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

/*
 * External definitions unique to PA-RISC cpu support.
 * These are the "public" declarations - those needed in
 * machine-independent source code.  The "private" ones
 * are in machdep.h.
 *
 * Note that the name of this file is NOT meant to imply
 * that it has anything to do with PA-RISC CPU stuff.
 * The name "cpu" is historical, and used in the common
 * code to identify machine-dependent functions, etc.
 */

/* clockframe describes the system before we took an interrupt. */
struct clockframe {
	int	cf_flags;
	int	cf_spl;
	u_int	cf_pc;
};
#define	CLKF_PC(framep)		((framep)->cf_pc)
#define	CLKF_INTR(framep)	((framep)->cf_flags & TFF_INTR)
#define	CLKF_USERMODE(framep)	((framep)->cf_flags & T_USER)

#define	cpu_signotify(l)	(setsoftast())
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, setsoftast())

#include <sys/cpu_data.h>
struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */

	struct	lwp	*ci_curlwp;	/* CPU owner */
	int		ci_cpuid;	/* CPU index (see cpus[] array) */
	int		ci_mtx_count;
	int		ci_mtx_oldspl;
	int		ci_want_resched;
};

#include <machine/intr.h>

extern struct cpu_info cpu_info_store;

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

#define	curcpu()			(&cpu_info_store)
#define	cpu_number()			0

#define cpu_proc_fork(p1, p2)

#ifdef MULTIPROCESSOR
#define	CPU_IS_PRIMARY(ci)		1
#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	cii = 0; ci = curcpu(), cii < 1; cii++

void	cpu_boot_secondary_processors(void);
#endif

static __inline struct lwp *
hppa_curlwp(void)
{
	struct lwp *l;

	__asm volatile("mfctl %1, %0" : "=r" (l): "i" (CR_CURLWP));

	return (struct lwp *)l;
}

#define	curlwp				hppa_curlwp()

#define DELAY(x) delay(x)

static __inline paddr_t
kvtop(const void *va)
{
	paddr_t pa;

	__asm volatile ("lpa %%r0(%1), %0" : "=r" (pa) : "r" (va));
	return pa;
}

extern int (*cpu_desidhash)(void);

void	delay(u_int);
void	hppa_init(paddr_t, void *);
void	trap(int, struct trapframe *);
void	hppa_ras(struct lwp *);
int	spcopy(pa_space_t, const void *, pa_space_t, void *, size_t);
int	spstrcpy(pa_space_t, const void *, pa_space_t, void *, size_t,
		 size_t *);
int	copy_on_fault(void);
void	lwp_trampoline(void);
void	setfunc_trampoline(void);
int	cpu_dumpsize(void);
int	cpu_dump(void);
#endif

/*
 * Boot arguments stuff
 */

#define	BOOTARG_LEN	(PAGE_SIZE)
#define	BOOTARG_OFF	(0x10000)

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_BOOTED_KERNEL	2	/* string: booted kernel name */
#define	CPU_MAXID		3	/* number of valid machdep ids */

#endif

#endif /* _MACHINE_CPU_H_ */
