/*      $NetBSD: cpu.h,v 1.51 2000/06/10 14:59:39 ragge Exp $      */

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
#include "opt_lockdebug.h"
#endif

#ifdef _KERNEL

#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/sched.h>

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
struct cpu_info;

struct	cpu_dep {
	void	(*cpu_steal_pages)(void); /* pmap init before mm is on */
	int	(*cpu_mchk)(caddr_t);   /* Machine check handling */
	void	(*cpu_memerr)(void); /* Memory subsystem errors */
	    /* Autoconfiguration */
	void	(*cpu_conf)(void);
	int	(*cpu_clkread)(time_t);	/* Read cpu clock time */
	void	(*cpu_clkwrite)(void);	/* Write system time to cpu */
	short	cpu_vups;	/* speed of cpu */
	short	cpu_scbsz;	/* (estimated) size of system control block */
	void	(*cpu_halt)(void); /* Cpu dependent halt call */
	void	(*cpu_reboot)(int); /* Cpu dependent reboot call */
	void	(*cpu_clrf)(void); /* Clear cold/warm start flags */
	void	(*cpu_subconf)(struct device *);/*config cpu dep. devs */
	int     cpu_flags;
#if defined(MULTIPROCESSOR)
	/* Kick off slave cpu */
	void	(*cpu_startslave)(struct device *, struct cpu_info *);
#endif
};
  
#define	CPU_RAISEIPL	1	/* Must raise IPL until intr is handled */ 

extern struct cpu_dep *dep_call; /* Holds pointer to current CPU struct. */

struct clockframe {
        int     pc;
        int     ps;
};

struct cpu_info {
	/*
	 * Public members.
	 */
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;           /* # of spin locks held */
	u_long ci_simple_locks;         /* # of simple locks held */
#endif

	struct proc *ci_curproc;        /* current owner of the processor */

	/*
	 * Private members.
	 */
	int ci_want_resched;		/* Should change process */
	int ci_cpunumber;		/* Index in cpu_cd.cd_devs[] array */
	long ci_exit;			/* Page to use while exiting */
#if defined(MULTIPROCESSOR)
	struct pcb *ci_pcb;		/* Idle PCB for this CPU */
	vaddr_t ci_istack;		/* Interrupt stack location */
	int ci_flags;			/* See below */
#endif
};
#define	CI_MASTERCPU	1		/* Set if master CPU */
#define	CI_RUNNING	2		/* Set when a slave CPU is running */

#define	curcpu() ((struct cpu_info *)mfpr(PR_SSP))
#define	curproc	(curcpu()->ci_curproc)
#define	cpu_number() (curcpu()->ci_cpunumber)
#define	need_resched() {curcpu()->ci_want_resched++; mtpr(AST_OK,PR_ASTLVL); }
#if defined(MULTIPROCESSOR)
#define	CPU_IS_PRIMARY(ci)	(ci->ci_flags & CI_MASTERCPU)

extern char tramp;
#endif

extern int mastercpu;

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
#if defined(MULTIPROCESSOR)
struct	cpu_info *cpu_slavesetup(struct device *);
void	cpu_boot_secondary_processors(void);
#endif
int	badaddr(caddr_t, int);
void	cpu_swapin(struct proc *);
void	dumpconf(void);
void	dumpsys(void);
void	swapconf(void);
void	disk_printtype(int, int);
void	disk_reallymapin(struct buf *, struct pte *, int, int);
vaddr_t	vax_map_physmem(paddr_t, int);
void	vax_unmap_physmem(vaddr_t, int);
void	ioaccess(vaddr_t, paddr_t, int);
void	iounaccess(vaddr_t, int);
void	findcpu(void);
void	child_return(void *);
#ifdef DDB
int	kdbrint(int);
#endif
#endif /* _KERNEL */
#ifdef _STANDALONE
void	findcpu(void);
#endif
#endif /* _VAX_CPU_H_ */
