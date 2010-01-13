/*	$NetBSD: cpu.h,v 1.90.16.10 2010/01/13 09:42:16 cliff Exp $	*/

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
#ifndef _LOCORE
#include <sys/cpu_data.h>
#include <sys/device.h>

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

struct pridtab {
	int	cpu_cid;
	int	cpu_pid;
	int	cpu_rev;	/* -1 == wildcard */
	int	cpu_copts;	/* -1 == wildcard */
	int	cpu_isa;	/* -1 == probed (mips32/mips64) */
	int	cpu_ntlb;	/* -1 == unknown, 0 == probed */
	int	cpu_flags;
	u_int	cpu_cp0flags;	/* presence of some cp0 regs */
	u_int	cpu_cidflags;	/* company-specific flags */
	const char	*cpu_name;
};

extern const struct pridtab *mycpu;

/*
 * bitfield defines for cpu_cp0flags
 */
#define  MIPS_CP0FL_USE		__BIT(0)	/* use these flags */
#define  MIPS_CP0FL_ECC		__BIT(1)
#define  MIPS_CP0FL_CACHE_ERR	__BIT(2)
#define  MIPS_CP0FL_EIRR	__BIT(3)
#define  MIPS_CP0FL_EIMR	__BIT(4)
#define  MIPS_CP0FL_EBASE	__BIT(5)
#define  MIPS_CP0FL_CONFIG	__BIT(6)
#define  MIPS_CP0FL_CONFIGn(n)	(__BIT(7) << ((n) & 7))

/*
 * cpu_cidflags defines, by company
 */
/*
 * RMI company-specific cpu_cidflags
 */
#define MIPS_CIDFL_RMI_TYPE     	__BITS(2,0)
# define  CIDFL_RMI_TYPE_XLR     	0
# define  CIDFL_RMI_TYPE_XLS     	1
# define  CIDFL_RMI_TYPE_XLP     	2
#define MIPS_CIDFL_RMI_THREADS_MASK	__BITS(6,3)
# define MIPS_CIDFL_RMI_THREADS_SHIFT	3
#define MIPS_CIDFL_RMI_CORES_MASK	__BITS(10,7)
# define MIPS_CIDFL_RMI_CORES_SHIFT	7
# define LOG2_1	0
# define LOG2_2	1
# define LOG2_4	2
# define LOG2_8	3
# define MIPS_CIDFL_RMI_CPUS(ncores, nthreads)				\
		((LOG2_ ## ncores << MIPS_CIDFL_RMI_CORES_SHIFT)	\
		|(LOG2_ ## nthreads << MIPS_CIDFL_RMI_THREADS_SHIFT))
# define MIPS_CIDFL_RMI_NTHREADS(cidfl)					\
		(1 << (((cidfl) & MIPS_CIDFL_RMI_THREADS_MASK)		\
			>> MIPS_CIDFL_RMI_THREADS_SHIFT))
# define MIPS_CIDFL_RMI_NCORES(cidfl)					\
		(1 << (((cidfl) & MIPS_CIDFL_RMI_CORES_MASK)		\
			>> MIPS_CIDFL_RMI_CORES_SHIFT))
#define MIPS_CIDFL_RMI_L2SZ_MASK	__BITS(14,11)
# define MIPS_CIDFL_RMI_L2SZ_SHIFT	11
# define RMI_L2SZ_256KB	 0
# define RMI_L2SZ_512KB  1
# define RMI_L2SZ_1MB    2
# define RMI_L2SZ_2MB    3
# define RMI_L2SZ_4MB    4
# define MIPS_CIDFL_RMI_L2(l2sz)					\
		(RMI_L2SZ_ ## l2sz << MIPS_CIDFL_RMI_L2SZ_SHIFT)
# define MIPS_CIDFL_RMI_L2SZ(cidfl)					\
		((256*1024) << (((cidfl) & MIPS_CIDFL_RMI_L2SZ_MASK)	\
			>> MIPS_CIDFL_RMI_L2SZ_SHIFT))



struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct cpu_info *ci_next;	/* Next CPU in list */
	cpuid_t ci_cpuid;		/* Machine-level identifier */
	u_long ci_cpu_freq;		/* CPU frequency */
	u_long ci_cycles_per_hz;	/* CPU freq / hz */
	u_long ci_divisor_delay;	/* for delay/DELAY */
	u_long ci_divisor_recip;	/* unused, for obsolete microtime(9) */
	struct lwp *ci_curlwp;		/* currently running lwp */
	struct lwp *ci_fpcurlwp;	/* the current FPU owner */
	int ci_want_resched;		/* user preemption pending */
	int ci_mtx_count;		/* negative count of held mutexes */
	int ci_mtx_oldspl;		/* saved SPL value */
	int ci_idepth;			/* hardware interrupt depth */
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

/*
 * Platform can override, but note this breaks userland compatibility
 * with other mips platforms.
 */
#ifndef CPU_MAXID
#define CPU_MAXID		5	/* number of valid machdep ids */

#endif

#ifdef _KERNEL
#if defined(_LKM) || defined(_STANDALONE)
/* Assume all CPU architectures are valid for LKM's and standlone progs */
#define	MIPS1	1
#define	MIPS3	1
#define	MIPS4	1
#define	MIPS32	1
#define	MIPS64	1
#endif

#if (MIPS1 + MIPS3 + MIPS4 + MIPS32 + MIPS64) == 0
#error at least one of MIPS1, MIPS3, MIPS4, MIPS32 or MIPS64 must be specified
#endif

/* Shortcut for MIPS3 or above defined */
#if defined(MIPS3) || defined(MIPS4) || defined(MIPS32) || defined(MIPS64)
#define	MIPS3_PLUS	1
#else
#undef MIPS3_PLUS
#endif

/*
 * Macros to find the CPU architecture we're on at run-time,
 * or if possible, at compile-time.
 */

#define	CPU_ARCH_MIPSx	0		/* XXX unknown */
#define	CPU_ARCH_MIPS1	(1 << 0)
#define	CPU_ARCH_MIPS2	(1 << 1)
#define	CPU_ARCH_MIPS3	(1 << 2)
#define	CPU_ARCH_MIPS4	(1 << 3)
#define	CPU_ARCH_MIPS5	(1 << 4)
#define	CPU_ARCH_MIPS32	(1 << 5)
#define	CPU_ARCH_MIPS64	(1 << 6)

/* Note: must be kept in sync with -ffixed-?? Makefile.mips. */
#define MIPS_CURLWP             $23
#define MIPS_CURLWP_QUOTED      "$23"
#define MIPS_CURLWP_CARD	23
#define	MIPS_CURLWP_FRAME(x)	FRAME_S7(x)

#ifndef _LOCORE

extern struct cpu_info cpu_info_store;
register struct lwp *mips_curlwp asm(MIPS_CURLWP_QUOTED);

#define	curlwp			mips_curlwp
#define	curcpu()		(curlwp->l_cpu)
#define	curpcb			((struct pcb *)curlwp->l_addr)
#define	fpcurlwp		(curcpu()->ci_fpcurlwp)
#define	cpu_number()		(0)
#define	cpu_proc_fork(p1, p2)	((void)((p2)->p_md.md_abi = (p1)->p_md.md_abi))

/* XXX simonb
 * Should the following be in a cpu_info type structure?
 * And how many of these are per-cpu vs. per-system?  (Ie,
 * we can assume that all cpus have the same mmu-type, but
 * maybe not that all cpus run at the same clock speed.
 * Some SGI's apparently support R12k and R14k in the same
 * box.)
 */
extern int cpu_arch;
extern int mips_cpu_flags;
extern int mips_has_r4k_mmu;
extern int mips_has_llsc;
extern int mips3_pg_cached;
#ifdef _LP64
extern uint64_t mips3_xkphys_cached;
#endif
extern u_int mips3_pg_shift;

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
#define	MIPS_NOT_SUPP			0x8000

#endif	/* !_LOCORE */

#if ((MIPS1 + MIPS3 + MIPS4 + MIPS32 + MIPS64) == 1) || defined(_LOCORE)

#if defined(MIPS1)

# define CPUISMIPS3		0
# define CPUIS64BITS		0
# define CPUISMIPS32		0
# define CPUISMIPS64		0
# define CPUISMIPSNN		0
# define MIPS_HAS_R4K_MMU	0
# define MIPS_HAS_CLOCK		0
# define MIPS_HAS_LLSC		0
# define MIPS_HAS_LLADDR	0

#elif defined(MIPS3) || defined(MIPS4)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS64		0
# define CPUISMIPSNN		0
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# if defined(_LOCORE)
#  if !defined(MIPS3_5900) && !defined(MIPS3_4100)
#   define MIPS_HAS_LLSC	1
#  else
#   define MIPS_HAS_LLSC	0
#  endif
# else	/* _LOCORE */
#  define MIPS_HAS_LLSC		(mips_has_llsc)
# endif	/* _LOCORE */
# define MIPS_HAS_LLADDR	((mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)

#elif defined(MIPS32)

# define CPUISMIPS3		1
# define CPUIS64BITS		0
# define CPUISMIPS32		1
# define CPUISMIPS64		0
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1
# define MIPS_HAS_LLADDR	((mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)

#elif defined(MIPS64)

# define CPUISMIPS3		1
# define CPUIS64BITS		1
# define CPUISMIPS32		0
# define CPUISMIPS64		1
# define CPUISMIPSNN		1
# define MIPS_HAS_R4K_MMU	1
# define MIPS_HAS_CLOCK		1
# define MIPS_HAS_LLSC		1
# define MIPS_HAS_LLADDR	((mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)

#endif

#else /* run-time test */

#ifndef	_LOCORE

#define	MIPS_HAS_R4K_MMU	(mips_has_r4k_mmu)
#define	MIPS_HAS_LLSC		(mips_has_llsc)
#define	MIPS_HAS_LLADDR		((mips_cpu_flags & CPU_MIPS_NO_LLADDR) == 0)

/* This test is ... rather bogus */
#define	CPUISMIPS3	((cpu_arch & \
	(CPU_ARCH_MIPS3 | CPU_ARCH_MIPS4 | CPU_ARCH_MIPS32 | CPU_ARCH_MIPS64)) != 0)

/* And these aren't much better while the previous test exists as is... */
#define	CPUISMIPS32	((cpu_arch & CPU_ARCH_MIPS32) != 0)
#define	CPUISMIPS64	((cpu_arch & CPU_ARCH_MIPS64) != 0)
#define	CPUISMIPSNN	((cpu_arch & (CPU_ARCH_MIPS32 | CPU_ARCH_MIPS64)) != 0)
#define	CPUIS64BITS	((cpu_arch & \
	(CPU_ARCH_MIPS3 | CPU_ARCH_MIPS4 | CPU_ARCH_MIPS64)) != 0)

#define	MIPS_HAS_CLOCK	(cpu_arch >= CPU_ARCH_MIPS3)

#else	/* !_LOCORE */

#define	MIPS_HAS_LLSC	0

#endif	/* !_LOCORE */

#endif /* run-time test */

#ifndef	_LOCORE

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapout(p)			panic("cpu_swapout: can't get here");

void cpu_intr(uint32_t, uint32_t, vaddr_t, uint32_t);

/*
 * Arguments to hardclock and gatherstats encapsulate the previous
 * machine state in an opaque clockframe.
 */
struct clockframe {
	vaddr_t	pc;	/* program counter at time of interrupt */
	uint32_t	sr;	/* status register at time of interrupt */
	u_int		ppl;	/* previous priority level at time of interrupt */
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
#define	CLKF_INTR(framep)	(0)

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
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	PROC_PC(p)							\
	(((struct frame *)(p)->p_md.md_regs)->f_regs[37])	/* XXX PC */

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
void	cpu_need_resched(struct cpu_info *, int);

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the MIPS, request an ast to send us
 * through trap, marking the proc as needing a profiling tick.
 */
#define	cpu_need_proftick(l)						\
do {									\
	(l)->l_pflag |= LP_OWEUPC;					\
	aston(l);							\
} while (/*CONSTCOND*/0)

/*
 * Notify the current lwp (l) that it has a signal pending,
 * process as soon as possible.
 */
#define	cpu_signotify(l)	aston(l)

#define aston(l)		((l)->l_md.md_astpending = 1)

/*
 * Misc prototypes and variable declarations.
 */
struct lwp;
struct user;

extern struct segtab *segbase;		/* current segtab base */
extern int mips_poolpage_vmfreelist;	/* freelist to allocate poolpages */

/* copy.S */
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

/* mips_machdep.c */
struct mips_vmfreelist;
struct phys_ram_seg;
void	dumpsys(void);
int	savectx(struct user *);
void	mips_init_msgbuf(void);
void	mips_init_lwp0_uarea(void);
void	mips_page_physload(vaddr_t, vaddr_t,
	    const struct phys_ram_seg *, size_t,
	    const struct mips_vmfreelist *, size_t);
void	savefpregs(struct lwp *);
void	loadfpregs(struct lwp *);

/* locore*.S */
int	badaddr(void *, size_t);
int	badaddr64(uint64_t, size_t);

/* mips_machdep.c */
void	cpu_identify(device_t);
void	mips_vector_init(void);

#endif /* ! _LOCORE */
#endif /* _KERNEL */
#endif /* _CPU_H_ */
