/* $NetBSD: cpu.h,v 1.60 2003/02/05 12:16:43 nakayama Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifdef _KERNEL
#include <sys/sched.h>
#include <machine/frame.h>

/*
 * Machine check information.
 */
struct mchkinfo {
	__volatile int mc_expected;	/* machine check is expected */
	__volatile int mc_received;	/* machine check was received */
};

struct cpu_info {
	/*
	 * Public members.
	 */
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
	struct lwp *ci_curlwp;		/* current owner of the processor */
	struct cpu_info *ci_next;	/* next cpu_info structure */

	/*
	 * Private members.
	 */
	struct mchkinfo ci_mcinfo;	/* machine check info */
	cpuid_t ci_cpuid;		/* our CPU ID */
	struct lwp *ci_fpcurlwp;	/* current owner of the FPU */
	paddr_t ci_curpcb;		/* PA of current HW PCB */
	struct pcb *ci_idle_pcb;	/* our idle PCB */
	paddr_t ci_idle_pcb_paddr;	/* PA of idle PCB */
	struct cpu_softc *ci_softc;	/* pointer to our device */
	u_long ci_want_resched;		/* preempt current process */
	u_long ci_intrdepth;		/* interrupt trap depth */
	struct trapframe *ci_db_regs;	/* registers for debuggers */

	/*
	 * Variables used by cc_microtime().
	 */
	struct timeval ci_cc_time;
	long ci_cc_cc;
	long ci_cc_ms_delta;
	long ci_cc_denom;

#if defined(MULTIPROCESSOR)
	__volatile u_long ci_flags;	/* flags; see below */
	__volatile u_long ci_ipis;	/* interprocessor interrupts pending */
#endif
};

#define	CPUF_PRIMARY	0x01		/* CPU is primary CPU */
#define	CPUF_PRESENT	0x02		/* CPU is present */
#define	CPUF_RUNNING	0x04		/* CPU is running */
#define	CPUF_PAUSED	0x08		/* CPU is paused */
#define	CPUF_FPUSAVE	0x10		/* CPU is currently in fpusave_cpu() */

extern	struct cpu_info cpu_info_primary;
extern	struct cpu_info *cpu_info_list;

#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	cii = 0, ci = cpu_info_list; \
					ci != NULL; ci = ci->ci_next

#if defined(MULTIPROCESSOR)
extern	__volatile u_long cpus_running;
extern	__volatile u_long cpus_paused;
extern	struct cpu_info *cpu_info[];

#define	curcpu()		((struct cpu_info *)alpha_pal_rdval())
#define	CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

void	cpu_boot_secondary_processors(void);

void	cpu_pause_resume(unsigned long, int);
void	cpu_pause_resume_all(int);
#else /* ! MULTIPROCESSOR */
#define	curcpu()	(&cpu_info_primary)
#endif /* MULTIPROCESSOR */

#define	curlwp		curcpu()->ci_curlwp
#define	fpcurlwp	curcpu()->ci_fpcurlwp
#define	curpcb		curcpu()->ci_curpcb

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_wait(p)		/* nothing */
#define	cpu_number()		alpha_pal_whami()
#define	cpu_proc_fork(p1, p2)	/* nothing */

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.  One the Alpha, we use
 * what we push on an interrupt (a trapframe).
 */
struct clockframe {
	struct trapframe	cf_tf;
};
#define	CLKF_USERMODE(framep)						\
	(((framep)->cf_tf.tf_regs[FRAME_PS] & ALPHA_PSL_USERMODE) != 0)
#define	CLKF_BASEPRI(framep)						\
	(((framep)->cf_tf.tf_regs[FRAME_PS] & ALPHA_PSL_IPL_MASK) == 0)
#define	CLKF_PC(framep)		((framep)->cf_tf.tf_regs[FRAME_PC])

/*
 * This isn't perfect; if the clock interrupt comes in before the
 * r/m/w cycle is complete, we won't be counted... but it's not
 * like this stastic has to be extremely accurate.
 */
#define	CLKF_INTR(framep)	(curcpu()->ci_intrdepth)

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	LWP_PC(p)		((l)->l_md.md_tf->tf_regs[FRAME_PC])

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched(ci)						\
do {									\
	(ci)->ci_want_resched = 1;					\
	if ((ci)->ci_curlwp != NULL)					\
		aston((ci)->ci_curlwp->l_proc);       			\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the Alpha, request an AST to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)						\
do {									\
	(p)->p_flag |= P_OWEUPC;					\
	aston(p);							\
} while (/*CONSTCOND*/0)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston(p)

/*
 * XXXSMP
 * Should we send an AST IPI?  Or just let it handle it next time
 * it sees a normal kernel entry?  I guess letting it happen later
 * follows the `asynchronous' part of the name...
 */
#define	aston(p)	((p)->p_md.md_astpending = 1)
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
#define	CPU_MAXID		8	/* 7 valid machdep IDs */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "root_device", CTLTYPE_STRING }, \
	{ "unaligned_print", CTLTYPE_INT }, \
	{ "unaligned_fix", CTLTYPE_INT }, \
	{ "unaligned_sigbus", CTLTYPE_INT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "fp_sync_complete", CTLTYPE_INT }, \
}

#ifdef _KERNEL

struct pcb;
struct proc;
struct reg;
struct rpb;
struct trapframe;

extern struct timeval cc_microset_time;

int	badaddr(void *, size_t);
#define microtime(tv)	cc_microtime(tv)
void	cc_microtime __P((struct timeval *));
void	cc_microset(struct cpu_info *);

#endif /* _KERNEL */
#endif /* _ALPHA_CPU_H_ */
