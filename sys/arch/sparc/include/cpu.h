/*	$NetBSD: cpu.h,v 1.84.14.1 2010/01/09 01:43:51 snj Exp $ */

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

#ifdef _KERNEL
/*
 * Exported definitions unique to SPARC cpu support.
 */

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_sparc_arch.h"
#endif

#include <machine/intr.h>
#include <machine/psl.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/intreg.h>

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	curcpu()		(cpuinfo.ci_self)
#define	curlwp			(cpuinfo.ci_curlwp)
#define	CPU_IS_PRIMARY(ci)	((ci)->master)

#define	cpu_swapin(p)		/* nothing */
#define	cpu_swapout(p)		/* nothing */
#define	cpu_number()		(cpuinfo.ci_cpuid)
void	cpu_proc_fork(struct proc *, struct proc *);

#if defined(MULTIPROCESSOR)
void	cpu_boot_secondary_processors(void);
#endif

/*
 * Arguments to hardclock, softclock and statclock encapsulate the
 * previous machine state in an opaque clockframe.  The ipl is here
 * as well for strayintr (see locore.s:interrupt and intr.c:strayintr).
 * Note that CLKF_INTR is valid only if CLKF_USERMODE is false.
 */
struct clockframe {
	u_int	psr;		/* psr before interrupt, excluding PSR_ET */
	u_int	pc;		/* pc at interrupt */
	u_int	npc;		/* npc at interrupt */
	u_int	ipl;		/* actual interrupt priority level */
	u_int	fp;		/* %fp at interrupt */
};
typedef struct clockframe clockframe;

extern int eintstack[];

#define	CLKF_USERMODE(framep)	(((framep)->psr & PSR_PS) == 0)
#define	CLKF_LOPRI(framep,n)	(((framep)->psr & PSR_PIL) < (n) << 8)
#define	CLKF_PC(framep)		((framep)->pc)
#if defined(MULTIPROCESSOR)
#define	CLKF_INTR(framep)						\
	((framep)->fp > (u_int)cpuinfo.eintstack - INT_STACK_SIZE &&	\
	 (framep)->fp < (u_int)cpuinfo.eintstack)
#else
#define	CLKF_INTR(framep)	((framep)->fp < (u_int)eintstack)
#endif

void	sparc_softintr_init(void);

/*
 * Preempt the current process on the target CPU if in interrupt from
 * user mode, or after the current trap/syscall if in system mode.
 */
#define cpu_need_resched(ci, flags) do {				\
	(ci)->ci_want_resched = 1;					\
	(ci)->ci_want_ast = 1;						\
									\
	/* Just interrupt the target CPU, so it can notice its AST */	\
	if (((flags) & RESCHED_IMMED) || (ci)->ci_cpuid != cpu_number()) \
		XCALL0(sparc_noop, 1U << (ci)->ci_cpuid);		\
} while (/*CONSTCOND*/0)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the sparc, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, cpuinfo.ci_want_ast = 1)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define cpu_signotify(l) do {						\
	struct cpu_info *_ci = (l)->l_cpu;				\
	_ci->ci_want_ast = 1;						\
									\
	/* Just interrupt the target CPU, so it can notice its AST */	\
	if (_ci->ci_cpuid != cpu_number())				\
		XCALL0(sparc_noop, 1U << _ci->ci_cpuid);		\
} while (/*CONSTCOND*/0)

/* CPU architecture version */
extern int cpu_arch;

/* Number of CPUs in the system */
extern int sparc_ncpus;

/*
 * Interrupt handler chains.  Interrupt handlers should return 0 for
 * ``not me'' or 1 (``I took care of it'').  intr_establish() inserts a
 * handler into the list.  The handler is called with its (single)
 * argument, or with a pointer to a clockframe if ih_arg is NULL.
 */
extern struct intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
	struct	intrhand *ih_next;
	int	ih_classipl;
} *intrhand[15];

void	intr_establish(int, int, struct intrhand *, void (*)(void));
void	intr_disestablish(int, struct intrhand *);

void	intr_lock_kernel(void);
void	intr_unlock_kernel(void);

/* disksubr.c */
struct dkbad;
int isbad(struct dkbad *, int, int, int);

/* machdep.c */
int	ldcontrolb(void *);
void	dumpconf(void);
void *	reserve_dumppages(void *);
void	wcopy(const void *, void *, u_int);
void	wzero(void *, u_int);

/* clock.c */
struct timeval;
void	lo_microtime(struct timeval *);
void	schedintr(void *);

/* locore.s */
struct fpstate;
void	savefpstate(struct fpstate *);
void	loadfpstate(struct fpstate *);
int	probeget(void *, int);
void	write_all_windows(void);
void	write_user_windows(void);
void 	lwp_trampoline(void);
void 	lwp_setfunc_trampoline(void);
struct pcb;
void	snapshot(struct pcb *);
struct frame *getfp(void);
int	xldcontrolb(void *, struct pcb *);
void	copywords(const void *, void *, size_t);
void	qcopy(const void *, void *, size_t);
void	qzero(void *, size_t);

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
struct trapframe;
int fixalign(struct lwp *, struct trapframe *);
int emulinstr(int, struct trapframe *);

/* cpu.c */
void mp_pause_cpus(void);
void mp_resume_cpus(void);
void mp_halt_cpus(void);
#ifdef DDB
void mp_pause_cpus_ddb(void);
void mp_resume_cpus_ddb(void);
#endif

/* intr.c */
u_int setitr(u_int);
u_int getitr(void);


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
	int	tv_instr[4];		/* the four instructions */
};

extern struct trapvec *trapbase;	/* the 256 vectors */

#endif /* _KERNEL */
#endif /* _CPU_H_ */
