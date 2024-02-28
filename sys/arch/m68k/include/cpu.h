/*	$NetBSD: cpu.h,v 1.25 2024/02/28 13:05:39 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _M68K_CPU_H_
#define	_M68K_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_m68k_arch.h"	/* XXX Should not do this here. */
#endif

/*
 * Exported definitions common to Motorola m68k-based ports.
 */

/*
 * XXX  The remaining contents of this file should be split out
 * XXX  into separate files (like m68k.h) and then this file
 * XXX  should go away.  Furthermore, most of the stuff defined
 * XXX  here does NOT belong in <machine/cpu.h>, and the ports
 * XXX  using this file should remove <m68k/cpu.h> from there.
 */

#include <m68k/m68k.h>

/* XXX - Move this stuff into <m68k/cacr.h> maybe? */

/* fields in the 68020 cache control register */
#define	IC_ENABLE	0x0001	/* enable instruction cache */
#define	IC_FREEZE	0x0002	/* freeze instruction cache */
#define	IC_CE		0x0004	/* clear instruction cache entry */
#define	IC_CLR		0x0008	/* clear entire instruction cache */

/* additional fields in the 68030 cache control register */
#define	IC_BE		0x0010	/* instruction burst enable */
#define	DC_ENABLE	0x0100	/* data cache enable */
#define	DC_FREEZE	0x0200	/* data cache freeze */
#define	DC_CE		0x0400	/* clear data cache entry */
#define	DC_CLR		0x0800	/* clear entire data cache */
#define	DC_BE		0x1000	/* data burst enable */
#define	DC_WA		0x2000	/* write allocate */

/* fields in the 68040 cache control register */
#define	IC40_ENABLE	0x00008000	/* instruction cache enable bit */
#define	DC40_ENABLE	0x80000000	/* data cache enable bit */

/* additional fields in the 68060 cache control register */
#define	DC60_NAD	0x40000000	/* no allocate mode, data cache */
#define	DC60_ESB	0x20000000	/* enable store buffer */
#define	DC60_DPI	0x10000000	/* disable CPUSH invalidation */
#define	DC60_FOC	0x08000000	/* four kB data cache mode (else 8) */

#define	IC60_EBC	0x00800000	/* enable branch cache */
#define IC60_CABC	0x00400000	/* clear all branch cache entries */
#define	IC60_CUBC	0x00200000	/* clear user branch cache entries */

#define	IC60_NAI	0x00004000	/* no allocate mode, instr. cache */
#define	IC60_FIC	0x00002000	/* four kB instr. cache (else 8) */

#define	CACHE_ON	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	CACHE_OFF	(DC_CLR|IC_CLR)
#define	CACHE_CLR	(CACHE_ON)
#define	IC_CLEAR	(DC_WA|DC_BE|DC_ENABLE|IC_BE|IC_CLR|IC_ENABLE)
#define	DC_CLEAR	(DC_WA|DC_BE|DC_CLR|DC_ENABLE|IC_BE|IC_ENABLE)

#define	CACHE40_ON	(IC40_ENABLE|DC40_ENABLE)
#define	CACHE40_OFF	(0x00000000)

#define	CACHE60_ON	(CACHE40_ON|IC60_CABC|IC60_EBC|DC60_ESB)
#define	CACHE60_OFF	(CACHE40_OFF|IC60_CABC)

#define CACHELINE_SIZE	16
#define CACHELINE_MASK	(CACHELINE_SIZE - 1)

/* CTL_MACHDEP definitions. (Common to all m68k ports.) */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_ROOT_DEVICE		2	/* string: root device name */
#define	CPU_BOOTED_KERNEL	3	/* string: booted kernel name */

#if defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/cpu_data.h>

struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	cpuid_t	ci_cpuid;
	int	ci_mtx_count;
	int	ci_mtx_oldspl;
	volatile int	ci_want_resched;
	volatile int	ci_idepth;
	struct lwp *ci_onproc;		/* current user LWP / kthread */
};
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL
extern struct cpu_info cpu_info_store;

struct	proc;
void	cpu_proc_fork(struct proc *, struct proc *);

#define	curcpu()	(&cpu_info_store)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define cpu_number()			0

#define LWP_PC(l)	(((struct trapframe *)((l)->l_md.md_regs))->tf_pc)

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the m68k platforms, we use
 * what the interrupt stub puts on the stack before calling C code.
 */
struct clockframe {
	/* regs saved on the stack by the interrupt stub */
	u_int	cf_regs[4];	/* d0,d1,a0,a1 */
	/* hardware frame */
	u_short	cf_sr;		/* sr at time of interrupt */
	u_long	cf_pc;		/* pc at time of interrupt */
	u_short	cf_vo;		/* vector offset (4-word HW frame) */
} __attribute__((__packed__));

#define	CLKF_USERMODE(framep)	(((framep)->cf_sr & PSL_S) == 0)
#define	CLKF_PC(framep)		((framep)->cf_pc)

#if 0
/*
 * We can determine if we were previously in an interrupt context
 * if we were running on the interrupt stack (as opposed to the
 * "master" stack).
 *
 * XXX Actually, we can't, because we don't use the master stack
 * XXX right now.
 *
 * (Actually, it's unlikely that we'll ever use the master stack in NetBSD.
 * It would complicate the spl*() functions considerably and it just doesn't
 * seem like a good trade-off for what seems like extremely marginal gains.
 * So, just blissfully run the kernel on the interrupt stack all the time,
 * and it's been that way for >30 years and no one has really complained
 * about it.)
 */
#define	CLKF_INTR(framep)	(((framep)->cf_sr & PSL_M) == 0)
#else
/*
 * The clock interrupt handler can determine if it's a nested
 * interrupt by checking for intr_depth > 1.
 * (Remember, the clock interrupt handler itself will cause the
 * depth counter to be incremented).
 */
extern volatile unsigned int intr_depth;
#define	CLKF_INTR(framep)	(intr_depth > 1)
#endif

#ifndef __HAVE_M68K_HW_AST
#define	cpu_set_hw_ast(l)	__nothing
#endif

extern volatile int astpending;
#define	cpu_set_ast(l)							\
	do {								\
		__USE(l); astpending = 1; cpu_set_hw_ast(l);		\
	} while (/*CONSTCOND*/0)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	cpu_need_resched(ci, l, flags)					\
	do {								\
		__USE(ci); __USE(flags); cpu_set_ast(l);		\
	} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On m68k, request an ast to send us through
 * trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)						\
	do {								\
		(l)->l_pflag |= LP_OWEUPC; cpu_set_ast(l);		\
	} while (/*CONSTCOND*/0)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	cpu_set_ast(l)

#endif /* _KERNEL */

#endif /* _M68K_CPU_H_ */
