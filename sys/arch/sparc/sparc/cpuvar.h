/*	$NetBSD: cpuvar.h,v 1.95 2018/01/16 08:23:17 mrg Exp $ */

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
#include "opt_modular.h"
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
#define CPUFLG_HATCHED		0x1000	/* CPU is alive */
#define CPUFLG_PAUSED		0x2000	/* CPU is paused */
#define CPUFLG_GOTMSG		0x4000	/* CPU got an lev13 IPI */


/*
 * Provide two forms of CPU_INFO_FOREACH.  One fast one for non-modular
 * non-SMP kernels, and the other for everyone else.  Both work in the
 * non-SMP case, just involving an extra indirection through cpus[0] for
 * the portable version.
 */
#if defined(MULTIPROCESSOR) || defined(MODULAR) || defined(_MODULE)
#define	CPU_INFO_FOREACH(cii, cp)	cii = 0; (cp = cpus[cii]) && cp->eintstack && cii < sparc_ncpus; cii++
#define CPU_INFO_ITERATOR		int
#else
#define CPU_INFO_FOREACH(cii, cp)	cp = curcpu(); cp != NULL; cp = NULL
#define CPU_INFO_ITERATOR		int __unused
#endif


/*
 * Related function prototypes
 */
void getcpuinfo (struct cpu_info *sc, int node);
void mmu_install_tables (struct cpu_info *);
void pmap_alloc_cpu (struct cpu_info *);

#define	CPUSET_ALL	0xffffffffU	/* xcall to all configured CPUs */

#if defined(MULTIPROCESSOR)
void cpu_init_system(void);
typedef void (*xcall_func_t)(int, int, int);
typedef void (*xcall_trap_t)(int, int, int);
void xcall(xcall_func_t, xcall_trap_t, int, int, int, u_int);
/* from intr.c */
void xcallintr(void *);
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

extern struct cpu_info *cpus[];
#ifdef MULTIPROCESSOR
extern u_int cpu_ready_mask;		/* the set of CPUs marked as READY */
#endif

#if defined(DDB) || defined(MULTIPROCESSOR)
/*
 * These are called by ddb mach functions.
 */
void cpu_debug_dump(void);
void cpu_xcall_dump(void);
#endif

#endif	/* _sparc_cpuvar_h */
