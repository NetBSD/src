/*	$NetBSD: cpu.h,v 1.68.2.3 2011/05/31 03:04:14 rmind Exp $	*/

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

#if defined(_KERNEL) || defined(_KMEMUSER)
#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#endif

#ifdef _KERNEL
#include <machine/frame.h>
#include <machine/psl.h>
#include <machine/intr.h>
#include <sys/device_if.h>
#include <sys/evcnt.h>
#endif

#include <sys/cpu_data.h>

struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
#ifdef _KERNEL
	device_t ci_dev;		/* device of corresponding cpu */
	struct cpu_softc *ci_softc;	/* private cpu info */
	struct lwp *ci_curlwp;		/* current owner of the processor */

	struct pcb *ci_curpcb;
	struct pmap *ci_curpm;
	struct lwp * volatile ci_fpulwp;
	struct lwp * volatile ci_veclwp;
	int ci_cpuid;

	volatile int ci_astpending;
	int ci_want_resched;
	volatile uint64_t ci_lastintr;
	volatile u_long ci_lasttb;
	volatile int ci_tickspending;
	volatile int ci_cpl;
	volatile int ci_iactive;
	volatile int ci_idepth;
#ifndef PPC_BOOKE
	volatile imask_t ci_ipending;
#endif
	int ci_mtx_oldspl;
	int ci_mtx_count;
#ifdef PPC_IBM4XX
	char *ci_intstk;
#endif
#ifndef PPC_BOOKE
#define	CPUSAVE_LEN	8
	register_t ci_tempsave[CPUSAVE_LEN];
	register_t ci_ddbsave[CPUSAVE_LEN];
	register_t ci_ipkdbsave[CPUSAVE_LEN];
#define	CPUSAVE_R28	0		/* where r28 gets saved */
#define	CPUSAVE_R29	1		/* where r29 gets saved */
#define	CPUSAVE_R30	2		/* where r30 gets saved */
#define	CPUSAVE_R31	3		/* where r31 gets saved */
#if defined(PPC_IBM4XX)
#define	CPUSAVE_DEAR	4		/* where SPR_DEAR gets saved */
#define	CPUSAVE_ESR	5		/* where SPR_ESR gets saved */
	register_t ci_tlbmisssave[CPUSAVE_LEN];
#else
#define	CPUSAVE_DAR	4		/* where SPR_DAR gets saved */
#define	CPUSAVE_DSISR	5		/* where SPR_DSISR gets saved */
#define	DISISAVE_LEN	4
	register_t ci_disisave[DISISAVE_LEN];
#endif
#define	CPUSAVE_SRR0	6		/* where SRR0 gets saved */
#define	CPUSAVE_SRR1	7		/* where SRR1 gets saved */
#else /* PPC_BOOKE */
#define	CPUSAVE_LEN	128
	register_t ci_savelifo[CPUSAVE_LEN];
	struct pmap_segtab *ci_pmap_segtabs[2];
#define	ci_pmap_kern_segtab	ci_pmap_segtabs[0]
#define	ci_pmap_user_segtab	ci_pmap_segtabs[1]
	struct pmap_tlb_info *ci_tlb_info;
#endif /* PPC_BOOKE */
	struct cache_info ci_ci;		
	void *ci_sysmon_cookie;
	void (*ci_idlespin)(void);
	uint32_t ci_khz;
	struct evcnt ci_ev_clock;	/* clock intrs */
	struct evcnt ci_ev_statclock; 	/* stat clock */
#ifndef PPC_BOOKE
	struct evcnt ci_ev_softclock;	/* softclock intrs */
	struct evcnt ci_ev_softnet;	/* softnet intrs */
	struct evcnt ci_ev_softserial;	/* softserial intrs */
#endif
	struct evcnt ci_ev_traps;	/* calls to trap() */
	struct evcnt ci_ev_kdsi;	/* kernel DSI traps */
	struct evcnt ci_ev_udsi;	/* user DSI traps */
	struct evcnt ci_ev_udsi_fatal;	/* user DSI trap failures */
	struct evcnt ci_ev_kisi;	/* kernel ISI traps */
	struct evcnt ci_ev_isi;		/* user ISI traps */
	struct evcnt ci_ev_isi_fatal;	/* user ISI trap failures */
	struct evcnt ci_ev_pgm;		/* user PGM traps */
	struct evcnt ci_ev_debug;	/* user debug traps */
	struct evcnt ci_ev_fpu;		/* FPU traps */
	struct evcnt ci_ev_fpusw;	/* FPU context switch */
	struct evcnt ci_ev_ali;		/* Alignment traps */
	struct evcnt ci_ev_ali_fatal;	/* Alignment fatal trap */
	struct evcnt ci_ev_scalls;	/* system call traps */
	struct evcnt ci_ev_vec;		/* Altivec traps */
	struct evcnt ci_ev_vecsw;	/* Altivec context switches */
	struct evcnt ci_ev_umchk;	/* user MCHK events */
	struct evcnt ci_ev_ipi;		/* IPIs received */
	struct evcnt ci_ev_tlbmiss_soft; /* tlb miss (no trap) */
	struct evcnt ci_ev_dtlbmiss_hard; /* data tlb miss (trap) */
	struct evcnt ci_ev_itlbmiss_hard; /* instruction tlb miss (trap) */
#endif /* _KERNEL */
};
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#ifdef MULTIPROCESSOR

struct cpu_hatch_data {
	struct device *self;
	struct cpu_info *ci;
	int running;
	int pir;
	int asr;
	int hid0;
	int sdr1;
	int sr[16];
	int batu[4], batl[4];
	int tbu, tbl;
};

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
	cii = 0, ci = &cpu_info[0]; cii < ncpu; cii++, ci++

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

	__asm volatile ("mfsprg %0,0" : "=r"(ci));
	return ci;
}

#define curlwp			(curcpu()->ci_curlwp)
#define curpcb			(curcpu()->ci_curpcb)
#define curpm			(curcpu()->ci_curpm)

static __inline register_t
mfmsr(void)
{
	register_t msr;

	__asm volatile ("mfmsr %0" : "=r"(msr));
	return msr;
}

static __inline void
mtmsr(register_t msr)
{
	//KASSERT(msr & PSL_CE);
	//KASSERT(msr & PSL_DE);
	__asm volatile ("mtmsr %0" : : "r"(msr));
}

static __inline uint32_t
mftbl(void)
{
	uint32_t tbl;

	__asm volatile (
#ifdef PPC_IBM403
	"	mftblo %[tbl]"		"\n"
#elif defined(PPC_BOOKE)
	"	mfspr %[tbl],268"	"\n"
#else
	"	mftbl %[tbl]"		"\n"
#endif
	: [tbl] "=r" (tbl));

	return tbl;
}

static __inline uint64_t
mftb(void)
{
	uint64_t tb;

#ifdef _LP64
	__asm volatile ("mftb %0" : "=r"(tb));
#else
	int tmp;

	__asm volatile (
#ifdef PPC_IBM403
	"1:	mftbhi %[tb]"		"\n"
	"	mftblo %L[tb]"		"\n"
	"	mftbhi %[tmp]"		"\n"
#elif defined(PPC_BOOKE)
	"1:	mfspr %[tb],269"	"\n"
	"	mfspr %L[tb],268"	"\n"
	"	mfspr %[tmp],269"	"\n"
#else
	"1:	mftbu %[tb]"		"\n"
	"	mftb %L[tb]"		"\n"
	"	mftbu %[tmp]"		"\n"
#endif
	"	cmplw %[tb],%[tmp]"	"\n"
	"	bne- 1b"		"\n"
	    : [tb] "=r" (tb), [tmp] "=r"(tmp)
	    :: "cr0");
#endif

	return tb;
}

static __inline uint32_t
mfrtcl(void)
{
	uint32_t rtcl;

	__asm volatile ("mfrtcl %0" : "=r"(rtcl));
	return rtcl;
}

static __inline void
mfrtc(uint32_t *rtcp)
{
	uint32_t tmp;

	__asm volatile (
	"1:	mfrtcu	%[rtcu]"	"\n"
	"	mfrtcl	%[rtcl]"	"\n"
	"	mfrtcu	%[tmp]"		"\n"
	"	cmplw	%[rtcu],%[tmp]"	"\n"
	"	bne-	1b"
	    : [rtcu] "=r"(rtcp[0]), [rtcl] "=r"(rtcp[1]), [tmp] "=r"(tmp)
	    :: "cr0");
}

static __inline uint32_t
mfpvr(void)
{
	uint32_t pvr;

	__asm volatile ("mfpvr %0" : "=r"(pvr));
	return (pvr);
}

static __inline int
cntlzw(uint32_t val)
{
	int 			cnt;

	__asm volatile ("cntlzw %0,%1" : "=r"(cnt) : "r"(val));
	return (cnt);
}

/*
 * functions to access the G3's cache throttling register
 * bits 1 - 9 specify additional waits on cache acess
 * bit 0 enables cache throttling
 */

static __inline int
mfictc(void)
{
	int reg;

	__asm ("mfspr %0,1019" : "=r"(reg));
	return reg;
}

static __inline void
mtictc(uint32_t reg)
{

	__asm ("mtspr 1019,%0" :: "r"(reg));
}

#define	CLKF_USERMODE(frame)	(((frame)->cf_srr1 & PSL_PR) != 0)
#define	CLKF_PC(frame)		((frame)->cf_srr0)
#define	CLKF_INTR(frame)	((frame)->cf_idepth > 0)

#define	LWP_PC(l)		(trapframe(l)->tf_srr0)

#define	cpu_proc_fork(p1, p2)

extern int powersave;
extern int cpu_timebase;
extern int cpu_printfataltraps;
extern char cpu_model[];

void cpu_uarea_remap(struct lwp *);
struct cpu_info *cpu_attach_common(struct device *, int);
void cpu_setup(struct device *, struct cpu_info *);
void cpu_identify(char *, size_t);
int cpu_get_dfs(void);
void cpu_set_dfs(int);
void delay (unsigned int);
void cpu_probe_cache(void);
#ifndef PPC_BOOKE
void dcache_flush_page(vaddr_t);
void icache_flush_page(vaddr_t);
void dcache_flush(vaddr_t, vsize_t);
void icache_flush(vaddr_t, vsize_t);
#else
void dcache_wb_page(vaddr_t);
void dcache_wbinv_page(vaddr_t);
void dcache_inv_page(vaddr_t);
void dcache_zero_page(vaddr_t);
void icache_inv_page(vaddr_t);
void dcache_wb(vaddr_t, vsize_t);
void dcache_wbinv(vaddr_t, vsize_t);
void dcache_inv(vaddr_t, vsize_t);
void icache_inv(vaddr_t, vsize_t);
#endif
void *mapiodev(paddr_t, psize_t);
void unmapiodev(vaddr_t, vsize_t);

#ifdef MULTIPROCESSOR
int md_setup_trampoline(volatile struct cpu_hatch_data *, struct cpu_info *);
void md_presync_timebase(volatile struct cpu_hatch_data *);
void md_start_timebase(volatile struct cpu_hatch_data *);
void md_sync_timebase(volatile struct cpu_hatch_data *);
void md_setup_interrupts(void);
int cpu_spinup(struct device *, struct cpu_info *);
register_t cpu_hatch(void);
void cpu_spinup_trampoline(void);
#endif

#define	DELAY(n)		delay(n)

#define	cpu_need_resched(ci, v)	(ci->ci_want_resched = ci->ci_astpending = 1)
#define	cpu_did_resched(l)	((void)(curcpu()->ci_want_resched = 0))
#define	cpu_need_proftick(l)	((l)->l_pflag |= LP_OWEUPC, curcpu()->ci_astpending = 1)
#define	cpu_signotify(l)	(curcpu()->ci_astpending = 1)	/* XXXSMP */

#if !defined(PPC_IBM4XX) && !defined(PPC_BOOKE)
void oea_init(void (*)(void));
void oea_startup(const char *);
void oea_dumpsys(void);
void oea_install_extint(void (*)(void));
paddr_t kvtop(void *); 
void softnet(int);

extern paddr_t msgbuf_paddr;
extern int cpu_altivec;
#endif

#endif /* _KERNEL */

/* XXX The below breaks unified pmap on ppc32 */

#if defined(_KERNEL) || defined(_STANDALONE)
#if !defined(CACHELINESIZE)
#ifdef PPC_IBM403
#define	CACHELINESIZE		16
#define MAXCACHELINESIZE	16
#else
#if defined (PPC_OEA64_BRIDGE)
#define	CACHELINESIZE		128
#define MAXCACHELINESIZE	128
#else
#define	CACHELINESIZE		32
#define MAXCACHELINESIZE	32
#endif /* PPC_OEA64_BRIDGE */
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
#define	CPU_POWERSAVE		8	/* int: use CPU powersave mode */
#define	CPU_BOOTED_DEVICE	9	/* string: device we booted from */
#define	CPU_BOOTED_KERNEL	10	/* string: kernel we booted */
#define	CPU_MAXID		11	/* number of valid machdep ids */

#endif	/* _POWERPC_CPU_H_ */
