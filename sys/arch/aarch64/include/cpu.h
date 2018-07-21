/* $NetBSD: cpu.h,v 1.4 2018/07/21 13:08:35 ryo Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _AARCH64_CPU_H_
#define _AARCH64_CPU_H_

#ifdef __aarch64__

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#if defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/evcnt.h>
#include <aarch64/frame.h>

struct clockframe {
	struct trapframe cf_tf;
};

/* (spsr & 15) == SPSR_M_EL0T(64bit,0) or USER(32bit,0) */
#define CLKF_USERMODE(cf)	((((cf)->cf_tf.tf_spsr) & 0x0f) == 0)
#define CLKF_PC(cf)		((cf)->cf_tf.tf_pc)
#define CLKF_INTR(cf)		((void)(cf), curcpu()->ci_intr_depth > 1)

#include <sys/cpu_data.h>
#include <sys/device_if.h>
#include <sys/intr.h>

struct cpu_info {
	struct cpu_data ci_data;
	device_t ci_dev;
	cpuid_t ci_cpuid;
	struct lwp *ci_curlwp;
	struct lwp *ci_softlwps[SOFTINT_COUNT];

	uint64_t ci_lastintr;

	int ci_mtx_oldspl;
	int ci_mtx_count;

	int ci_want_resched;
	int ci_cpl;
	volatile u_int ci_softints;
	volatile u_int ci_astpending;
	volatile u_int ci_intr_depth;

	/* event counters */
	struct evcnt ci_vfp_use;
	struct evcnt ci_vfp_reuse;
	struct evcnt ci_vfp_save;
	struct evcnt ci_vfp_release;

	/* per cpu pmap private */
	uint64_t ci_pmap_flags;
};

static inline struct cpu_info *
curcpu(void)
{
	struct cpu_info *ci;
	__asm __volatile ("mrs %0, tpidr_el1" : "=r"(ci));
	return ci;
}
#define curlwp			(curcpu()->ci_curlwp)

#define setsoftast(ci)		atomic_or_uint(&(ci)->ci_astpending, __BIT(0))
#define cpu_signotify(l)	setsoftast((l)->l_cpu)
void cpu_set_curpri(int);
void cpu_proc_fork(struct proc *, struct proc *);
void cpu_need_proftick(struct lwp *l);
void cpu_boot_secondary_processors(void);
void cpu_hatch(struct cpu_info *);

extern struct cpu_info *cpu_info[];
extern volatile u_int arm_cpu_hatched;	/* MULTIPROCESSOR */
extern uint32_t cpus_midr[];		/* MULTIPROCESSOR */
extern uint64_t cpus_mpidr[];		/* MULTIPROCESSOR */

#define CPU_INFO_ITERATOR	cpuid_t
#ifdef MULTIPROCESSOR
#define cpu_number()		(curcpu()->ci_index)
#define CPU_IS_PRIMARY(ci)	((ci)->ci_index == 0)
#define CPU_INFO_FOREACH(cii, ci)					\
	cii = 0, ci = cpu_info[0];					\
	cii < (ncpu ? ncpu : 1) && (ci = cpu_info[cii]) != NULL;	\
	cii++
#else /* MULTIPROCESSOR */
#define cpu_number()		0
#define CPU_IS_PRIMARY(ci)	true
#define CPU_INFO_FOREACH(cii, ci)					\
	cii = 0, __USE(cii), ci = curcpu(); ci != NULL; ci = NULL
#endif /* MULTIPROCESSOR */


static inline void
cpu_dosoftints(void)
{
#if defined(__HAVE_FAST_SOFTINTS) && !defined(__HAVE_PIC_FAST_SOFTINTS)
	void dosoftints(void);
	struct cpu_info * const ci = curcpu();

	if (ci->ci_intr_depth == 0 && (ci->ci_softints >> ci->ci_cpl) > 0)
		dosoftints();
#endif
}

static inline bool
cpu_intr_p(void)
{
#ifdef __HAVE_PIC_FAST_SOFTINTS
	if (ci->ci_cpl < IPL_VM)
		return false;
#endif
	return curcpu()->ci_intr_depth > 0;
}

void	cpu_attach(device_t, cpuid_t);

#endif /* _KERNEL || _KMEMUSER */

#elif defined(__arm__)

#include <arm/cpu.h>

#endif

#endif /* _AARCH64_CPU_H_ */
