/* $NetBSD: cpu.h,v 1.101.2.1 2021/08/01 22:42:01 thorpej Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Charles M. Hannum.
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

#ifndef _ALPHA_CPU_H_
#define _ALPHA_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#endif

/*
 * Exported definitions unique to Alpha cpu support.
 */

#include <machine/alpha_cpu.h>

#if defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/cpu_data.h>
#include <sys/cctr.h>
#include <sys/intr.h>
#include <machine/frame.h>

/*
 * Machine check information.
 */
struct mchkinfo {
	volatile int mc_expected;	/* machine check is expected */
	volatile int mc_received;	/* machine check was received */
};

/*
 * Per-cpu information.  Data accessed by MI code is marked [MI].
 */
struct cpu_info {
	struct cpu_data ci_data;	/* [MI] general per-cpu data */
	struct lwp *ci_curlwp;		/* [MI] current owner of the cpu */
	struct lwp *ci_onproc;		/* [MI] current user LWP / kthread */
	struct cctr_state ci_cc;	/* [MI] cycle counter state */

	volatile int ci_mtx_count;	/* [MI] neg count of spin mutexes */
	volatile int ci_mtx_oldspl;	/* [MI] for spin mutex splx() */

	u_long ci_intrdepth;		/* interrupt trap depth */
	volatile u_long ci_ssir;	/* simulated software interrupt reg */
					/* LWPs for soft intr dispatch */
	struct lwp *ci_silwps[SOFTINT_COUNT];
	struct cpu_softc *ci_softc;	/* pointer to our device */

	struct pmap *ci_pmap;		/* currently-activated pmap */
	u_int ci_next_asn;		/* next ASN to assign */
	u_long ci_asn_gen;		/* current ASN generation */

	struct mchkinfo ci_mcinfo;	/* machine check info */

	/*
	 * The following must be in their own cache line, as they are
	 * stored to regularly by remote CPUs.
	 */
	volatile u_long ci_ipis		/* interprocessor interrupts pending */
			__aligned(64);
	u_int	ci_want_resched;	/* [MI] preempt current process */

	/*
	 * These are largely static, and will frequently be fetched
	 * by other CPUs.  For that reason, they get their own cache
	 * line, too.
	 */
	struct cpu_info *ci_next	/* next cpu_info structure */
			__aligned(64);
	cpuid_t ci_cpuid;		/* [MI] our CPU ID */
	volatile u_long ci_flags;	/* flags; see below */
	uint64_t ci_pcc_freq;		/* cpu cycles/second */
	struct trapframe *ci_db_regs;	/* registers for debuggers */
	u_int	ci_nintrhand;		/* # of interrupt handlers */
};

/* Ensure some cpu_info fields are within the signed 16-bit displacement. */
__CTASSERT(offsetof(struct cpu_info, ci_curlwp) <= 0x7ff0);
__CTASSERT(offsetof(struct cpu_info, ci_ssir) <= 0x7ff0);

#endif /* _KERNEL || _KMEMUSER */

#if defined(_KERNEL)

#define	CPUF_PRIMARY	0x01		/* CPU is primary CPU */
#define	CPUF_PRESENT	0x02		/* CPU is present */
#define	CPUF_RUNNING	0x04		/* CPU is running */
#define	CPUF_PAUSED	0x08		/* CPU is paused */

extern	struct cpu_info cpu_info_primary;
extern	struct cpu_info *cpu_info_list;

#define	CPU_INFO_ITERATOR		int __unused
#define	CPU_INFO_FOREACH(cii, ci)	ci = cpu_info_list; \
					ci != NULL; ci = ci->ci_next

#if defined(MULTIPROCESSOR)
extern	volatile u_long cpus_running;
extern	volatile u_long cpus_paused;
extern	struct cpu_info *cpu_info[];

#define	curlwp			((struct lwp *)alpha_pal_rdval())
#define	curcpu()		curlwp->l_cpu
#define	CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

void	cpu_boot_secondary_processors(void);

void	cpu_pause_resume(unsigned long, int);
void	cpu_pause_resume_all(int);
#else /* ! MULTIPROCESSOR */
#define	curcpu()	(&cpu_info_primary)
#define	curlwp		curcpu()->ci_curlwp
#endif /* MULTIPROCESSOR */


/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_number()		alpha_pal_whami()
#define	cpu_proc_fork(p1, p2)	/* nothing */

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  On the alpha, we use
 * what we push on an interrupt (a trapframe).
 */
struct clockframe {
	struct trapframe	cf_tf;
};
#define	CLKF_USERMODE(framep)						\
	(((framep)->cf_tf.tf_regs[FRAME_PS] & ALPHA_PSL_USERMODE) != 0)
#define	CLKF_PC(framep)		((framep)->cf_tf.tf_regs[FRAME_PC])

/*
 * This isn't perfect; if the clock interrupt comes in before the
 * r/m/w cycle is complete, we won't be counted... but it's not
 * like this statistic has to be extremely accurate.
 */
#define	CLKF_INTR(framep)						\
	((curcpu()->ci_intrdepth & 0xf) != 0)	/* see interrupt() */

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	LWP_PC(p)		((l)->l_md.md_tf->tf_regs[FRAME_PC])

void	cpu_need_proftick(struct lwp *);
void	cpu_signotify(struct lwp *);

#define	aston(l)		((l)->l_md.md_astpending = 1)
#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_ROOT_DEVICE		2	/* string: root device name */
#define	CPU_UNALIGNED_PRINT	3	/* int: print unaligned accesses */
#define	CPU_UNALIGNED_FIX	4	/* int: fix unaligned accesses */
#define	CPU_UNALIGNED_SIGBUS	5	/* int: SIGBUS unaligned accesses */
#define	CPU_BOOTED_KERNEL	6	/* string: booted kernel name */
#define	CPU_FP_SYNC_COMPLETE	7	/* int: always fixup sync fp traps */
#define	CPU_CCTR		8	/* int: using CC timecounter */
#define	CPU_IS_QEMU		9	/* int: running under Qemu */
#define	CPU_FP_COMPLETE_DEBUG	10	/* int: enable FP completion debug */


#ifdef _KERNEL

struct pcb;
struct proc;
struct reg;
struct rpb;
struct trapframe;

int	badaddr(void *, size_t);
void *	cpu_uarea_alloc(bool);
bool	cpu_uarea_free(void *);

void	cpu_idle_wtint(void);
extern	void (*cpu_idle_fn)(void);
#define	cpu_idle()	(*cpu_idle_fn)()

void	cpu_initclocks_secondary(void);

#endif /* _KERNEL */
#endif /* _ALPHA_CPU_H_ */
