/*	$NetBSD: cpu.h,v 1.53.8.1 2006/06/19 03:45:14 chap Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _CPU_H_
#define _CPU_H_

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_BOOTED_KERNEL	1	/* string: booted kernel name */
#define	CPU_BOOTED_DEVICE	2	/* string: device booted from */
#define	CPU_BOOT_ARGS		3	/* string: args booted with */
#define	CPU_ARCH		4	/* integer: cpu architecture version */
#define	CPU_MAXID		5	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES {			\
	{ 0, 0 },				\
	{ "booted_kernel", CTLTYPE_STRING },	\
	{ "booted_device", CTLTYPE_STRING },	\
	{ "boot_args", CTLTYPE_STRING },	\
	{ "cpu_arch", CTLTYPE_INT },		\
}

#ifdef _KERNEL
/*
 * Exported definitions unique to SPARC cpu support.
 */

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#endif

#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/intr.h>
#include <machine/cpuset.h>
#include <sparc64/sparc64/intreg.h>

#include <sys/cpu_data.h>
/*
 * The cpu_info structure is part of a 64KB structure mapped both the kernel
 * pmap and a single locked TTE a CPUINFO_VA for that particular processor.
 * Each processor's cpu_info is accessible at CPUINFO_VA only for that
 * processor.  Other processors can access that through an additional mapping
 * in the kernel pmap.
 *
 * The 64KB page contains:
 *
 * cpu_info
 * interrupt stack (all remaining space)
 * idle PCB
 * idle stack (STACKSPACE - sizeof(PCB))
 * 32KB TSB
 */

struct cpu_info {

	/*
	 * SPARC cpu_info structures live at two VAs: one global
	 * VA (so each CPU can access any other CPU's cpu_info)
	 * and an alias VA CPUINFO_VA which is the same on each
	 * CPU and maps to that CPU's cpu_info.  Since the alias
	 * CPUINFO_VA is how we locate our cpu_info, we have to
	 * self-reference the global VA so that we can return it
	 * in the curcpu() macro.
	 */
	struct cpu_info * volatile ci_self;

	/* Most important fields first */
	struct lwp		*ci_curlwp;
	struct pcb		*ci_cpcb;
	struct cpu_info		*ci_next;

	struct lwp		*ci_fplwp;

	void			*ci_eintstack;
	struct pcb		*ci_idle_u;

	/* Spinning up the CPU */
	void			(*ci_spinup)(void);
	void			*ci_initstack;
	paddr_t			ci_paddr;

	int			ci_number;
	int			ci_upaid;
	int			ci_cpuid;

	/* CPU PROM information. */
	u_int			ci_node;

	int			ci_flags;
	int			ci_want_ast;
	int			ci_want_resched;

	struct cpu_data		ci_data;	/* MI per-cpu data */
};

#define CPUF_PRIMARY	1

/*
 * CPU boot arguments. Used by secondary CPUs at the bootstrap time.
 */
struct cpu_bootargs {
	u_int	cb_node;	/* PROM CPU node */
	volatile int cb_flags;

	vaddr_t cb_ktext;
	paddr_t cb_ktextp;
	vaddr_t cb_ektext;

	vaddr_t cb_kdata;
	paddr_t cb_kdatap;
	vaddr_t cb_ekdata;

	paddr_t	cb_cpuinfo;

	void	*cb_initstack;
};

extern struct cpu_bootargs *cpu_args;

extern int sparc_ncpus;
extern struct cpu_info *cpus;

#define	curcpu()	(((struct cpu_info *)CPUINFO_VA)->ci_self)
#define	cpu_number()	(curcpu()->ci_number)
#define	CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

#define CPU_INFO_ITERATOR		int
#define CPU_INFO_FOREACH(cii, ci)	cii = 0, ci = cpus; ci != NULL; \
					ci = ci->ci_next

#define curlwp		curcpu()->ci_curlwp
#define fplwp		curcpu()->ci_fplwp
#define curpcb		curcpu()->ci_cpcb

#define want_ast	curcpu()->ci_want_ast
#define want_resched	curcpu()->ci_want_resched

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapin(p)	/* nothing */
#define	cpu_swapout(p)	/* nothing */
#define	cpu_wait(p)	/* nothing */
void cpu_proc_fork(struct proc *, struct proc *);

#if defined(MULTIPROCESSOR)
extern vaddr_t cpu_spinup_trampoline;

extern  char   *mp_tramp_code;
extern  u_long  mp_tramp_code_len;
extern  u_long  mp_tramp_tlb_slots;
extern  u_long  mp_tramp_func;
extern  u_long  mp_tramp_ci;

void	cpu_hatch(void);
void	cpu_boot_secondary_processors(void);
#endif

extern uint64_t cpu_clockrate[];

/*
 * Arguments to hardclock, softclock and gatherstats encapsulate the
 * previous machine state in an opaque clockframe.  The ipl is here
 * as well for strayintr (see locore.s:interrupt and intr.c:strayintr).
 * Note that CLKF_INTR is valid only if CLKF_USERMODE is false.
 */
extern int intstack[];
extern int eintstack[];
struct clockframe {
	struct trapframe64 t;
};

#define	CLKF_USERMODE(framep)	(((framep)->t.tf_tstate & TSTATE_PRIV) == 0)
/*
 * XXX Disable CLKF_BASEPRI() for now.  If we use a counter-timer for
 * the clock, the interrupt remains blocked until the interrupt handler
 * returns and we write to the clear interrupt register.  If we use 
 * %tick for the clock, we could get multiple interrupts, but the 
 * currently enabled INTR_INTERLOCK will prevent the interrupt from being
 * posted twice anyway.
 * 
 * Switching to %tick for all machines and disabling INTR_INTERLOCK
 * in locore.s would allow us to take advantage of CLKF_BASEPRI().
 */
#if 0
#define	CLKF_BASEPRI(framep)	(((framep)->t.tf_oldpil) == 0)
#else
#define	CLKF_BASEPRI(framep)	(0)
#endif
#define	CLKF_PC(framep)		((framep)->t.tf_pc)
/* Since some files in sys/kern do not know BIAS, I'm using 0x7ff here */
#define	CLKF_INTR(framep)						\
	((!CLKF_USERMODE(framep))&&					\
		(((framep)->t.tf_out[6] & 1 ) ?				\
			(((vaddr_t)(framep)->t.tf_out[6] <		\
				(vaddr_t)EINTSTACK-0x7ff) &&		\
			((vaddr_t)(framep)->t.tf_out[6] >		\
				(vaddr_t)INTSTACK-0x7ff)) :		\
			(((vaddr_t)(framep)->t.tf_out[6] <		\
				(vaddr_t)EINTSTACK) &&			\
			((vaddr_t)(framep)->t.tf_out[6] >		\
				(vaddr_t)INTSTACK))))


extern struct intrhand soft01intr, soft01net, soft01clock;

void setsoftint(void);
void setsoftnet(void);

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched(ci)	(want_resched = 1, want_ast = 1)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the sparc, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, want_ast = 1)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)		(want_ast = 1)

/*
 * Interrupt handler chains.  Interrupt handlers should return 0 for
 * ``not me'' or 1 (``I took care of it'').  intr_establish() inserts a
 * handler into the list.  The handler is called with its (single)
 * argument, or with a pointer to a clockframe if ih_arg is NULL.
 */
struct intrhand {
	int			(*ih_fun)(void *);
	void			*ih_arg;
	short			ih_number;	/* interrupt number */
						/* the H/W provides */
	char			ih_pil;		/* interrupt priority */
	struct intrhand		*ih_next;	/* global list */
	struct intrhand		*ih_pending;	/* interrupt queued */
	volatile uint64_t	*ih_map;	/* Interrupt map reg */
	volatile uint64_t	*ih_clr;	/* clear interrupt reg */
};
extern struct intrhand *intrhand[];
extern struct intrhand *intrlev[MAXINTNUM];

void	intr_establish(int level, struct intrhand *);

#define mp_pause_cpus()		sparc64_ipi_pause_cpus()
#define mp_resume_cpus()	sparc64_ipi_resume_cpus()

/* disksubr.c */
struct dkbad;
int isbad(struct dkbad *bt, int, int, int);
/* machdep.c */
caddr_t	reserve_dumppages(caddr_t);
/* clock.c */
struct timeval;
int	tickintr(void *);	/* level 10 (tick) interrupt code */
int	clockintr(void *);	/* level 10 (clock) interrupt code */
int	statintr(void *);	/* level 14 (statclock) interrupt code */
/* locore.s */
struct fpstate64;
void	savefpstate(struct fpstate64 *);
void	loadfpstate(struct fpstate64 *);
uint64_t	probeget(paddr_t, int, int);
int	probeset(paddr_t, int, int, uint64_t);

#define	 write_all_windows() __asm volatile("flushw" : : )
#define	 write_user_windows() __asm volatile("flushw" : : )

void 	proc_trampoline(void);
struct pcb;
void	snapshot(struct pcb *);
struct frame *getfp(void);
void	switchtoctx(int);
/* trap.c */
void	kill_user_windows(struct lwp *);
int	rwindow_save(struct lwp *);
/* cons.c */
int	cnrom(void);
/* zs.c */
void zsconsole(struct tty *, int, int, void (**)(struct tty *, int));
#ifdef KGDB
void zs_kgdb_init(void);
#endif
/* fb.c */
void	fb_unblank(void);
/* kgdb_stub.c */
#ifdef KGDB
void kgdb_attach(int (*)(void *), void (*)(void *, int), void *);
void kgdb_connect(int);
void kgdb_panic(void);
#endif
/* emul.c */
int	fixalign(struct lwp *, struct trapframe64 *);
int	emulinstr(vaddr_t, struct trapframe64 *);

/*
 *
 * The SPARC has a Trap Base Register (TBR) which holds the upper 20 bits
 * of the trap vector table.  The next eight bits are supplied by the
 * hardware when the trap occurs, and the bottom four bits are always
 * zero (so that we can shove up to 16 bytes of executable code---exactly
 * four instructions---into each trap vector).
 *
 * The hardware allocates half the trap vectors to hardware and half to
 * software.
 *
 * Traps have priorities assigned (lower number => higher priority).
 */

struct trapvec {
	int	tv_instr[8];		/* the eight instructions */
};
extern struct trapvec *trapbase;	/* the 256 vectors */

#endif /* _KERNEL */
#endif /* _CPU_H_ */
