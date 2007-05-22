/*      $NetBSD: cpu.h,v 1.76.10.1 2007/05/22 17:27:40 matt Exp $      */

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

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#endif

#define	CPU_PRINTFATALTRAPS	1
#define	CPU_CONSDEV		2
#define	CPU_BOOTED_DEVICE	3
#define	CPU_BOOTED_KERNEL	4
#define CPU_MAXID		5

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "printfataltraps", CTLTYPE_INT }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "booted_device", CTLTYPE_STRING }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
}

#ifdef _KERNEL

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/cpu_data.h>

#include <machine/mtpr.h>
#include <machine/pcb.h>
#include <machine/uvax.h>
#include <machine/psl.h>

#define enablertclock()

/*
 * All cpu-dependent info is kept in this struct. Pointer to the
 * struct for the current cpu is set up in locore.c.
 */
struct cpu_info;

struct cpu_dep {
	void	(*cpu_steal_pages)(void); /* pmap init before mm is on */
	int	(*cpu_mchk)(void *);   /* Machine check handling */
	void	(*cpu_memerr)(void); /* Memory subsystem errors */
	    /* Autoconfiguration */
	void	(*cpu_conf)(void);
	int	(*cpu_gettime)(volatile struct timeval *);
						/* Read cpu clock time */
	void	(*cpu_settime)(volatile struct timeval *);
						/* Write system time to cpu */
	short	cpu_vups;	/* speed of cpu */
	short	cpu_scbsz;	/* (estimated) size of system control block */
	void	(*cpu_halt)(void); /* Cpu dependent halt call */
	void	(*cpu_reboot)(int); /* Cpu dependent reboot call */
	void	(*cpu_clrf)(void); /* Clear cold/warm start flags */
	void	(*cpu_subconf)(struct device *);/*config cpu dep. devs */
	int     cpu_flags;
	void	(*cpu_badaddr)(void); /* cpu-specific badaddr() */
};

#if defined(MULTIPROCESSOR)
/*
 * All cpu-dependent calls for multicpu systems goes here.
 */
struct cpu_mp_dep {
	void	(*cpu_startslave)(struct device *, struct cpu_info *);
	void	(*cpu_send_ipi)(struct device *dest);
	void	(*cpu_cnintr)(void);
};
/*
 * NOTE: This is not bit mask, this is bit _number_.
 */
#define	IPI_START_CNTX	1	/* Start console transmitter, proc out */
#define	IPI_SEND_CNCHAR	2	/* Write char to console, kernel printf */
#define	IPI_RUNNING	3	/* This CPU just started to run */
#define	IPI_TBIA	4	/* Flush the TLB */
#define	IPI_DDB		5	/* Jump into the DDB loop */

#define	IPI_DEST_MASTER	-1	/* Destination is mastercpu */
#define	IPI_DEST_ALL	-2	/* Broadcast */

extern struct cpu_mp_dep *mp_dep_call;
#endif /* defined(MULTIPROCESSOR) */
  
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
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct lwp *ci_curlwp;		/* current owner of the processor */
	int ci_mtx_oldspl;		/* saved spl */
	int ci_mtx_count;		/* negative count of mutexes */

	/*
	 * Private members.
	 */
	int ci_need_resched;		/* Should change process */
	struct device *ci_dev;		/* device struct for this cpu */
#if defined(MULTIPROCESSOR)
	struct pcb *ci_pcb;		/* Idle PCB for this CPU */
	vaddr_t ci_istack;		/* Interrupt stack location */
	int ci_flags;			/* See below */
	long ci_ipimsgs;		/* Sent IPI bits */
	struct trapframe *ci_ddb_regs;	/* Used by DDB */
	SIMPLEQ_ENTRY(cpu_info) ci_next; /* next cpu_info */
#endif
};
#define	CI_MASTERCPU	1		/* Set if master CPU */
#define	CI_RUNNING	2		/* Set when a slave CPU is running */
#define	CI_STOPPED	4		/* Stopped (in debugger) */

extern int cpu_printfataltraps;

#if defined(MULTIPROCESSOR)
/*
 * "helper" struct. All multicpu systems must have the first two field
 * of its cpu softc like this. (used in multicpu.c).
 */
struct cpu_mp_softc {
	struct device sc_dev;
	struct cpu_info sc_ci;
};
#endif /* defined(MULTIPROCESSOR) */

				/* XXX need to cache this in cpu_info */
#define	ci_cpuid		ci_dev->dv_unit
#define	curcpu()		((struct cpu_info *)mfpr(PR_SSP))
#define	curlwp			(curcpu()->ci_curlwp)
#define	cpu_number()		(curcpu()->ci_cpuid)
#define	cpu_need_resched(ci, flags)		\
	do {					\
		(ci)->ci_need_resched = 1;	\
		mtpr(AST_OK,PR_ASTLVL);		\
	} while (/*CONSTCOND*/ 0)
#define cpu_did_resched()	((void)(curcpu()->ci_need_resched = 0))
#define	cpu_proc_fork(x, y)	do { } while (/*CONSCOND*/0)
#define	cpu_lwp_free(l, f)	do { } while (/*CONSCOND*/0)
#define	cpu_lwp_free2(l)	do { } while (/*CONSCOND*/0)
#define	cpu_idle()		do { } while (/*CONSCOND*/0)
#if defined(MULTIPROCESSOR)
#define	CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CI_MASTERCPU)

#define	CPU_INFO_ITERATOR	int
#define	CPU_INFO_FOREACH(cii, ci)	cii = 0, ci = SIMPLEQ_FIRST(&cpus); \
					ci != NULL; \
					ci = SIMPLEQ_NEXT(ci, ci_next)

extern SIMPLEQ_HEAD(cpu_info_qh, cpu_info) cpus;
extern char vax_mp_tramp;
#endif

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */

#define cpu_signotify(l)     mtpr(AST_OK,PR_ASTLVL)


/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the hp300, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define cpu_need_proftick(l) do { (l)->l_pflag |= LP_OWEUPC; mtpr(AST_OK,PR_ASTLVL); } while (/*CONSTCOND*/ 0)

/*
 * This defines the I/O device register space size in pages.
 */
#define	IOSPSZ	((64*1024) / VAX_NBPG)	/* 64k == 128 pages */

struct device;
struct buf;
struct pte;

/* Some low-level prototypes */
#if defined(MULTIPROCESSOR)
void	cpu_slavesetup(struct device *);
void	cpu_boot_secondary_processors(void);
void	cpu_send_ipi(int, int);
void	cpu_handle_ipi(void);
#endif
int	badaddr(void *, int);
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
#ifdef DDB
int	kdbrint(int);
#endif
#endif /* _KERNEL */
#ifdef _STANDALONE
void	findcpu(void);
#endif
#endif /* _VAX_CPU_H_ */
