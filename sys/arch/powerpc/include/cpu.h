/*	$NetBSD: cpu.h,v 1.16 2002/07/05 18:45:21 matt Exp $	*/

/*
 * Copyright (C) 1999 Wolfgang Solfrank.
 * Copyright (C) 1999 TooLs GmbH.
 * Copyright (C) 1995-1997 Wolfgang Solfrank.
 * Copyright (C) 1995-1997 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_POWERPC_CPU_H_
#define	_POWERPC_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#endif

#include <sys/device.h>
#include <machine/frame.h>
#include <machine/psl.h>
#include <machine/intr.h>


struct cache_info {
	int dcache_size;
	int dcache_line_size;
	int icache_size;
	int icache_line_size;
};


#ifdef _KERNEL
#include <sys/sched.h>
#include <dev/sysmon/sysmonvar.h>

struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
	struct device *ci_dev;		/* device of corresponding cpu */
	struct proc *ci_curproc;	/* current owner of the processor */

	struct pcb *ci_curpcb;
	struct pmap *ci_curpm;
	struct proc *ci_fpuproc;
	struct proc *ci_vecproc;
	struct pcb *ci_idle_pcb;	/* PA of our idle pcb */
	int ci_cpuid;

	int ci_astpending;
	int ci_want_resched;
	u_long ci_lasttb;
	int ci_tickspending;
	int ci_cpl;
	int ci_ipending;
	int ci_intrdepth;
	char *ci_intstk;
	char *ci_spillstk;
	int ci_tempsave[8];
	int ci_ddbsave[8];
	int ci_ipkdbsave[8];
	int ci_disisave[4];
	struct cache_info ci_ci;		
	struct sysmon_envsys ci_sysmon;
	struct envsys_tre_data ci_tau_info;
	struct evcnt ci_ev_traps;	/* calls to trap() */
	struct evcnt ci_ev_kdsi;	/* kernel DSI traps */
	struct evcnt ci_ev_udsi;	/* user DSI traps */
	struct evcnt ci_ev_udsi_fatal;	/* user DSI trap failures */
	struct evcnt ci_ev_isi;		/* user ISI traps */
	struct evcnt ci_ev_isi_fatal;	/* user ISI trap failures */
	struct evcnt ci_ev_pgm;		/* user PGM traps */
	struct evcnt ci_ev_fpu;		/* FPU traps */
	struct evcnt ci_ev_fpusw;	/* FPU context switch */
	struct evcnt ci_ev_ali;		/* Alignment traps */
	struct evcnt ci_ev_ali_fatal;	/* Alignment fatal trap */
	struct evcnt ci_ev_scalls;	/* system call traps */
	struct evcnt ci_ev_vec;		/* Altivec traps */
	struct evcnt ci_ev_vecsw;	/* Altivec context switches */
	struct evcnt ci_ev_umchk;	/* user MCHK events */
};

#ifdef MULTIPROCESSOR
static __inline int
cpu_number(void)
{
	int pir;

	asm ("mfspr %0,1023" : "=r"(pir));
	return pir;
}

static __inline struct cpu_info *
curcpu(void)
{
	struct cpu_info *ci;

	asm volatile ("mfsprg %0,0" : "=r"(ci));
	return ci;
}

void	cpu_boot_secondary_processors(void);

extern struct cpu_info cpu_info[];

#define CPU_IS_PRIMARY(ci)	((ci)->ci_cpuid == 0)
#define curproc			curcpu()->ci_curproc
#define curpcb			curcpu()->ci_curpcb
#define curpm			curcpu()->ci_curpm
#define want_resched		curcpu()->ci_want_resched
#define astpending		curcpu()->ci_astpending
#define	intr_depth		curcpu()->ci_intrdepth

#else
extern struct cpu_info cpu_info_store;
extern volatile int want_resched;
extern volatile int astpending;
extern volatile int intr_depth;

#define curcpu()		(&cpu_info_store)
#define cpu_number()		0

#endif /* MULTIPROCESSOR */

#define	CLKF_USERMODE(frame)	(((frame)->srr1 & PSL_PR) != 0)
#define	CLKF_BASEPRI(frame)	((frame)->pri == 0)
#define	CLKF_PC(frame)		((frame)->srr0)
#define	CLKF_INTR(frame)	((frame)->depth > 0)

#define	PROC_PC(p)		(trapframe(p)->srr0)

#define	cpu_swapout(p)
#define cpu_wait(p)

extern int powersave;
extern int cpu_timebase;
extern int cpu_printfataltraps;
extern char cpu_model[];

struct cpu_info *cpu_attach_common(struct device *, int);
void cpu_identify(char *, size_t);
void delay (unsigned int);
void cpu_probe_cache(void);
void dcache_flush_page(vaddr_t);
void icache_flush_page(vaddr_t);
void dcache_flush(vaddr_t, vsize_t);
void icache_flush(vaddr_t, vsize_t);

#define	DELAY(n)		delay(n)

#define	need_resched(ci)	(want_resched = 1, astpending = 1)
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, astpending = 1)
#define	signotify(p)		(astpending = 1)

#ifdef PPC_MPC6XX
void mpc6xx_init(void (*)(void));
void mpc6xx_startup(const char *);
void mpc6xx_dumpsys(void);
void mpc6xx_install_extint(void (*)(void));
void *mapiodev(paddr_t, psize_t);
paddr_t kvtop(caddr_t); 
void softnet(int);

extern paddr_t msgbuf_paddr;
extern int cpu_altivec;
#endif

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_STANDALONE)
#if !defined(CACHELINESIZE)
#define	CACHELINESIZE	32
#endif
#endif

void __syncicache(void *, size_t);


/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CACHELINE		1
#define	CPU_TIMEBASE		2
#define	CPU_CPUTEMP		3
#define	CPU_PRINTFATALTRAPS	4
#define	CPU_CACHEINFO		5
#define	CPU_ALTIVEC		6
#define	CPU_MODEL		7
#define	CPU_MAXID		8

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "cachelinesize", CTLTYPE_INT }, \
	{ "timebase", CTLTYPE_INT }, \
	{ "cputempature", CTLTYPE_INT }, \
	{ "printfataltraps", CTLTYPE_INT }, \
	{ "cacheinfo", CTLTYPE_STRUCT }, \
	{ "altivec", CTLTYPE_INT }, \
	{ "model", CTLTYPE_STRING }, \
}

#endif	/* _POWERPC_CPU_H_ */
