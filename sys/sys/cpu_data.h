/*	$NetBSD: cpu_data.h,v 1.54 2023/07/13 12:06:20 riastradh Exp $	*/

/*-
 * Copyright (c) 2004, 2006, 2007, 2008, 2019, 2020 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * based on arch/i386/include/cpu.h:
 *	NetBSD: cpu.h,v 1.115 2004/05/16 12:32:53 yamt Exp
 */

#ifndef _SYS_CPU_DATA_H_
#define	_SYS_CPU_DATA_H_

struct callout;
struct lwp;

#include <sys/sched.h>	/* for schedstate_percpu */
#include <sys/condvar.h>
#include <sys/pcu.h>
#include <sys/percpu_types.h>
#include <sys/queue.h>
#include <sys/kcpuset.h>
#include <sys/ipi.h>
#include <sys/intr.h>

/* Per-CPU counters.  New elements must be added in blocks of 8. */
enum cpu_count {
	CPU_COUNT_NSWTCH,		/* 0 */
	CPU_COUNT_NSYSCALL,
	CPU_COUNT_NTRAP,
	CPU_COUNT_NINTR,
	CPU_COUNT_NSOFT,
	CPU_COUNT_FORKS,
	CPU_COUNT_FORKS_PPWAIT,
	CPU_COUNT_FORKS_SHAREVM,
	CPU_COUNT_COLORHIT,		/* 8 */
	CPU_COUNT_COLORMISS,
	CPU_COUNT__UNUSED3,
	CPU_COUNT__UNUSED4,
	CPU_COUNT_CPUHIT,
	CPU_COUNT_CPUMISS,
	CPU_COUNT_FREEPAGES,
	CPU_COUNT__UNUSED5,
	CPU_COUNT_PAGEINS,		/* 16 */
	CPU_COUNT_FLTUP,
	CPU_COUNT_FLTNOUP,
	CPU_COUNT_FLTPGWAIT,
	CPU_COUNT_FLTRELCK,
	CPU_COUNT_FLTRELCKOK,
	CPU_COUNT__UNUSED1,
	CPU_COUNT__UNUSED2,
	CPU_COUNT_NFAULT,		/* 24 */
	CPU_COUNT_FLT_ACOW,
	CPU_COUNT_FLT_ANON,
	CPU_COUNT_FLT_OBJ,
	CPU_COUNT_FLT_PRCOPY,
	CPU_COUNT_FLT_PRZERO,
	CPU_COUNT_FLTAMCOPY,
	CPU_COUNT_FLTANGET,
	CPU_COUNT_FLTANRETRY,		/* 32 */
	CPU_COUNT_FLTGET,
	CPU_COUNT_FLTLGET,
	CPU_COUNT_FLTNAMAP,
	CPU_COUNT_FLTNOMAP,
	CPU_COUNT_FLTNOANON,
	CPU_COUNT_FLTNORAM,
	CPU_COUNT_FLTPGRELE,
	CPU_COUNT_ANONUNKNOWN,		/* 40 */
	CPU_COUNT_ANONCLEAN,
	CPU_COUNT_ANONDIRTY,
	CPU_COUNT_FILEUNKNOWN,
	CPU_COUNT_FILECLEAN,
	CPU_COUNT_FILEDIRTY,
	CPU_COUNT_EXECPAGES,
	CPU_COUNT_SYNC,
	CPU_COUNT_MAX			/* 48 */
};

/*
 * MI per-cpu data
 *
 * this structure is intended to be included in MD cpu_info structure.
 *	struct cpu_info {
 *		struct cpu_data ci_data;
 *	}
 *
 * note that cpu_data is not expected to contain much data,
 * as cpu_info is size-limited on most ports.
 */

struct lockdebug;

enum cpu_rel {
	/*
	 * This is a circular list of peer CPUs in the same core (SMT /
	 * Hyperthreading).  It always includes the CPU it is referenced
	 * from as the last entry.
	 */
	CPUREL_CORE,

	/*
	 * This is a circular list of peer CPUs in the same physical
	 * package.  It always includes the CPU it is referenced from as
	 * the last entry.
	 */
	CPUREL_PACKAGE,

	/*
	 * This is a circular list of the first CPUs in each physical
	 * package.  It may or may not include the CPU it is referenced
	 * from.
	 */
	CPUREL_PACKAGE1ST,

	/* Terminator. */
	CPUREL_COUNT
};

struct cpu_data {
	/*
	 * The first section is likely to be touched by other CPUs -
	 * it is cache hot.
	 */
	u_int		cpu_index;		/* CPU index */
	lwp_t		*cpu_biglock_wanted;	/* LWP spinning on biglock */
	kcondvar_t	cpu_xcall;		/* cross-call support */
	int		cpu_xcall_pending;	/* cross-call support */
	u_int		cpu_psz_read_depth;	/* pserialize(9) read depth */
	uint32_t	cpu_ipipend[IPI_BITWORDS];	/* pending IPIs */
	struct schedstate_percpu cpu_schedstate; /* scheduler state */

	/* Basic topology information.  May be fake. */
	u_int		cpu_package_id;
	u_int		cpu_core_id;
	u_int		cpu_smt_id;
	u_int		cpu_numa_id;
	bool		cpu_is_slow;
	u_int		cpu_nsibling[CPUREL_COUNT];
	struct cpu_info	*cpu_sibling[CPUREL_COUNT];
	struct cpu_info *cpu_package1st;	/* 1st CPU in our package */

	/*
	 * This section is mostly CPU-private.
	 */
	lwp_t		*cpu_idlelwp __aligned(64);/* idle lwp */
	void		*cpu_lockstat;		/* lockstat private tables */
	u_int		cpu_biglock_count;	/* # recursive holds */
	u_int		cpu_spin_locks;		/* # of spinlockmgr locks */
	u_int		cpu_simple_locks;	/* # of simple locks held */
	u_int		cpu_spin_locks2;	/* # of spin locks held XXX */
	u_int		cpu_lkdebug_recurse;	/* LOCKDEBUG recursion */
	u_int		cpu_softints;		/* pending (slow) softints */
	struct uvm_cpu	*cpu_uvm;		/* uvm per-cpu data */
	u_int		cpu_faultrng;		/* counter for fault rng */
	void		*cpu_callout;		/* per-CPU callout state */
	void		*cpu_softcpu;		/* soft interrupt table */
	TAILQ_HEAD(,buf) cpu_biodone;		/* finished block xfers */
	percpu_cpu_t	cpu_percpu;		/* per-cpu data */
	struct selcluster *cpu_selcluster;	/* per-CPU select() info */
	void		*cpu_nch;		/* per-cpu vfs_cache data */
	_TAILQ_HEAD(,struct lockdebug,volatile) cpu_ld_locks;/* !: lockdebug */
	__cpu_simple_lock_t cpu_ld_lock;	/* lockdebug */
	uint64_t	cpu_cc_freq;		/* cycle counter frequency */
	int64_t		cpu_cc_skew;		/* counter skew vs cpu0 */
	char		cpu_name[8];		/* eg, "cpu4" */
	kcpuset_t	*cpu_kcpuset;		/* kcpuset_t of this cpu only */
	struct lwp * volatile cpu_pcu_curlwp[PCU_UNIT_COUNT];
	int64_t		cpu_counts[CPU_COUNT_MAX];/* per-CPU counts */

	unsigned	cpu_heartbeat_count;		/* # of heartbeats */
	unsigned	cpu_heartbeat_uptime_cache;	/* last time_uptime */
	unsigned	cpu_heartbeat_uptime_stamp;	/* heartbeats since
							 * uptime changed */
};

#define	ci_schedstate		ci_data.cpu_schedstate
#define	ci_index		ci_data.cpu_index
#define	ci_biglock_count	ci_data.cpu_biglock_count
#define	ci_biglock_wanted	ci_data.cpu_biglock_wanted
#define	ci_cpuname		ci_data.cpu_name
#define	ci_spin_locks		ci_data.cpu_spin_locks
#define	ci_simple_locks		ci_data.cpu_simple_locks
#define	ci_lockstat		ci_data.cpu_lockstat
#define	ci_spin_locks2		ci_data.cpu_spin_locks2
#define	ci_lkdebug_recurse	ci_data.cpu_lkdebug_recurse
#define	ci_pcu_curlwp		ci_data.cpu_pcu_curlwp
#define	ci_kcpuset		ci_data.cpu_kcpuset
#define	ci_ipipend		ci_data.cpu_ipipend
#define	ci_psz_read_depth	ci_data.cpu_psz_read_depth

#define	ci_package_id		ci_data.cpu_package_id
#define	ci_core_id		ci_data.cpu_core_id
#define	ci_smt_id		ci_data.cpu_smt_id
#define	ci_numa_id		ci_data.cpu_numa_id
#define	ci_is_slow		ci_data.cpu_is_slow
#define	ci_nsibling		ci_data.cpu_nsibling
#define	ci_sibling		ci_data.cpu_sibling
#define	ci_package1st		ci_data.cpu_package1st
#define	ci_faultrng		ci_data.cpu_faultrng
#define	ci_counts		ci_data.cpu_counts

#define	ci_heartbeat_count		ci_data.cpu_heartbeat_count
#define	ci_heartbeat_uptime_cache	ci_data.cpu_heartbeat_uptime_cache
#define	ci_heartbeat_uptime_stamp	ci_data.cpu_heartbeat_uptime_stamp

#define	cpu_nsyscall		cpu_counts[CPU_COUNT_NSYSCALL]
#define	cpu_ntrap		cpu_counts[CPU_COUNT_NTRAP]
#define	cpu_nswtch		cpu_counts[CPU_COUNT_NSWTCH]
#define	cpu_nintr		cpu_counts[CPU_COUNT_NINTR]
#define	cpu_nsoft		cpu_counts[CPU_COUNT_NSOFT]
#define	cpu_nfault		cpu_counts[CPU_COUNT_NFAULT]

void	mi_cpu_init(void);
int	mi_cpu_attach(struct cpu_info *);

/*
 * Adjust a count with preemption already disabled.  If the counter being
 * adjusted can be updated from interrupt context, SPL must be raised.
 */
#define	CPU_COUNT(idx, d)					\
do {								\
	extern bool kpreempt_disabled(void);			\
	KASSERT(kpreempt_disabled());				\
	KASSERT((unsigned)idx < CPU_COUNT_MAX);			\
	curcpu()->ci_counts[(idx)] += (d);			\
} while (/* CONSTCOND */ 0)

/*
 * Fetch a potentially stale count - cheap, use as often as you like.
 */
static inline int64_t
cpu_count_get(enum cpu_count idx)
{
	extern int64_t cpu_counts[];
	return cpu_counts[idx];
}

void	cpu_count(enum cpu_count, int64_t);
void	cpu_count_sync(bool);

#endif /* _SYS_CPU_DATA_H_ */
