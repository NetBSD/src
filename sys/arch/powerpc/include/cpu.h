/*	$NetBSD: cpu.h,v 1.37 2003/09/03 21:33:31 matt Exp $	*/

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

struct cache_info {
	int dcache_size;
	int dcache_line_size;
	int icache_size;
	int icache_line_size;
};

#ifdef _KERNEL
#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#endif

#include <machine/frame.h>
#include <machine/psl.h>
#include <machine/intr.h>
#include <sys/device.h>

#include <sys/sched.h>
#include <dev/sysmon/sysmonvar.h>

struct cpu_info {
	struct schedstate_percpu ci_schedstate; /* scheduler state */
	struct device *ci_dev;		/* device of corresponding cpu */
	struct lwp *ci_curlwp;		/* current owner of the processor */

	struct pcb *ci_curpcb;
	struct lwp *ci_fpulwp;
	struct lwp *ci_veclwp;
	struct pcb *ci_idle_pcb;	/* PA of our idle pcb */
	int ci_cpuid;

	volatile int ci_astpending;
	int ci_want_resched;
	volatile u_long ci_lasttb;
	volatile int ci_tickspending;
	int ci_cpl;
	int ci_iactive;
	int ci_ipending;
	int ci_intrdepth;
	char *ci_intstk;
#define	CPUSAVE_LEN	8
	register_t ci_tempsave[CPUSAVE_LEN];
	register_t ci_ddbsave[CPUSAVE_LEN];
	register_t ci_ipkdbsave[CPUSAVE_LEN];
#define	CPUSAVE_R28	0		/* where r28 gets saved */
#define	CPUSAVE_R29	1		/* where r29 gets saved */
#define	CPUSAVE_R30	2		/* where r30 gets saved */
#define	CPUSAVE_R31	3		/* where r31 gets saved */
#define	CPUSAVE_DAR	4		/* where SPR_DAR gets saved */
#define	CPUSAVE_DSISR	5		/* where SPR_DSISR gets saved */
#define	CPUSAVE_SRR0	6		/* where SRR0 gets saved */
#define	CPUSAVE_SRR1	7		/* where SRR1 gets saved */
#define	DISISAVE_LEN	4
	register_t ci_disisave[DISISAVE_LEN];
	struct cache_info ci_ci;		
	struct sysmon_envsys ci_sysmon;
	struct envsys_tre_data ci_tau_info;
	struct evcnt ci_ev_clock;	/* clock intrs */
	struct evcnt ci_ev_softclock;	/* softclock intrs */
	struct evcnt ci_ev_softnet;	/* softnet intrs */
	struct evcnt ci_ev_softserial;	/* softserial intrs */
	struct evcnt ci_ev_traps;	/* calls to trap() */
	struct evcnt ci_ev_kdsi;	/* kernel DSI traps */
	struct evcnt ci_ev_udsi;	/* user DSI traps */
	struct evcnt ci_ev_udsi_fatal;	/* user DSI trap failures */
	struct evcnt ci_ev_kisi;	/* kernel ISI traps */
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
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif
};

#ifdef MULTIPROCESSOR
static __inline int
cpu_number(void)
{
	int pir;

	__asm ("mfspr %0,1023" : "=r"(pir));
	return pir;
}

void	cpu_boot_secondary_processors(void);


#define CPU_IS_PRIMARY(ci)	((ci)->ci_cpuid == 0)
#define CPU_INFO_ITERATOR		int
#define CPU_INFO_FOREACH(cii, ci)					\
	cii = 0, ci = &cpu_info[0]; cii < CPU_MAXNUM; cii++, ci++

#else

#define cpu_number()		0

#define CPU_INFO_ITERATOR		int
#define CPU_INFO_FOREACH(cii, ci)					\
	cii = 0, ci = curcpu(); ci != NULL; ci = NULL

#endif /* MULTIPROCESSOR */

extern struct cpu_info cpu_info[];

static __inline struct cpu_info *
curcpu(void)
{
	struct cpu_info *ci;

	__asm __volatile ("mfsprg %0,0" : "=r"(ci));
	return ci;
}

#define curlwp			(curcpu()->ci_curlwp)
#define curpcb			(curcpu()->ci_curpcb)

static __inline register_t
mfmsr(void)
{
	register_t msr;

	__asm __volatile ("mfmsr %0" : "=r"(msr));
	return msr;
}

static __inline void
mtmsr(register_t msr)
{

	__asm __volatile ("mtmsr %0" : : "r"(msr));
}

static __inline uint32_t
mftbl(void)
{
	uint32_t tbl;

	__asm __volatile (
#ifdef PPC_IBM403
"	mftblo %0	\n"
#else
"	mftbl %0	\n"
#endif
	: "=r" (tbl));

	return tbl;
}

static __inline uint64_t
mftb(void)
{
	uint64_t tb;

#ifdef _LP64
	__asm __volatile ("mftb %0" : "=r"(tb));
#else
	int tmp;

	__asm __volatile (
#ifdef PPC_IBM403
"1:	mftbhi %0	\n"
"	mftblo %0+1	\n"
"	mftbhi %1	\n"
#else
"1:	mftbu %0	\n"
"	mftb %0+1	\n"
"	mftbu %1	\n"
#endif
"	cmplw %0,%1	\n"
"	bne- 1b		\n"
	: "=r" (tb), "=r"(tmp) :: "cr0");
#endif

	return tb;
}

static __inline uint32_t
mfrtcl(void)
{
	uint32_t rtcl;

	__asm __volatile ("mfrtcl %0" : "=r"(rtcl));
	return rtcl;
}

static __inline void
mfrtc(uint32_t *rtcp)
{
	uint32_t tmp;

	__asm __volatile (
"1:	mfrtcu	%0	\n"
"	mfrtcl	%1	\n"
"	mfrtcu	%2	\n"
"	cmplw	%0,%2	\n"
"	bne-	1b"
	    : "=r"(*rtcp), "=r"(*(rtcp + 1)), "=r"(tmp));
}

static __inline uint32_t
mfpvr(void)
{
	uint32_t pvr;

	__asm __volatile ("mfpvr %0" : "=r"(pvr));
	return (pvr);
}

/*
 * CLKF_BASEPRI is dependent on the underlying interrupt code
 * and can not be defined here.  It should be defined in
 * <machine/intr.h>
 */
#define	CLKF_USERMODE(frame)	(((frame)->srr1 & PSL_PR) != 0)
#define	CLKF_PC(frame)		((frame)->srr0)
#define	CLKF_INTR(frame)	((frame)->depth > 0)

#define	LWP_PC(l)		(trapframe(l)->srr0)

#define	cpu_swapout(p)
#define cpu_wait(p)
#define	cpu_proc_fork(p1, p2)

extern int powersave;
extern int cpu_timebase;
extern int cpu_printfataltraps;
extern char cpu_model[];

struct cpu_info *cpu_attach_common(struct device *, int);
void cpu_setup(struct device *, struct cpu_info *);
void cpu_identify(char *, size_t);
void delay (unsigned int);
void cpu_probe_cache(void);
void dcache_flush_page(vaddr_t);
void icache_flush_page(vaddr_t);
void dcache_flush(vaddr_t, vsize_t);
void icache_flush(vaddr_t, vsize_t);
void *mapiodev(paddr_t, psize_t);

#define	DELAY(n)		delay(n)

#define	need_resched(ci)	(ci->ci_want_resched = 1, ci->ci_astpending = 1)
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, curcpu()->ci_astpending = 1)
#define	signotify(p)		(curcpu()->ci_astpending = 1)

#ifdef PPC_OEA
void oea_init(void (*)(void));
void oea_startup(const char *);
void oea_dumpsys(void);
void oea_install_extint(void (*)(void));
paddr_t kvtop(caddr_t); 
void softnet(int);

extern paddr_t msgbuf_paddr;
extern int cpu_altivec;
#endif

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_STANDALONE)
#if !defined(CACHELINESIZE)
#ifdef PPC_IBM403
#define	CACHELINESIZE	16
#else
#define	CACHELINESIZE	32
#endif
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
#define	CPU_POWERSAVE		8
#define	CPU_MAXID		9

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "cachelinesize", CTLTYPE_INT }, \
	{ "timebase", CTLTYPE_INT }, \
	{ "cputempature", CTLTYPE_INT }, \
	{ "printfataltraps", CTLTYPE_INT }, \
	{ "cacheinfo", CTLTYPE_STRUCT }, \
	{ "altivec", CTLTYPE_INT }, \
	{ "model", CTLTYPE_STRING }, \
	{ "powersave", CTLTYPE_INT }, \
}

#endif	/* _POWERPC_CPU_H_ */
