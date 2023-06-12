/* $NetBSD: cpu.h,v 1.12 2023/06/12 19:04:14 skrll Exp $ */

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

#ifndef _RISCV_CPU_H_
#define _RISCV_CPU_H_

#if defined(_KERNEL) || defined(_KMEMUSER)

struct clockframe {
	vaddr_t cf_epc;
	register_t cf_status;
	int cf_intr_depth;
};

#define CLKF_USERMODE(cf)	(((cf)->cf_status & SR_SPP) == 0)
#define CLKF_PC(cf)		((cf)->cf_epc)
#define CLKF_INTR(cf)		((cf)->cf_intr_depth > 1)

#include <sys/cpu_data.h>
#include <sys/device_if.h>
#include <sys/evcnt.h>
#include <sys/intr.h>

struct cpu_info {
	struct cpu_data ci_data;
	device_t ci_dev;
	cpuid_t ci_cpuid;
	struct lwp *ci_curlwp;
	struct lwp *ci_onproc;		/* current user LWP / kthread */
	struct lwp *ci_softlwps[SOFTINT_COUNT];
	struct trapframe *ci_ddb_regs;

	uint64_t ci_lastintr;
	uint64_t ci_lastintr_scheduled;
	struct evcnt ci_ev_timer;
	struct evcnt ci_ev_timer_missed;

	u_long ci_cpu_freq;		/* CPU frequency */
	int ci_mtx_oldspl;
	int ci_mtx_count;
	int ci_cpl;
	volatile u_int ci_intr_depth;

	int ci_want_resched __aligned(COHERENCY_UNIT);
	u_int ci_softints;

	tlb_asid_t ci_pmap_asid_cur;

	union pmap_segtab *ci_pmap_user_segtab;
#ifdef _LP64
	union pmap_segtab *ci_pmap_user_seg0tab;
#endif

	struct evcnt ci_ev_fpu_saves;
	struct evcnt ci_ev_fpu_loads;
	struct evcnt ci_ev_fpu_reenables;

	struct pmap_tlb_info *ci_tlb_info;

#ifdef MULTIPROCESSOR

	volatile u_long ci_flags;
	volatile u_long ci_request_ipis;
					/* bitmask of IPIs requested */
	u_long ci_active_ipis;	/* bitmask of IPIs being serviced */

	struct evcnt ci_evcnt_all_ipis;	/* aggregated IPI counter */
	struct evcnt ci_evcnt_per_ipi[NIPIS];	/* individual IPI counters */
	struct evcnt ci_evcnt_synci_onproc_rqst;
	struct evcnt ci_evcnt_synci_deferred_rqst;
	struct evcnt ci_evcnt_synci_ipi_rqst;

#define	CPUF_PRIMARY	__BIT(0)		/* CPU is primary CPU */
#define	CPUF_PRESENT	__BIT(1)		/* CPU is present */
#define	CPUF_RUNNING	__BIT(2)		/* CPU is running */
#define	CPUF_PAUSED	__BIT(3)		/* CPU is paused */
#define	CPUF_USERPMAP	__BIT(4)		/* CPU has a user pmap activated */
	kcpuset_t *ci_shootdowncpus;
	kcpuset_t *ci_multicastcpus;
	kcpuset_t *ci_watchcpus;
	kcpuset_t *ci_ddbcpus;
#endif

#if defined(GPROF) && defined(MULTIPROCESSOR)
	struct gmonparam *ci_gmon;	/* MI per-cpu GPROF */
#endif
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern struct cpu_info *cpu_info[];
extern struct cpu_info cpu_info_store[];


#ifdef MULTIPROCESSOR
extern u_int riscv_cpu_max;
extern cpuid_t cpu_hartid[];

void cpu_hatch(struct cpu_info *);

void cpu_init_secondary_processor(int);
void cpu_boot_secondary_processors(void);
void cpu_mpstart(void);
bool cpu_hatched_p(u_int);

void cpu_clr_mbox(int);
void cpu_set_hatched(int);


void	cpu_halt(void);
void	cpu_halt_others(void);
bool	cpu_is_paused(cpuid_t);
void	cpu_pause(void);
void	cpu_pause_others(void);
void	cpu_resume(cpuid_t);
void	cpu_resume_others(void);
void	cpu_debug_dump(void);

extern kcpuset_t *cpus_running;
extern kcpuset_t *cpus_hatched;
extern kcpuset_t *cpus_paused;
extern kcpuset_t *cpus_resumed;
extern kcpuset_t *cpus_halted;

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */

/*
 * Send an inter-processor interrupt to each other CPU (excludes curcpu())
 */
void cpu_broadcast_ipi(int);

/*
 * Send an inter-processor interrupt to CPUs in kcpuset (excludes curcpu())
 */
void cpu_multicast_ipi(const kcpuset_t *, int);

/*
 * Send an inter-processor interrupt to another CPU.
 */
int cpu_send_ipi(struct cpu_info *, int);

#endif

struct lwp;
static inline struct cpu_info *lwp_getcpu(struct lwp *);

register struct lwp *riscv_curlwp __asm("tp");
#define	curlwp		riscv_curlwp
#define	curcpu()	lwp_getcpu(curlwp)
#define	curpcb		((struct pcb *)lwp_getpcb(curlwp))

static inline cpuid_t
cpu_number(void)
{
#ifdef MULTIPROCESSOR
	return curcpu()->ci_cpuid;
#else
	return 0;
#endif
}

void	cpu_proc_fork(struct proc *, struct proc *);
void	cpu_signotify(struct lwp *);
void	cpu_need_proftick(struct lwp *l);
void	cpu_boot_secondary_processors(void);

#define CPU_INFO_ITERATOR	cpuid_t
#ifdef MULTIPROCESSOR
#define	CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)
#define	CPU_INFO_FOREACH(cii, ci)		\
    cii = 0, ci = &cpu_info_store[0]; 		\
    ci != NULL; 				\
    cii++, ncpu ? (ci = cpu_infos[cii]) 	\
		: (ci = NULL)
#else
#define CPU_IS_PRIMARY(ci)	true
#define CPU_INFO_FOREACH(cii, ci) \
	(cii) = 0, (ci) = curcpu(); (cii) == 0; (cii)++
#endif

#define CPU_INFO_CURPMAP(ci)	(curlwp->l_proc->p_vmspace->vm_map.pmap)

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

#define LWP_PC(l)	cpu_lwp_pc(l)

vaddr_t	cpu_lwp_pc(struct lwp *);

static inline void
cpu_idle(void)
{
	asm volatile("wfi" ::: "memory");
}

#endif /* _KERNEL */

#endif /* _RISCV_CPU_H_ */
