/* $NetBSD: cpu.h,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $ */

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

#if defined(_KERNEL) || defined(_KMEMUSER)
struct clockframe {
	uintptr_t cf_pc;
	uint32_t cf_psr;
	int cf_intr_depth;
};

#define CLKF_USERMODE(cf)	(((cf)->cf_psr & 0x0f) == 0)
#define CLKF_PC(cf)		((cf)->cf_pc)
#define CLKF_INTR(cf)		((cf)->cf_intr_depth > 0)

#include <sys/cpu_data.h>
#include <sys/device_if.h>
#include <sys/intr.h>

struct cpu_info {
	struct cpu_data ci_data;
	device_t ci_dev;
	cpuid_t ci_cpuid;
	cpuid_t ci_gicid;		// used for IPIs
	struct lwp *ci_curlwp;
	struct lwp *ci_softlwps[SOFTINT_COUNT];

	uint64_t ci_lastintr;

	int ci_mtx_oldspl;
	int ci_mtx_count;

	int ci_want_resched;
	int ci_cpl;
	u_int ci_softints;
	volatile u_int ci_astpending;
	volatile u_int ci_intr_depth;
};

static inline struct cpu_info *
curcpu(void)
{
	struct cpu_info *ci;
	__asm __volatile("mrs %0, tpidr_el1" : "=r"(ci));
	return ci;
}

#define curlwp		(curcpu()->ci_curlwp)

static inline cpuid_t
cpu_number(void)
{
	return curcpu()->ci_gicid;
}

void	cpu_set_curpri(int);
void	cpu_proc_fork(struct proc *, struct proc *);
void	cpu_signotify(struct lwp *);
void	cpu_need_proftick(struct lwp *l);
void	cpu_boot_secondary_processors(void);

#define CPU_INFO_ITERATOR	cpuid_t
#define CPU_INFO_FOREACH(cii, ci) \
	(cii) = 0; ((ci) = cpu_infos[cii]) != NULL; (cii)++

static inline void
cpu_dosoftints(void)
{
	extern void dosoftints(void);
        struct cpu_info * const ci = curcpu();
        if (ci->ci_intr_depth == 0
	    && (ci->ci_data.cpu_softints >> ci->ci_cpl) > 0)
                dosoftints();
}

static inline bool
cpu_intr_p(void)
{
	return curcpu()->ci_intr_depth > 0;
}

#endif /* _KERNEL || _KMEMUSER */

#elif defined(__arm__)

#include <arm/cpu.h>

#endif

#endif /* _AARCH64_CPU_H_ */
