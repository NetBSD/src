/*	$NetBSD: cpu.h,v 1.10 2001/04/20 18:08:49 matt Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpu.h
 *
 * CPU specific symbols
 *
 * Created      : 18/09/94
 *
 * Based on kate/katelib/arm6.h
 */

#ifndef _ARM32_CPU_H_
#define _ARM32_CPU_H_

/*
 * User-visible definitions
 */

/*  CTL_MACHDEP definitions. */
#define	CPU_DEBUG		1	/* int: misc kernel debug control */
#define	CPU_BOOTED_DEVICE	2	/* string: device we booted from */
#define	CPU_BOOTED_KERNEL	3	/* string: kernel we booted */
#define	CPU_CONSDEV		4	/* struct: dev_t of our console */
#define	CPU_MAXID		5	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "debug", CTLTYPE_INT }, \
	{ "booted_device", CTLTYPE_STRING }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "console_device", CTLTYPE_STRUCT }, \
}    

#ifdef _KERNEL

/*
 * Kernel-only definitions
 */

#ifndef _LKM
#include "opt_cputypes.h"
#include "opt_lockdebug.h"
#include "opt_progmode.h"

#if defined(PROG26) && defined(PROG32)
#error "26-bit and 32-bit CPU support are not compatible"
#endif
#if !defined(PROG26) && !defined(PROG32)
#error "Support for at least one CPU type must be configured into the kernel"
#endif

#ifdef CPU_ARM7500
#ifndef CPU_ARM7
#error "option CPU_ARM7 is required with CPU_ARM7500"
#endif
#ifdef CPU_ARM6
#error "CPU options CPU_ARM6 and CPU_ARM7500 are not compatible"
#endif
#ifdef CPU_ARM8
#error "CPU options CPU_ARM8 and CPU_ARM7500 are not compatible"
#endif
#ifdef CPU_SA110
#error "CPU options CPU_SA110 and CPU_ARM7500 are not compatible"
#endif
#endif /* CPU_ARM7500 */

#endif /* !_LKM */


#ifndef _LOCORE
#include <sys/user.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#endif	/* !_LOCORE */

#ifdef arm26
extern int astpending;
#define setsoftast() (astpending = 1)
#else
#include <machine/psl.h>
#endif

#include <arm/armreg.h>

#ifdef PROG32
#ifdef _LOCORE
#define IRQdisable \
	stmfd	sp!, {r0} ; \
	mrs	r0, cpsr_all ; \
	orr	r0, r0, #(I32_bit) ; \
	msr	cpsr_all, r0 ; \
	ldmfd	sp!, {r0}

#define IRQenable \
	stmfd	sp!, {r0} ; \
	mrs	r0, cpsr_all ; \
	bic	r0, r0, #(I32_bit) ; \
	msr	cpsr_all, r0 ; \
	ldmfd	sp!, {r0}		

#else
#define IRQdisable SetCPSR(I32_bit, I32_bit);
#define IRQenable SetCPSR(I32_bit, 0);
#endif	/* _LOCORE */
#endif

/* All the CLKF_* macros take a struct clockframe * as an argument. */

#ifdef PROG32
/*
 * Return TRUE/FALSE (1/0) depending on whether the frame came from USR
 * mode or not.
 */
 
#define CLKF_USERMODE(frame) ((frame->if_spsr & PSR_MODE) == PSR_USR32_MODE)

/*
 * This needs straighening, prob is the frame does not have info on the
 * priority a guess that needs trying is (current_spl_level == SPL0)
 */

#define CLKF_BASEPRI(frame) ((frame->if_spsr & PSR_MODE) == PSR_USR32_MODE)

#define CLKF_PC(frame) (frame->if_pc)

/*#define CLKF_INTR(frame) (current_intr_depth > 1)*/

/* Hack to treat FPE time as interrupt time so we can measure it */
#define CLKF_INTR(frame) ((current_intr_depth > 1) || (frame->if_spsr & PSR_MODE) == PSR_UND32_MODE)

#define	PROC_PC(p)	((p)->p_addr->u_pcb.pcb_tf->tf_pc)

#elif defined(PROG26)

/* True if we took the interrupt in user mode */
#define CLKF_USERMODE(frame)	((frame->if_r15 & R15_MODE) == R15_MODE_USR)

/* True if we were at spl0 before the interrupt */
#define CLKF_BASEPRI(frame)	0	/* FIXME */

/* Extract the program counter from a clockframe */
#define CLKF_PC(frame)		(frame->if_r15 & R15_PC)

/* True if we took the interrupt from inside another interrupt handler. */
/* Non-trivial to check because we handle interrupts in SVC mode. */
#define CLKF_INTR(frame)	0	/* FIXME */

#endif

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

#ifndef _LOCORE
#include <sys/sched.h>
struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};
#ifdef _KERNEL
extern struct cpu_info cpu_info_store;
#define	curcpu()	(&cpu_info_store)
#endif /* _KERNEL */
#endif /* ! _LOCORE */

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */

#define signotify(p)            setsoftast()

#define cpu_wait(p)	/* nothing */
#define cpu_number()	0

#ifndef _LOCORE
extern int current_intr_depth;

struct device;
void	cpu_attach	__P((struct device *));

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
int	want_resched;		/* resched() was called */
#define	need_resched(ci)	(want_resched = 1, setsoftast())

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the i386, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, setsoftast())

/* locore.S */
void atomic_set_bit	__P((u_int *address, u_int setmask));
void atomic_clear_bit	__P((u_int *address, u_int clearmask));

/* cpuswitch.S */
struct pcb;
void	savectx		__P((struct pcb *pcb));

#ifndef arm26
/* ast.c */
void userret		__P((register struct proc *p));
#endif

/* machdep.h */
void bootsync		__P((void));

/* strstr.c */
char *strstr		__P((const char *s1, const char *s2));

/* syscall.c */
void child_return	__P((void *));

#endif	/* !_LOCORE */

#endif /* _KERNEL */

#endif /* !_ARM32_CPU_H_ */

/* End of cpu.h */
