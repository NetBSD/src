/*	$NetBSD: cpu.h,v 1.107 2013/02/28 12:44:38 macallan Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)cpu.h	8.4 (Berkeley) 1/4/94
 */

#ifndef _CPU_H_
#define _CPU_H_

#include <mips/cpuregs.h>

/*
 * Exported definitions unique to NetBSD/mips cpu support.
 */

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_cputype.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#endif

#ifndef _LOCORE
#include <sys/cpu_data.h>
#include <sys/device_if.h>
#include <sys/evcnt.h>

typedef struct cpu_watchpoint {
	register_t	cw_addr;
	register_t	cw_mask;
	uint32_t	cw_asid;
	uint32_t	cw_mode;
} cpu_watchpoint_t;
/* (abstract) mode bits */
#define CPUWATCH_WRITE	__BIT(0)
#define CPUWATCH_READ	__BIT(1)
#define CPUWATCH_EXEC	__BIT(2)
#define CPUWATCH_MASK	__BIT(3)
#define CPUWATCH_ASID	__BIT(4)
#define CPUWATCH_RWX	(CPUWATCH_EXEC|CPUWATCH_READ|CPUWATCH_WRITE)

#define CPUWATCH_MAX	8	/* max possible number of watchpoints */

u_int		  cpuwatch_discover(void);
void		  cpuwatch_free(cpu_watchpoint_t *);
cpu_watchpoint_t *cpuwatch_alloc(void);
void		  cpuwatch_set_all(void);
void		  cpuwatch_clr_all(void);
void		  cpuwatch_set(cpu_watchpoint_t *);
void		  cpuwatch_clr(cpu_watchpoint_t *);

struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct cpu_info *ci_next;	/* Next CPU in list */
	struct cpu_softc *ci_softc;	/* chip-dependent hook */
	device_t ci_dev;		/* owning device */
	cpuid_t ci_cpuid;		/* Machine-level identifier */
	u_long ci_cctr_freq;		/* cycle counter frequency */
	u_long ci_cpu_freq;		/* CPU frequency */
	u_long ci_cycles_per_hz;	/* CPU freq / hz */
	u_long ci_divisor_delay;	/* for delay/DELAY */
	u_long ci_divisor_recip;	/* unused, for obsolete microtime(9) */
	struct lwp *ci_curlwp;		/* currently running lwp */
	volatile int ci_want_resched;	/* user preemption pending */
	int ci_mtx_count;		/* negative count of held mutexes */
	int ci_mtx_oldspl;		/* saved SPL value */
	int ci_idepth;			/* hardware interrupt depth */
	int ci_cpl;			/* current [interrupt] priority level */
	uint32_t ci_next_cp0_clk_intr;	/* for hard clock intr scheduling */
	struct evcnt ci_ev_count_compare;		/* hard clock intr counter */
	struct evcnt ci_ev_count_compare_missed;	/* hard clock miss counter */
	struct lwp *ci_softlwps[SOFTINT_COUNT];
	volatile u_int ci_softints;
	struct evcnt ci_ev_fpu_loads;	/* fpu load counter */
	struct evcnt ci_ev_fpu_saves;	/* fpu save counter */
	struct evcnt ci_ev_dsp_loads;	/* dsp load counter */
	struct evcnt ci_ev_dsp_saves;	/* dsp save counter */
	struct evcnt ci_ev_tlbmisses;

	/*
	 * Per-cpu pmap information
	 */
	int ci_tlb_slot;		/* reserved tlb entry for cpu_info */
	u_int ci_pmap_asid_cur;		/* current ASID */
	struct pmap_tlb_info *ci_tlb_info; /* tlb information for this cpu */
	union segtab *ci_pmap_seg0tab;
#ifdef _LP64
	union segtab *ci_pmap_segtab;
#else
	vaddr_t ci_pmap_srcbase;	/* starting VA of ephemeral src space */
	vaddr_t ci_pmap_dstbase;	/* starting VA of ephemeral dst space */
#endif

	u_int ci_cpuwatch_count;	/* number of watchpoints on this CPU */
	cpu_watchpoint_t ci_cpuwatch_tab[CPUWATCH_MAX];

#ifdef MULTIPROCESSOR
	volatile u_long ci_flags;
	volatile uint64_t ci_request_ipis;
					/* bitmask of IPIs requested */
					/*  use on chips where hw cannot pass tag */
	uint64_t ci_active_ipis;	/* bitmask of IPIs being serviced */
	uint32_t ci_ksp_tlb_slot;	/* tlb entry for kernel stack */
	struct evcnt ci_evcnt_all_ipis;	/* aggregated IPI counter */
	struct evcnt ci_evcnt_per_ipi[NIPIS];	/* individual IPI counters*/
	struct evcnt ci_evcnt_synci_activate_rqst;
	struct evcnt ci_evcnt_synci_onproc_rqst;
	struct evcnt ci_evcnt_synci_deferred_rqst;
	struct evcnt ci_evcnt_synci_ipi_rqst;

#define	CPUF_PRIMARY	0x01		/* CPU is primary CPU */
#define	CPUF_PRESENT	0x02		/* CPU is present */
#define	CPUF_RUNNING	0x04		/* CPU is running */
#define	CPUF_PAUSED	0x08		/* CPU is paused */
#define	CPUF_USERPMAP	0x20		/* CPU has a user pmap activated */
#endif

};

#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	\
    (void)(cii), ci = &cpu_info_store; ci != NULL; ci = ci->ci_next

#endif /* !_LOCORE */
#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define CPU_CONSDEV		1	/* dev_t: console terminal device */
#define CPU_BOOTED_KERNEL	2	/* string: booted kernel name */
#define CPU_ROOT_DEVICE		3	/* string: root device name */
#define CPU_LLSC		4	/* OS/CPU supports LL/SC instruction */
#define CPU_LMMI		5	/* Loongson multimedia instructions */

/*
 * Platform can override, but note this breaks userland compatibility
 * with other mips platforms.
 */
#ifndef CPU_MAXID
#define CPU_MAXID		5	/* number of valid machdep ids */
#endif

#ifdef _KERNEL
#if defined(_MODULAR) || defined(_LKM) || defined(_STANDALONE)
/* Assume all CPU architectures are valid for LKM's and standlone progs */
#define	MIPS1		1
#define	MIPS3		1
#define	MIPS4		1
#define	MIPS32		1
#define	MIPS32R2	1
#define	MIPS64		1
#define	MIPS64R2	1
#endif

#if (MIPS1 + MIPS3 + MIPS4 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) == 0
#error at least one of MIPS1, MIPS3, MIPS4, MIPS32, MIPS32R2, MIPS64, or MIPS64RR2 must be specified
#endif

/* Shortcut for MIPS3 or above defined */
#if defined(MIPS3) || defined(MIPS4) \
    || defined(MIPS32) || defined(MIPS32R2) \
    || defined(MIPS64) || defined(MIPS64R2)

#define	MIPS3_PLUS	1
#define __HAVE_CPU_COUNTER
#else
#undef MIPS3_PLUS
#endif

/*
 * Macros to find the CPU architecture we're on at run-time,
 * or if possible, at compile-time.
 */

#define	CPU_ARCH_MIPSx		0		/* XXX unknown */
#define	CPU_ARCH_MIPS1		(1 << 0)
#define	CPU_ARCH_MIPS2		(1 << 1)
#define	CPU_ARCH_MIPS3		(1 << 2)
#define	CPU_ARCH_MIPS4		(1 << 3)
#define	CPU_ARCH_MIPS5		(1 << 4)
#define	CPU_ARCH_MIPS32		(1 << 5)
#define	CPU_ARCH_MIPS64		(1 << 6)
#define	CPU_ARCH_MIPS32R2	(1 << 7)
#define	CPU_ARCH_MIPS64R2	(1 << 8)

/* Note: must be kept in sync with -ffixed-?? Makefile.mips. */
#define MIPS_CURLWP             $24
#define MIPS_CURLWP_QUOTED      "$24"
#define MIPS_CURLWP_LABEL	_L_T8
#define MIPS_CURLWP_REG		_R_T8
#define TF_MIPS_CURLWP(x)	TF_REG_T8(x)

#ifndef _LOCORE

extern struct cpu_info cpu_info_store;
register struct lwp *mips_curlwp asm(MIPS_CURLWP_QUOTED);

#define	curlwp			mips_curlwp
#define	curcpu()		(curlwp->l_cpu)
#define	curpcb			((struct pcb *)lwp_getpcb(curlwp))
#ifdef MULTIPROCESSOR
#define	cpu_number()		(curcpu()->ci_index)
#define	CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)
#else
#define	cpu_number()		(0)
#define	CPU_IS_PRIMARY(ci)	(true)
#endif

/* XXX simonb
 * Should the following be in a cpu_info type structure?
 * And how many of these are per-cpu vs. per-system?  (Ie,
 * we can assume that all cpus have the same mmu-type, but
 * maybe not that all cpus run at the same clock speed.
 * Some SGI's apparently support R12k and R14k in the same
 * box.)
 */
struct mips_options {
	const struct pridtab *mips_cpu;

	u_int mips_cpu_arch;
	u_int mips_cpu_mhz; /* CPU speed in MHz, estimated by mc_cpuspeed(). */
	u_int mips_cpu_flags;
	u_int mips_num_tlb_entries;
	mips_prid_t mips_cpu_id;
	mips_prid_t mips_fpu_id;
	bool mips_has_r4k_mmu;
	bool mips_has_llsc;
	u_int mips3_pg_shift;
	u_int mips3_pg_cached;
	u_int mips3_cca_devmem;
#ifdef MIPS3_PLUS
#ifdef _LP64
	uint64_t mips3_xkphys_cached;
#endif
	uint64_t mips3_tlb_vpn_mask;
	uint64_t mips3_tlb_pfn_mask;
	uint32_t mips3_tlb_pg_mask;
#endif
};
extern struct mips_options mips_options;

#define	CPU_MIPS_R4K_MMU		0x0001
#define	CPU_MIPS_NO_LLSC		0x0002
#define	CPU_MIPS_CAUSE_IV		0x0004
#define	CPU_MIPS_HAVE_SPECIAL_CCA	0x0008	/* Defaults to '3' if not set. */
#define	CPU_MIPS_CACHED_CCA_MASK	0x0070
#define	CPU_MIPS_CACHED_CCA_SHIFT	 4
#define	CPU_MIPS_DOUBLE_COUNT		0x0080	/* 1 cp0 count == 2 clock cycles */
#define	CPU_MIPS_USE_WAIT		0x0100	/* Use "wait"-based cpu_idle() */
#define	CPU_MIPS_NO_WAIT		0x0200	/* Inverse of previous, for mips32/64 */
#define	CPU_MIPS_D_CACHE_COHERENT	0x0400	/* D-cache is fully coherent */
#define	CPU_MIPS_I_D_CACHE_COHERENT	0x0800	/* I-cache funcs don't need to flush the D-cache */
#define	CPU_MIPS_NO_LLADDR		0x1000
#define	CPU_MIPS_HAVE_MxCR		0x2000	/* have mfcr, mtcr insns */
#define	CPU_MIPS_LOONGSON2		0x4000
#define	MIPS_NOT_SUPP			0x8000
#define	CPU_MIPS_HAVE_DSP		0x10000

#endif	/* !_LOCORE */

#if ((MIPS1 + MIPS3 + MIPS4 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) == 1) || defined(_LOCORE)

#if defined(MIPS1)

# define CPUISMIPS3		0
# define CPUIS64BITS		0
# define CPUISMIPS32		0
# define CPUISMIPS32R2		0
# define CPUISMIPS64		0
# define CPUISMIPS64R2		0
# define CPUISMIPSNN		0
# define MIPS_HAS_R4K_MMU	0
# define MIPS_HAS_CLOCK		0
# define MIPS_HAS_LLSC		0
# define MIPS_HAS_LLADDR	0
# define MIPS_HAS_DSP		0
# define MIPS_HAS_LMMI		0

#elif defined(MIPS3) || defined(MIPS4)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS32R2		0
# define CPUISMIPS64		0
# define CPUISMIPS64R2		0
# define CPUISMIPSNN		0
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# if defined(_LOCORE)
#  if !defined(MIPS3_4100)
#   define MIPS_HAS_LLSC	1
#  else
#   define MIPS_HAS_LLSC	0
#  endif
# else	/* _LOCORE */
#  define MIPS_HAS_LLSC		(mips_options.mips_has_llsc)
# endif	/* _LOCORE */
# define MIPS_HAS_LLADDR	((mips_options.mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)
# define MIPS_HAS_DSP		0
# if defined(MIPS3_LOONGSON2)
#  define MIPS_HAS_LMMI		((mips_options.mips_cpu_flags & CPU_MIPS_LOONGSON2) != 0)
# else
#  define MIPS_HAS_LMMI		0
# endif
#elif defined(MIPS32)

# define CPUISMIPS3		1
# define CPUIS64BITS		0
# define CPUISMIPS32		1
# define CPUISMIPS32R2		0
# define CPUISMIPS64		0
# define CPUISMIPS64R2		0
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1
# define MIPS_HAS_LLADDR	((mips_options.mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)
# define MIPS_HAS_DSP		0
# define MIPS_HAS_LMMI		0

#elif defined(MIPS32R2)

# define CPUISMIPS3		1
# define CPUIS64BITS		0
# define CPUISMIPS32		0
# define CPUISMIPS32R2		1
# define CPUISMIPS64		0
# define CPUISMIPS64R2		0
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1
# define MIPS_HAS_LLADDR	((mips_options.mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)
# define MIPS_HAS_DSP		(mips_options.mips_cpu_flags & CPU_MIPS_HAVE_DSP)
# define MIPS_HAS_LMMI		0

#elif defined(MIPS64)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS32R2		0
# define CPUISMIPS64		1
# define CPUISMIPS64R2		0
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1
# define MIPS_HAS_LLADDR	((mips_options.mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)
# define MIPS_HAS_DSP		0
# define MIPS_HAS_LMMI		0

#elif defined(MIPS64R2)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS32R2		0
# define CPUISMIPS64		0
# define CPUISMIPS64R2		1
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1
# define MIPS_HAS_LLADDR	((mips_options.mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)
# define MIPS_HAS_DSP		(mips_options.mips_cpu_flags & CPU_MIPS_HAVE_DSP)
# define MIPS_HAS_LMMI		0

#endif

#else /* run-time test */

#ifndef	_LOCORE

#define	MIPS_HAS_R4K_MMU	(mips_options.mips_has_r4k_mmu)
#define	MIPS_HAS_LLSC		(mips_options.mips_has_llsc)
#define	MIPS_HAS_LLADDR		((mips_options.mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)
# define MIPS_HAS_DSP		(mips_options.mips_cpu_flags & CPU_MIPS_HAVE_DSP)

/* This test is ... rather bogus */
#define	CPUISMIPS3	((mips_options.mips_cpu_arch & \
	(CPU_ARCH_MIPS3 | CPU_ARCH_MIPS4 | CPU_ARCH_MIPS32 | CPU_ARCH_MIPS64)) != 0)

/* And these aren't much better while the previous test exists as is... */
#define	CPUISMIPS4	((mips_options.mips_cpu_arch & CPU_ARCH_MIPS4) != 0)
#define	CPUISMIPS5	((mips_options.mips_cpu_arch & CPU_ARCH_MIPS5) != 0)
#define	CPUISMIPS32	((mips_options.mips_cpu_arch & CPU_ARCH_MIPS32) != 0)
#define	CPUISMIPS32R2	((mips_options.mips_cpu_arch & CPU_ARCH_MIPS32R2) != 0)
#define	CPUISMIPS64	((mips_options.mips_cpu_arch & CPU_ARCH_MIPS64) != 0)
#define	CPUISMIPS64R2	((mips_options.mips_cpu_arch & CPU_ARCH_MIPS64R2) != 0)
#define	CPUISMIPSNN	((mips_options.mips_cpu_arch & (CPU_ARCH_MIPS32 | CPU_ARCH_MIPS32R2 | CPU_ARCH_MIPS64 | CPU_ARCH_MIPS64R2)) != 0)
#define	CPUIS64BITS	((mips_options.mips_cpu_arch & \
	(CPU_ARCH_MIPS3 | CPU_ARCH_MIPS4 | CPU_ARCH_MIPS64 | CPU_ARCH_MIPS64R2)) != 0)

#define	MIPS_HAS_CLOCK	(mips_options.mips_cpu_arch >= CPU_ARCH_MIPS3)

#else	/* !_LOCORE */

#define	MIPS_HAS_LLSC	0

#endif	/* !_LOCORE */

#endif /* run-time test */

#ifndef	_LOCORE

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

/*
 * Send an inter-processor interupt to each other CPU (excludes curcpu())
 */
void cpu_broadcast_ipi(int);

/*
 * Send an inter-processor interupt to CPUs in cpuset (excludes curcpu())
 */
void cpu_multicast_ipi(__cpuset_t, int);

/*
 * Send an inter-processor interupt to another CPU.
 */
int cpu_send_ipi(struct cpu_info *, int);

/*
 * cpu_intr(ppl, pc, status);  (most state needed by clockframe)
 */
void cpu_intr(int, vaddr_t, uint32_t);

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.
 */
struct clockframe {
	vaddr_t		pc;	/* program counter at time of interrupt */
	uint32_t	sr;	/* status register at time of interrupt */
	bool		intr;	/* interrupted a interrupt */
};

/*
 * A port must provde CLKF_USERMODE() for use in machine-independent code.
 * These differ on r4000 and r3000 systems; provide them in the
 * port-dependent file that includes this one, using the macros below.
 */

/* mips1 versions */
#define	MIPS1_CLKF_USERMODE(framep)	((framep)->sr & MIPS_SR_KU_PREV)

/* mips3 versions */
#define	MIPS3_CLKF_USERMODE(framep)	((framep)->sr & MIPS_SR_KSU_USER)

#define	CLKF_PC(framep)		((framep)->pc)
#define	CLKF_INTR(framep)	((framep)->intr)

#if defined(MIPS3_PLUS) && !defined(MIPS1)		/* XXX bogus! */
#define	CLKF_USERMODE(framep)	MIPS3_CLKF_USERMODE(framep)
#endif

#if !defined(MIPS3_PLUS) && defined(MIPS1)		/* XXX bogus! */
#define	CLKF_USERMODE(framep)	MIPS1_CLKF_USERMODE(framep)
#endif

#if defined(MIPS3_PLUS) && defined(MIPS1)		/* XXX bogus! */
#define CLKF_USERMODE(framep) \
    ((CPUISMIPS3) ? MIPS3_CLKF_USERMODE(framep):  MIPS1_CLKF_USERMODE(framep))
#endif

/*
 * Misc prototypes and variable declarations.
 */
#define	LWP_PC(l)	cpu_lwp_pc(l)

struct proc;
struct lwp;
struct pcb;
struct reg;

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
void	cpu_need_resched(struct cpu_info *, int);
/*
 * Notify the current lwp (l) that it has a signal pending,
 * process as soon as possible.
 */
void	cpu_signotify(struct lwp *);

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the MIPS, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
void	cpu_need_proftick(struct lwp *);
void	cpu_set_curpri(int);

extern int mips_poolpage_vmfreelist;	/* freelist to allocate poolpages */

struct cpu_info *
	cpu_info_alloc(struct pmap_tlb_info *, cpuid_t, cpuid_t, cpuid_t,
	    cpuid_t);
void	cpu_attach_common(device_t, struct cpu_info *);
void	cpu_startup_common(void);
#ifdef _LP64
void	cpu_vmspace_exec(struct lwp *, vaddr_t, vaddr_t);
#endif

#ifdef MULTIPROCESSOR
void	cpu_hatch(struct cpu_info *ci);
void	cpu_trampoline(void);
void	cpu_boot_secondary_processors(void);
void	cpu_halt(void);
void	cpu_halt_others(void);
void	cpu_pause(struct reg *);
void	cpu_pause_others(void);
void	cpu_resume(int);
void	cpu_resume_others(void);
int	cpu_is_paused(int);
void	cpu_debug_dump(void);

extern volatile __cpuset_t cpus_running;
extern volatile __cpuset_t cpus_hatched;
extern volatile __cpuset_t cpus_paused;
extern volatile __cpuset_t cpus_resumed;
extern volatile __cpuset_t cpus_halted;
#endif

/* copy.S */
int32_t kfetch_32(volatile uint32_t *, uint32_t);
int8_t	ufetch_int8(void *);
int16_t	ufetch_int16(void *);
int32_t ufetch_int32(void *);
uint8_t	ufetch_uint8(void *);
uint16_t ufetch_uint16(void *);
uint32_t ufetch_uint32(void *);
int8_t	ufetch_int8_intrsafe(void *);
int16_t	ufetch_int16_intrsafe(void *);
int32_t ufetch_int32_intrsafe(void *);
uint8_t	ufetch_uint8_intrsafe(void *);
uint16_t ufetch_uint16_intrsafe(void *);
uint32_t ufetch_uint32_intrsafe(void *);
#ifdef _LP64
int64_t ufetch_int64(void *);
uint64_t ufetch_uint64(void *);
int64_t ufetch_int64_intrsafe(void *);
uint64_t ufetch_uint64_intrsafe(void *);
#endif
char	ufetch_char(void *);
short	ufetch_short(void *);
int	ufetch_int(void *);
long	ufetch_long(void *);
char	ufetch_char_intrsafe(void *);
short	ufetch_short_intrsafe(void *);
int	ufetch_int_intrsafe(void *);
long	ufetch_long_intrsafe(void *);

u_char	ufetch_uchar(void *);
u_short	ufetch_ushort(void *);
u_int	ufetch_uint(void *);
u_long	ufetch_ulong(void *);
u_char	ufetch_uchar_intrsafe(void *);
u_short	ufetch_ushort_intrsafe(void *);
u_int	ufetch_uint_intrsafe(void *);
u_long	ufetch_ulong_intrsafe(void *);
void 	*ufetch_ptr(void *);

int	ustore_int8(void *, int8_t);
int	ustore_int16(void *, int16_t);
int	ustore_int32(void *, int32_t);
int	ustore_uint8(void *, uint8_t);
int	ustore_uint16(void *, uint16_t);
int	ustore_uint32(void *, uint32_t);
int	ustore_int8_intrsafe(void *, int8_t);
int	ustore_int16_intrsafe(void *, int16_t);
int	ustore_int32_intrsafe(void *, int32_t);
int	ustore_uint8_intrsafe(void *, uint8_t);
int	ustore_uint16_intrsafe(void *, uint16_t);
int	ustore_uint32_intrsafe(void *, uint32_t);
#ifdef _LP64
int	ustore_int64(void *, int64_t);
int	ustore_uint64(void *, uint64_t);
int	ustore_int64_intrsafe(void *, int64_t);
int	ustore_uint64_intrsafe(void *, uint64_t);
#endif
int	ustore_char(void *, char);
int	ustore_char_intrsafe(void *, char);
int	ustore_short(void *, short);
int	ustore_short_intrsafe(void *, short);
int	ustore_int(void *, int);
int	ustore_int_intrsafe(void *, int);
int	ustore_long(void *, long);
int	ustore_long_intrsafe(void *, long);
int	ustore_uchar(void *, u_char);
int	ustore_uchar_intrsafe(void *, u_char);
int	ustore_ushort(void *, u_short);
int	ustore_ushort_intrsafe(void *, u_short);
int	ustore_uint(void *, u_int);
int	ustore_uint_intrsafe(void *, u_int);
int	ustore_ulong(void *, u_long);
int	ustore_ulong_intrsafe(void *, u_long);
int 	ustore_ptr(void *, void *);
int	ustore_ptr_intrsafe(void *, void *);

int	ustore_uint32_isync(void *, uint32_t);

/* trap.c */
void	netintr(void);
int	kdbpeek(vaddr_t);

/* mips_dsp.c */
void	dsp_init(void);
void	dsp_discard(void);
void	dsp_load(void);
void	dsp_save(void);
bool	dsp_used_p(void);
extern const pcu_ops_t mips_dsp_ops;

/* mips_fpu.c */
void	fpu_init(void);
void	fpu_discard(void);
void	fpu_load(void);
void	fpu_save(void);
bool	fpu_used_p(void);
extern const pcu_ops_t mips_fpu_ops;

/* mips_machdep.c */
void	dumpsys(void);
int	savectx(struct pcb *);
void	cpu_identify(device_t);

/* locore*.S */
int	badaddr(void *, size_t);
int	badaddr64(uint64_t, size_t);

/* vm_machdep.c */
void *	cpu_uarea_alloc(bool);
bool	cpu_uarea_free(void *);
void	cpu_proc_fork(struct proc *, struct proc *);
vaddr_t	cpu_lwp_pc(struct lwp *);
int	ioaccess(vaddr_t, paddr_t, vsize_t);
int	iounaccess(vaddr_t, vsize_t);

#endif /* ! _LOCORE */
#endif /* _KERNEL */
#endif /* _CPU_H_ */
