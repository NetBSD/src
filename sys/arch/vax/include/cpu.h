/*      $NetBSD: cpu.h,v 1.42 2000/03/19 14:56:53 ragge Exp $      */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed at Ludd, University of Lule}
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VAX_CPU_H_
#define _VAX_CPU_H_

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_multiprocessor.h"
#endif


#ifdef _KERNEL

#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/lock.h>

#include <machine/mtpr.h>
#include <machine/pcb.h>
#include <machine/uvax.h>
#include <machine/psl.h>

#define enablertclock()
#define	cpu_wait(p)
#define	cpu_swapout(p)

/*
 * All cpu-dependent info is kept in this struct. Pointer to the
 * struct for the current cpu is set up in locore.c.
 */
struct	cpu_dep {
	void	(*cpu_steal_pages) __P((void)); /* pmap init before mm is on */
	int	(*cpu_mchk) __P((caddr_t));   /* Machine check handling */
	void	(*cpu_memerr) __P((void)); /* Memory subsystem errors */
	    /* Autoconfiguration */
	void	(*cpu_conf) __P((void));
	int	(*cpu_clkread) __P((time_t));	/* Read cpu clock time */
	void	(*cpu_clkwrite) __P((void));	/* Write system time to cpu */
	short	cpu_vups;	/* speed of cpu */
	short	cpu_scbsz;	/* (estimated) size of system control block */
	void	(*cpu_halt) __P((void)); /* Cpu dependent halt call */
	void	(*cpu_reboot) __P((int)); /* Cpu dependent reboot call */
	void	(*cpu_clrf) __P((void)); /* Clear cold/warm start flags */
	void	(*cpu_subconf) __P((struct device *));/*config cpu dep. devs */
};

extern struct cpu_dep *dep_call; /* Holds pointer to current CPU struct. */

struct clockframe {
        int     pc;
        int     ps;
};

#if defined(MULTIPROCESSOR)

struct cpu_info {
	/*
	 * Public members.
	 */
	struct proc *ci_curproc;        /* current owner of the processor */
	struct simplelock ci_slock;     /* lock on this data structure */
	cpuid_t ci_cpuid;               /* our CPU ID */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;           /* # of spin locks held */
	u_long ci_simple_locks;         /* # of simple locks held */
#endif
	/*
	 * Private members.
	 */
	int	ci_want_resched;	/* Should change process */
};

/*
 * VAX internal CPU numbering is not sequential; therefore have a separate
 * function call that returns the cpu_info struct for this CPU.
 *
 * For the master CPU (or only) this struct is allocated early in startup;
 * for other CPUs it is allocated when the CPU is found.
 */
extern	int (*vax_cpu_number)(void);
extern	struct cpu_info *(*vax_curcpu)(void);

#define	cpu_number()		(*vax_cpu_number)()
#define	curcpu()		(*vax_curcpu)()
#define	need_resched() {curcpu()->ci_want_resched++; mtpr(AST_OK,PR_ASTLVL); }
#define	cpu_boot_secondary_processors()

#else /* MULTIPROCESSOR */

extern	int     want_resched;   /* resched() was called */

#define	cpu_number()			0
#define need_resched() { want_resched++; mtpr(AST_OK,PR_ASTLVL); }

#endif /* MULTIPROCESSOR */

extern struct device *booted_from;
extern int mastercpu;
extern int bootdev;

#define	setsoftnet()	mtpr(12,PR_SIRR)
#define setsoftclock()	mtpr(8,PR_SIRR)
#define	todr()		mfpr(PR_TODR)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */

#define signotify(p)     mtpr(AST_OK,PR_ASTLVL);


/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define need_proftick(p) {(p)->p_flag |= P_OWEUPC; mtpr(AST_OK,PR_ASTLVL); }

/*
 * This defines the I/O device register space size in pages.
 */
#define	IOSPSZ	((64*1024) / VAX_NBPG)	/* 64k == 128 pages */

struct device;

/* Some low-level prototypes */
int	badaddr __P((caddr_t, int));
void	cpu_swapin __P((struct proc *));
int	hp_getdev __P((int, int, struct device **));
int	ra_getdev __P((int, int, int, struct device **));
void	dumpconf __P((void));
void	dumpsys __P((void));
void	swapconf __P((void));
void	disk_printtype __P((int, int));
void	disk_reallymapin __P((struct buf *, struct pte *, int, int));
vaddr_t	vax_map_physmem __P((paddr_t, int));
void	vax_unmap_physmem __P((vaddr_t, int));
void	ioaccess __P((vaddr_t, paddr_t, int));
void	iounaccess __P((vaddr_t, int));
void	findcpu(void);
#ifdef DDB
int	kdbrint __P((int));
#endif
#endif /* _KERNEL */
#endif /* _VAX_CPU_H_ */
