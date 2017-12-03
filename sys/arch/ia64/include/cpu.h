/*	$NetBSD: cpu.h,v 1.10.12.2 2017/12/03 11:36:20 jdolecek Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
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


/*-
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


#ifndef _IA64_CPU_H_
#define _IA64_CPU_H_

#ifdef _KERNEL
#include <sys/cpu_data.h>
#include <sys/cctr.h>
#include <machine/frame.h>
#include <machine/ia64_cpu.h>
#include <sys/device_if.h>

struct cpu_info {

	/*
	 * Public members.
	 */

	struct cpu_data ci_data;	/* MI per-cpu data */
	device_t ci_dev;		/* pointer to our device */
	struct lwp *ci_curlwp;		/* current owner of the processor */
	struct cctr_state ci_cc;	/* cycle counter state */
	struct cpu_info *ci_next;	/* next cpu_info structure */

	volatile int ci_mtx_count;	/* Negative count of spin mutexes */
	volatile int ci_mtx_oldspl;	/* Old SPL at this ci_idepth */

	/* XXX: Todo */
	/*
	 * Private members.
	 */
	cpuid_t ci_cpuid;		/* our CPU ID */
	uint32_t ci_acpiid;		/* our ACPI/MADT ID */
	uint32_t ci_initapicid;		/* our intitial APIC ID */
	struct pmap *ci_pmap;		/* current pmap */ /* XXX FreeBSD has *pcb_current_pmap in pcb ? */
	struct lwp *ci_fpcurlwp;	/* current owner of the FPU */
	paddr_t ci_curpcb;		/* PA of current HW PCB */
	struct pcb *ci_idle_pcb;	/* our idle PCB */
	u_long ci_want_resched;		/* preempt current process */
	u_long ci_intrdepth;		/* interrupt trap depth */
	struct trapframe *ci_db_regs;	/* registers for debuggers */
	uint64_t ci_clock;		/* clock counter */
	uint64_t ci_clockadj;		/* clock adjust */
	uint64_t ci_vhpt;		/* address of vhpt */
};


extern struct cpu_info cpu_info_primary;
extern struct cpu_info *cpu_info_list;

#define	CPU_INFO_ITERATOR		int __unused
#define	CPU_INFO_FOREACH(cii, ci)	ci = cpu_info_list; \
					ci != NULL; ci = ci->ci_next
#ifdef MULTIPROCESSOR
/*
 * XXX: TODO use percpu infrastructure that yamt proposed or use KR? for
 * storing the percpu pointer.
 */
#else
#define	curcpu() (&cpu_info_primary)
#endif /* MULTIPROCESSOR */
#define curlwp	(curcpu()->ci_curlwp)

#define cpu_number() 0              /*XXX: FIXME */

#define aston(l) ((l)->l_md.md_astpending = 1)

#define	need_resched(ci)            /*XXX: FIXME */

struct clockframe {
	struct trapframe cf_tf;
};

#define	CLKF_PC(cf)		(TRAPF_PC(&(cf)->cf_tf))
#define	CLKF_CPL(cf)		(TRAPF_CPL(&(cf)->cf_tf))
#define	CLKF_USERMODE(cf)	(TRAPF_USERMODE(&(cf)->cf_tf))
#define	CLKF_INTR(frame)	(curcpu()->ci_intrdepth)

#define	TRAPF_PC(tf)		((tf)->tf_special.iip)
#define	TRAPF_CPL(tf)		((tf)->tf_special.psr & IA64_PSR_CPL)
#define	TRAPF_USERMODE(tf)	(TRAPF_CPL(tf) != IA64_PSR_CPL_KERN)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid. XXX:Fixme.... On the ia64 I haven't yet figured 
 * out what to do about this.. XXX.
 */
/* extern void	cpu_need_proftick(struct lwp *l); */
#define cpu_need_proftick(l) __nothing

/*
 * Notify the LWP l that it has a signal pending, process as soon as possible.
 */
#define	cpu_signotify(l)	aston(l)

// void cpu_need_resched(struct cpu_info *ci, int flags)
#define cpu_need_resched(ci, f) do {	\
	__USE(ci);			\
	__USE(f);			\
} while(/*CONSTCOND*/0)

#define setsoftclock()        __nothing       /*XXX: FIXME */

/* machdep.c */
int cpu_maxproc(void); /*XXX: Fill in machdep.c */

#define	cpu_proc_fork(p1, p2)  __nothing	/* XXX: Look into this. */

#define DELAY(x)	 __nothing	/* XXX: FIXME */

static inline void cpu_idle(void);
static inline
void cpu_idle(void)
{
	asm ("hint @pause" ::: "memory");
}

#endif /* _KERNEL_ */
#endif /* _IA64_CPU_H */
