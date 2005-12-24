/*	$NetBSD: cpu.h,v 1.17 2005/12/24 20:07:32 perry Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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
/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc. All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)cpu.h	5.4 (Berkeley) 5/9/91
 */

/*
 * SH5 support.
 *
 * Based loosely on T.Horiuchi's version for NetBSD/sh3.
 */

#ifndef _SH5_CPU_H_
#define	_SH5_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

#include <machine/frame.h>
#include <sh5/conreg.h>
#include <sh5/pte.h>

/*
 * Important kernel virtual addresses
 */
#ifndef _LP64
#define	SH5_KSEG0_BASE		0xc0000000
#define	SH5_KSEG1_BASE		0xe0000000
#else
#define	SH5_KSEG0_BASE		0xffffffffc0000000
#define	SH5_KSEG1_BASE		0xffffffffe0000000
#endif
#define	SH5_KSEG0_SIZE		0x20000000
#define	SH5_KSEG1_SIZE		0x20000000

/*
 * Default to NEFF == 32 if port-specific code doesn't define it
 */
#ifndef SH5_NEFF_BITS
#define	SH5_NEFF_BITS 32
#endif

/*
 * This hairy macro evaluates true if (register_t)`eff' is a valid
 * effective address for the CPU's Implemented Effective Address space.
 */
#if SH5_NEFF_BITS < 64
#define	SH5_EFF_IS_VALID(eff)	(((((eff) & (1ULL << 63)) ?	\
    ((eff) ^ 0xffffffff00000000ULL) : (eff)) &			\
	~((1ULL << SH5_NEFF_BITS) - 1)) == 0)
#else
#define	SH5_EFF_IS_VALID(eff)	1	/* Everything's valid in this case */
#endif


#ifndef SH5_ASID_BITS
#define	SH5_ASID_BITS 8
#endif
#define	SH5_NUM_ASIDS (1 << SH5_ASID_BITS)


#ifdef _KERNEL
#include <sys/cpu_data.h>
struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */

	struct lwp *ci_curlwp;		/* current lwp on this cpu */
	struct pcb *ci_curpcb;		/* current process' pcb */
	vsid_t ci_curvsid;		/* current pmap's vsid */
	u_int ci_want_resched;		/* current process pre-empted */
	volatile u_int ci_intr_depth;	/* nesting level of interrupts */
	struct exc_scratch_frame ci_escratch;	/* exception scratch area */
	struct tlb_scratch_frame ci_tscratch;	/* TLB miss scratch area */
};

static inline struct cpu_info *
curcpu(void)
{
	struct cpu_info *ci;
	__asm volatile("getcon kcr0, %0" : "=r"(ci));
	return (ci);
}

#define	curlwp		curcpu()->ci_curlwp
#define	curpcb		curcpu()->ci_curpcb

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_number()			0
#define	cpu_proc_fork(p1, p2)		/* nothing */

/*
 * Can swapout u-area
 */
#define	cpu_swapin(p)			/* nothing */
#define	cpu_swapout(p)			/* nothing */

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.
 */
struct clockframe {
	struct stateframe	cf_state;
};

#define	CLKF_USERMODE(cf)  (((cf)->cf_state.sf_ssr&SH5_CONREG_SR_MD)==0)
#define	CLKF_BASEPRI(cf)   (((cf)->cf_state.sf_ssr&SH5_CONREG_SR_IMASK_ALL)==0)
#define	CLKF_PC(cf)        ((cf)->cf_state.sf_spc)

#define	CLKF_INTR(cf)      (curcpu()->ci_intr_depth > 1)

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	PROC_PC(p)							\
	(((struct trapframe *)(p)->p_md.md_regs)->tf_state.sf_spc)

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched(ci)						\
do {									\
	(ci)->ci_want_resched = 1;					\
	if (curproc != NULL)						\
		aston(curproc);						\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the MIPS, request an ast to send us
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
 * Schedule an AST for the specified process
 */
#define	aston(p)	((p)->p_md.md_astpending = 1)

/*
 * We need a machine-independent name for this.
 */
#define	DELAY(x)		delay(x)
#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_MAXID		3	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES {						\
	{ 0, 0 },							\
	{ "console_device",	CTLTYPE_STRUCT },			\
}

#ifdef _KERNEL
extern void delay(u_int);
extern u_int _sh5_delay_constant;
extern u_int _sh5_ctc_ticks_per_us;

struct pcb;
extern void savectx(struct pcb *);
extern void dumpsys(void);

extern void cpu_identify(void);
extern void sh5_reboot(int, char *);
#endif /* _KERNEL */

#endif /* _SH5_CPU_H_ */
