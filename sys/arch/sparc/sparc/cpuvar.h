/*	$NetBSD: cpuvar.h,v 1.77 2009/05/18 01:36:11 mrg Exp $ */

/*
 *  Copyright (c) 1996 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to The NetBSD Foundation
 *  by Paul Kranenburg.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _sparc_cpuvar_h
#define _sparc_cpuvar_h

#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_ddb.h"
#include "opt_sparc_arch.h"
#endif

#include <sys/device.h>
#include <sys/lock.h>
#include <sys/cpu_data.h>

#include <sparc/include/reg.h>
#include <sparc/sparc/cache.h>	/* for cacheinfo */

/*
 * CPU/MMU module information.
 * There is one of these for each "mainline" CPU module we support.
 * The information contained in the structure is used only during
 * auto-configuration of the CPUs; some fields are copied into the
 * per-cpu data structure (cpu_info) for easy access during normal
 * operation.
 */
struct cpu_info;
struct module_info {
	int  cpu_type;
	enum vactype vactype;
	void (*cpu_match)(struct cpu_info *, struct module_info *, int);
	void (*getcacheinfo)(struct cpu_info *sc, int node);
	void (*hotfix)(struct cpu_info *);
	void (*mmu_enable)(void);
	void (*cache_enable)(void);
	int  (*getmid)(void);		/* Get MID of current CPU */
	int  ncontext;			/* max. # of contexts (that we use) */

	void (*get_syncflt)(void);
	int  (*get_asyncflt)(u_int *, u_int *);
	void (*cache_flush)(void *, u_int);
	void (*sp_vcache_flush_page)(int, int);
	void (*ft_vcache_flush_page)(int, int);
	void (*sp_vcache_flush_segment)(int, int, int);
	void (*ft_vcache_flush_segment)(int, int, int);
	void (*sp_vcache_flush_region)(int, int);
	void (*ft_vcache_flush_region)(int, int);
	void (*sp_vcache_flush_context)(int);
	void (*ft_vcache_flush_context)(int);
	void (*sp_vcache_flush_range)(int, int, int);
	void (*ft_vcache_flush_range)(int, int, int);
	void (*pcache_flush_page)(paddr_t, int);
	void (*pure_vcache_flush)(void);
	void (*cache_flush_all)(void);
	void (*memerr)(unsigned, u_int, u_int, struct trapframe *);
	void (*zero_page)(paddr_t);
	void (*copy_page)(paddr_t, paddr_t);
};

/*
 * Message structure for Inter Processor Communication in MP systems
 */
struct xpmsg {
	volatile int tag;
#define	XPMSG15_PAUSECPU	1
#define	XPMSG_FUNC		4
#define	XPMSG_FTRP		5

	volatile union {
		/*
		 * Cross call: ask to run (*func)(arg0,arg1,arg2)
		 * or (*trap)(arg0,arg1,arg2). `trap' should be the
		 * address of a `fast trap' handler that executes in
		 * the trap window (see locore.s).
		 */
		struct xpmsg_func {
			int	(*func)(int, int, int);
			void	(*trap)(int, int, int);
			int	arg0;
			int	arg1;
			int	arg2;
			int	retval;
		} xpmsg_func;
	} u;
	volatile int	received;
	volatile int	complete;
};

/*
 * This must be locked around all message transactions to ensure only
 * one CPU is generating them.
 */
extern struct simplelock xpmsg_lock;

#define LOCK_XPMSG()	simple_lock(&xpmsg_lock);
#define UNLOCK_XPMSG()	simple_unlock(&xpmsg_lock);

/*
 * The cpuinfo structure. This structure maintains information about one
 * currently installed CPU (there may be several of these if the machine
 * supports multiple CPUs, as on some Sun4m architectures). The information
 * in this structure supersedes the old "cpumod", "mmumod", and similar
 * fields.
 */

struct cpu_info {
	struct cpu_data ci_data;	/* MI per-cpu data */

	/* Scheduler flags */
	int	ci_want_ast;
	int	ci_want_resched;

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

	/* Primary Inter-processor message area */
	struct xpmsg	msg;

	int		ci_cpuid;	/* CPU index (see cpus[] array) */

	/* Context administration */
	int		*ctx_tbl;	/* [4m] SRMMU-edible context table */
	paddr_t		ctx_tbl_pa;	/* [4m] ctx table physical address */

	/* Cache information */
	struct cacheinfo	cacheinfo;	/* see cache.h */

	/* various flags to workaround anomalies in chips */
	volatile int	flags;		/* see CPUFLG_xxx, below */

	/* Per processor counter register (sun4m only) */
	volatile struct counter_4m	*counterreg_4m;

	/* Per processor interrupt mask register (sun4m only) */
	volatile struct icr_pi	*intreg_4m;
	/*
	 * Send a IPI to (cpi).  For Ross cpus we need to read
	 * the pending register to avoid a hardware bug.
	 */
#define raise_ipi(cpi,lvl)	do {			\
	volatile int x;					\
	(cpi)->intreg_4m->pi_set = PINTR_SINTRLEV(lvl);	\
	x = (cpi)->intreg_4m->pi_pend;			\
} while (0)

	int		sun4_mmu3l;	/* [4]: 3-level MMU present */
#if defined(SUN4_MMU3L)
#define HASSUN4_MMU3L	(cpuinfo.sun4_mmu3l)
#else
#define HASSUN4_MMU3L	(0)
#endif
	int		ci_idepth;		/* Interrupt depth */

	/*
	 * The following pointers point to processes that are somehow
	 * associated with this CPU--running on it, using its FPU,
	 * etc.
	 */
	struct	lwp	*ci_curlwp;		/* CPU owner */
	struct	lwp 	*fplwp;			/* FPU owner */

	int		ci_mtx_count;
	int		ci_mtx_oldspl;

	/*
	 * Idle PCB and Interrupt stack;
	 */
	void		*eintstack;		/* End of interrupt stack */
#define INT_STACK_SIZE	(128 * 128)		/* 128 128-byte stack frames */
	void		*redzone;		/* DEBUG: stack red zone */
#define REDSIZE		(8*96)			/* some room for bouncing */

	struct	pcb	*curpcb;		/* CPU's PCB & kernel stack */

	/* locore defined: */
	void	(*get_syncflt)(void);		/* Not C-callable */
	int	(*get_asyncflt)(u_int *, u_int *);

	/* Synchronous Fault Status; temporary storage */
	struct {
		int	sfsr;
		int	sfva;
	} syncfltdump;

	/*
	 * Cache handling functions.
	 * Most cache flush function come in two flavours: one that
	 * acts only on the CPU it executes on, and another that
	 * uses inter-processor signals to flush the cache on
	 * all processor modules.
	 * The `ft_' versions are fast trap cache flush handlers.
	 */
	void	(*cache_flush)(void *, u_int);
	void	(*vcache_flush_page)(int, int);
	void	(*sp_vcache_flush_page)(int, int);
	void	(*ft_vcache_flush_page)(int, int);
	void	(*vcache_flush_segment)(int, int, int);
	void	(*sp_vcache_flush_segment)(int, int, int);
	void	(*ft_vcache_flush_segment)(int, int, int);
	void	(*vcache_flush_region)(int, int);
	void	(*sp_vcache_flush_region)(int, int);
	void	(*ft_vcache_flush_region)(int, int);
	void	(*vcache_flush_context)(int);
	void	(*sp_vcache_flush_context)(int);
	void	(*ft_vcache_flush_context)(int);

	/* The are helpers for (*cache_flush)() */
	void	(*sp_vcache_flush_range)(int, int, int);
	void	(*ft_vcache_flush_range)(int, int, int);

	void	(*pcache_flush_page)(paddr_t, int);
	void	(*pure_vcache_flush)(void);
	void	(*cache_flush_all)(void);

	/* Support for hardware-assisted page clear/copy */
	void	(*zero_page)(paddr_t);
	void	(*copy_page)(paddr_t, paddr_t);

	/* Virtual addresses for use in pmap copy_page/zero_page */
	void *	vpage[2];
	int	*vpage_pte[2];		/* pte location of vpage[] */

	void	(*cache_enable)(void);

	int	cpu_type;	/* Type: see CPUTYP_xxx below */

	/* Inter-processor message area (high priority but used infrequently) */
	struct xpmsg	msg_lev15;

	/* CPU information */
	int		node;		/* PROM node for this CPU */
	int		mid;		/* Module ID for MP systems */
	int		mbus;		/* 1 if CPU is on MBus */
	int		mxcc;		/* 1 if a MBus-level MXCC is present */
	const char	*cpu_name;	/* CPU model */
	int		cpu_impl;	/* CPU implementation code */
	int		cpu_vers;	/* CPU version code */
	int		mmu_impl;	/* MMU implementation code */
	int		mmu_vers;	/* MMU version code */
	int		master;		/* 1 if this is bootup CPU */

	vaddr_t		mailbox;	/* VA of CPU's mailbox */

	int		mmu_ncontext;	/* Number of contexts supported */
	int		mmu_nregion; 	/* Number of regions supported */
	int		mmu_nsegment;	/* [4/4c] Segments */
	int		mmu_npmeg;	/* [4/4c] Pmegs */

/* XXX - we currently don't actually use the following */
	int		arch;		/* Architecture: CPU_SUN4x */
	int		class;		/* Class: SuperSPARC, microSPARC... */
	int		classlvl;	/* Iteration in class: 1, 2, etc. */
	int		classsublvl;	/* stepping in class (version) */

	int		hz;		/* Clock speed */

	/* FPU information */
	int		fpupresent;	/* true if FPU is present */
	int		fpuvers;	/* FPU revision */
	const char	*fpu_name;	/* FPU model */
	char		fpu_namebuf[32];/* Buffer for FPU name, if necessary */

	/* XXX */
	volatile void	*ci_ddb_regs;		/* DDB regs */

	/*
	 * The following are function pointers to do interesting CPU-dependent
	 * things without having to do type-tests all the time
	 */

	/* bootup things: access to physical memory */
	u_int	(*read_physmem)(u_int addr, int space);
	void	(*write_physmem)(u_int addr, u_int data);
	void	(*cache_tablewalks)(void);
	void	(*mmu_enable)(void);
	void	(*hotfix)(struct cpu_info *);


#if 0
	/* hardware-assisted block operation routines */
	void		(*hwbcopy)(const void *from, void *to, size_t len);
	void		(*hwbzero)(void *buf, size_t len);

	/* routine to clear mbus-sbus buffers */
	void		(*mbusflush)(void);
#endif

	/*
	 * Memory error handler; parity errors, unhandled NMIs and other
	 * unrecoverable faults end up here.
	 */
	void		(*memerr)(unsigned, u_int, u_int, struct trapframe *);
	void		(*idlespin)(struct cpu_info *);
	/* Module Control Registers */
	/*bus_space_handle_t*/ long ci_mbusport;
	/*bus_space_handle_t*/ long ci_mxccregs;

	u_int	ci_tt;			/* Last trap (if tracing) */
};

/*
 * CPU architectures
 */
#define CPUARCH_UNKNOWN		0
#define CPUARCH_SUN4		1
#define CPUARCH_SUN4C		2
#define CPUARCH_SUN4M		3
#define	CPUARCH_SUN4D		4
#define CPUARCH_SUN4U		5

/*
 * CPU classes
 */
#define CPUCLS_UNKNOWN		0

#if defined(SUN4)
#define CPUCLS_SUN4		1
#endif

#if defined(SUN4C)
#define CPUCLS_SUN4C		5
#endif

#if defined(SUN4M) || defined(SUN4D)
#define CPUCLS_MICROSPARC	10	/* MicroSPARC-II */
#define CPUCLS_SUPERSPARC	11	/* Generic SuperSPARC */
#define CPUCLS_HYPERSPARC	12	/* Ross HyperSPARC RT620 */
#endif

/*
 * CPU types. Each of these should uniquely identify one platform/type of
 * system, i.e. "MBus-based 75 MHz SuperSPARC-II with ECache" is
 * CPUTYP_SS2_MBUS_MXCC. The general form is
 * 	CPUTYP_proctype_bustype_cachetype_etc_etc
 *
 * XXX: This is far from complete/comprehensive
 * XXX: ADD SUN4, SUN4C TYPES
 */
#define CPUTYP_UNKNOWN		0

#define CPUTYP_4_100		1 	/* Sun4/100 */
#define CPUTYP_4_200		2	/* Sun4/200 */
#define CPUTYP_4_300		3	/* Sun4/300 */
#define CPUTYP_4_400		4	/* Sun4/400 */

#define CPUTYP_SLC		10	/* SPARCstation SLC */
#define CPUTYP_ELC		11	/* SPARCstation ELC */
#define CPUTYP_IPX		12	/* SPARCstation IPX */
#define CPUTYP_IPC		13	/* SPARCstation IPC */
#define CPUTYP_1		14	/* SPARCstation 1 */
#define CPUTYP_1P		15	/* SPARCstation 1+ */
#define CPUTYP_2		16	/* SPARCstation 2 */

/* We classify the Sun4m's by feature, not by model (XXX: do same for 4/4c) */
#define	CPUTYP_SS2_MBUS_MXCC	20 	/* SuperSPARC-II, Mbus, MXCC (SS20) */
#define CPUTYP_SS1_MBUS_MXCC	21	/* SuperSPARC-I, Mbus, MXCC (SS10) */
#define CPUTYP_SS2_MBUS_NOMXCC	22	/* SuperSPARC-II, on MBus w/o MXCC */
#define CPUTYP_SS1_MBUS_NOMXCC	23	/* SuperSPARC-I, on MBus w/o MXCC */
#define CPUTYP_MS2		24	/* MicroSPARC-2 */
#define CPUTYP_MS1		25 	/* MicroSPARC-1 */
#define CPUTYP_HS_MBUS		26	/* MBus-based HyperSPARC */
#define CPUTYP_CYPRESS		27	/* MBus-based Cypress */

/*
 * CPU flags
 */
#define CPUFLG_CACHEPAGETABLES	0x1	/* caching pagetables OK on Sun4m */
#define CPUFLG_CACHEIOMMUTABLES	0x2	/* caching IOMMU translations OK */
#define CPUFLG_CACHEDVMA	0x4	/* DVMA goes through cache */
#define CPUFLG_SUN4CACHEBUG	0x8	/* trap page can't be cached */
#define CPUFLG_CACHE_MANDATORY	0x10	/* if cache is on, don't use
					   uncached access */
#define CPUFLG_HATCHED		0x1000	/* CPU is alive */
#define CPUFLG_PAUSED		0x2000	/* CPU is paused */
#define CPUFLG_GOTMSG		0x4000	/* CPU got an lev13 IPI */
#define CPUFLG_READY		0x8000	/* CPU available for IPI */


#define CPU_INFO_ITERATOR		int
#ifdef MULTIPROCESSOR
#define CPU_INFO_FOREACH(cii, cp)	cii = 0; cp = cpus[cii], cii < sparc_ncpus; cii++
#else
#define	CPU_INFO_FOREACH(cii, cp)	(void)cii, cp = curcpu(); cp != NULL; cp = NULL
#endif

/*
 * Useful macros.
 */
#define CPU_NOTREADY(cpi)	((cpi) == NULL || cpuinfo.mid == (cpi)->mid || \
				    ((cpi)->flags & CPUFLG_READY) == 0)

/*
 * Related function prototypes
 */
void getcpuinfo (struct cpu_info *sc, int node);
void mmu_install_tables (struct cpu_info *);
void pmap_alloc_cpu (struct cpu_info *);

#define	CPUSET_ALL	0xffffffffU	/* xcall to all configured CPUs */

#if defined(MULTIPROCESSOR)
typedef int (*xcall_func_t)(int, int, int);
typedef void (*xcall_trap_t)(int, int, int);
void xcall(xcall_func_t, xcall_trap_t, int, int, int, u_int);
/* Shorthand */
#define XCALL0(f,cpuset)		\
	xcall((xcall_func_t)f, NULL, 0, 0, 0, cpuset)
#define XCALL1(f,a1,cpuset)		\
	xcall((xcall_func_t)f, NULL, (int)a1, 0, 0, cpuset)
#define XCALL2(f,a1,a2,cpuset)		\
	xcall((xcall_func_t)f, NULL, (int)a1, (int)a2, 0, cpuset)
#define XCALL3(f,a1,a2,a3,cpuset)	\
	xcall((xcall_func_t)f, NULL, (int)a1, (int)a2, (int)a3, cpuset)

#define FXCALL0(f,tf,cpuset)		\
	xcall((xcall_func_t)f, (xcall_trap_t)tf, 0, 0, 0, cpuset)
#define FXCALL1(f,tf,a1,cpuset)		\
	xcall((xcall_func_t)f, (xcall_trap_t)tf, (int)a1, 0, 0, cpuset)
#define FXCALL2(f,tf,a1,a2,cpuset)	\
	xcall((xcall_func_t)f, (xcall_trap_t)tf, (int)a1, (int)a2, 0, cpuset)
#define FXCALL3(f,tf,a1,a2,a3,cpuset)	\
	xcall((xcall_func_t)f, (xcall_trap_t)tf, (int)a1, (int)a2, (int)a3, cpuset)
#else
#define XCALL0(f,cpuset)		/**/
#define XCALL1(f,a1,cpuset)		/**/
#define XCALL2(f,a1,a2,cpuset)		/**/
#define XCALL3(f,a1,a2,a3,cpuset)	/**/
#define FXCALL0(f,tf,cpuset)		/**/
#define FXCALL1(f,tf,a1,cpuset)		/**/
#define FXCALL2(f,tf,a1,a2,cpuset)	/**/
#define FXCALL3(f,tf,a1,a2,a3,cpuset)	/**/
#endif /* MULTIPROCESSOR */

extern int bootmid;			/* Module ID of boot CPU */
#define CPU_MID2CPUNO(mid)		((mid) != 0 ? (mid) - 8 : 0)

#ifdef MULTIPROCESSOR
extern struct cpu_info *cpus[];
extern u_int cpu_ready_mask;		/* the set of CPUs marked as READY */
#endif

#define cpuinfo	(*(struct cpu_info *)CPUINFO_VA)


#endif	/* _sparc_cpuvar_h */
