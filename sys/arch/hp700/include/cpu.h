/*	$NetBSD: cpu.h,v 1.70.2.1 2013/02/25 00:28:41 tls Exp $	*/

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

#ifdef _KERNEL_OPT
#include "opt_cputype.h"
#include "opt_multiprocessor.h"
#endif

#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/intrdefs.h>

#ifndef __ASSEMBLER__
#include <machine/intr.h>
#endif

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
extern int cpu_modelno;
extern int cpu_revision;
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
#if defined(HP8000_CPU) || defined(HP8200_CPU) || \
    defined(HP8500_CPU) || defined(HP8600_CPU)

/* PA2.0 aliases */
#define	HPPA_PGALIAS	0x00400000
#define	HPPA_PGAMASK	0xffc00000	/* PA bits 0-9 not used in index */
#define	HPPA_PGAOFF	0x003fffff

#else

/* PA1.x aliases */
#define	HPPA_PGALIAS	0x00100000
#define	HPPA_PGAMASK	0xfff00000	/* PA bits 0-11 not used in index */
#define	HPPA_PGAOFF	0x000fffff

#endif

#define	HPPA_SPAMASK	0xf0f0f000	/* PA bits 0-3,8-11,16-19 not used */

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

#ifdef MULTIPROCESSOR

#define	GET_CURCPU(r)		mfctl CR_CURCPU, r
#define	GET_CURCPU_SPACE(s, r)	GET_CURCPU(r)
#define	GET_CURLWP(r)		mfctl CR_CURCPU, r ! ldw CI_CURLWP(r), r
#define	GET_CURLWP_SPACE(s, r)	mfctl CR_CURCPU, r ! ldw CI_CURLWP(s, r), r

#define	SET_CURLWP(r,t)		mfctl CR_CURCPU, t ! stw r, CI_CURLWP(t)

#else /*  MULTIPROCESSOR */

#define	GET_CURCPU(r)		mfctl CR_CURLWP, r ! ldw L_CPU(r), r
#define	GET_CURCPU_SPACE(s, r)	mfctl CR_CURLWP, r ! ldw L_CPU(s, r), r
#define	GET_CURLWP(r)		mfctl CR_CURLWP, r
#define	GET_CURLWP_SPACE(s, r)	GET_CURLWP(r)

#define	SET_CURLWP(r,t) mtctl   r, CR_CURLWP

#endif /*  MULTIPROCESSOR */

#ifndef _LOCORE
#ifdef _KERNEL

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

int	clock_intr(void *);

/*
 * LWP_PC: the program counter for the given lwp.
 */
#define	LWP_PC(l)		((l)->l_md.md_regs->tf_iioq_head)

#define	cpu_signotify(l)	(setsoftast(l))
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, setsoftast(l))

#endif /* _KERNEL */

#ifndef __ASSEMBLER__
#if defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/cpu_data.h>
#include <sys/evcnt.h>

/*
 * Note that the alignment of ci_trap_save is important since we want to keep
 * it within a single cache line.  As a result, it must be kept as the first
 * entry within the cpu_info struct.
 */
struct cpu_info {
	/* Keep this first to simplify the trap handlers */
	register_t	ci_trapsave[16];/* the "phys" part of frame */

	struct cpu_data ci_data;	/* MI per-cpu data */

#ifndef _KMEMUSER
	hppa_hpa_t	ci_hpa;
	register_t	ci_psw;		/* Processor Status Word. */
	paddr_t		ci_fpu_state;	/* LWP FPU state address, or zero. */
	u_long		ci_itmr;

	int		ci_cpuid;	/* CPU index (see cpus[] array) */
	int		ci_mtx_count;
	int		ci_mtx_oldspl;
	int		ci_want_resched;

	volatile int	ci_cpl;
	volatile int	ci_ipending;	/* The pending interrupts. */
	u_int		ci_intr_depth;	/* Nonzero iff running an interrupt. */
	u_int		ci_ishared;
	u_int		ci_eiem;

	u_int		ci_imask[NIPL];

	struct hp700_interrupt_register	ci_ir;
	struct hp700_interrupt_bit	ci_ib[HP700_INTERRUPT_BITS];
	
#if defined(MULTIPROCESSOR)
	struct lwp	*ci_curlwp;	/* CPU owner */
	paddr_t		ci_stack;	/* stack for spin up */
	volatile int	ci_flags;	/* CPU status flags */
#define	CPUF_PRIMARY	0x0001		/* ... is monarch/primary */
#define	CPUF_RUNNING	0x0002 		/* ... is running. */

	volatile u_long	ci_ipi;		/* IPIs pending */

	struct cpu_softc *ci_softc;
#endif

#endif /* !_KMEMUSER */
} __aligned(64);

#endif /* _KERNEL || _KMEMUSER */
#endif /* __ASSEMBLER__ */

#if defined(_KERNEL)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

#define	cpu_proc_fork(p1, p2)

#ifdef MULTIPROCESSOR

/* Number of CPUs in the system */
extern int hppa_ncpu;

#define	HPPA_MAXCPUS	4
#define	cpu_number()			(curcpu()->ci_cpuid)

#define	CPU_IS_PRIMARY(ci)		((ci)->ci_cpuid == 0)
#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	cii = 0, ci =  &cpus[0]; cii < hppa_ncpu; cii++, ci++

void	cpu_boot_secondary_processors(void);

static __inline struct cpu_info *
hppa_curcpu(void)
{
	struct cpu_info *ci;

	__asm volatile("mfctl %1, %0" : "=r" (ci): "i" (CR_CURCPU));

	return ci;
}

#define	curcpu()			hppa_curcpu()

#else /*  MULTIPROCESSOR */

#define	HPPA_MAXCPUS	1
#define	curcpu()			(&cpus[0])
#define	cpu_number()			0

static __inline struct lwp *
hppa_curlwp(void)
{
	struct lwp *l;

	__asm volatile("mfctl %1, %0" : "=r" (l): "i" (CR_CURLWP));

	return l;
}

#define	curlwp				hppa_curlwp()

#endif /* MULTIPROCESSOR */

extern struct cpu_info cpus[HPPA_MAXCPUS];

#define	DELAY(x) delay(x)

static __inline paddr_t
kvtop(const void *va)
{
	paddr_t pa;

	__asm volatile ("lpa %%r0(%1), %0" : "=r" (pa) : "r" (va));
	return pa;
}

extern int (*cpu_desidhash)(void);

static __inline bool
hppa_cpu_ispa20_p(void)
{

	return (hppa_cpu_info->hci_features & HPPA_FTRS_W32B) != 0;
}

static __inline bool
hppa_cpu_hastlbu_p(void)
{

	return (hppa_cpu_info->hci_features & HPPA_FTRS_TLBU) != 0;
}

void	delay(u_int);
void	hppa_init(paddr_t, void *);
void	trap(int, struct trapframe *);
void	hppa_ras(struct lwp *);
int	spcopy(pa_space_t, const void *, pa_space_t, void *, size_t);
int	spstrcpy(pa_space_t, const void *, pa_space_t, void *, size_t,
    size_t *);
int	copy_on_fault(void);
void	lwp_trampoline(void);
int	cpu_dumpsize(void);
int	cpu_dump(void);

#ifdef MULTIPROCESSOR
void	cpu_boot_secondary_processors(void);
void	cpu_hw_init(void);
void	cpu_hatch(void);
#endif
#endif	/* _KERNEL */

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
#define	CPU_LCD_BLINK           3	/* int: twiddle heartbeat LED/LCD */
#define	CPU_MAXID		4	/* number of valid machdep ids */

#ifdef _KERNEL
#include <sys/queue.h>

struct blink_lcd {
	void (*bl_func)(void *, int);
	void *bl_arg;
	SLIST_ENTRY(blink_lcd) bl_next;
};

extern void blink_lcd_register(struct blink_lcd *);
#endif	/* _KERNEL */
#endif	/* !_LOCORE */

#endif /* _MACHINE_CPU_H_ */
