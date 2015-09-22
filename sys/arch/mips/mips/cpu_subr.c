/*	$NetBSD: cpu_subr.c,v 1.17.4.2 2015/09/22 12:05:47 skrll Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.17.4.2 2015/09/22 12:05:47 skrll Exp $");

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/bitops.h>
#include <sys/idle.h>
#include <sys/xcall.h>
#include <sys/kernel.h>
#include <sys/ipi.h>

#include <uvm/uvm.h>

#include <mips/locore.h>
#include <mips/regnum.h>
#include <mips/pcb.h>
#include <mips/cache.h>
#include <mips/frame.h>
#include <mips/userret.h>
#include <mips/pte.h>

#if defined(DDB) || defined(KGDB)
#ifdef DDB
#include <mips/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#endif
#endif

#ifdef MIPS64_OCTEON
extern struct cpu_softc octeon_cpu0_softc;
#endif

struct cpu_info cpu_info_store
#if defined(MULTIPROCESSOR) && !defined(MIPS64_OCTEON)
	__section(".data1")
	__aligned(1LU << ilog2((2*sizeof(struct cpu_info)-1)))
#endif
    = {
	.ci_curlwp = &lwp0,
	.ci_tlb_info = &pmap_tlb0_info,
	.ci_pmap_user_segtab = (void *)(MIPS_KSEG2_START + 0x1eadbeef),
#ifdef _LP64
	.ci_pmap_user_seg0tab = (void *)(MIPS_KSEG2_START + 0x1eadbeef),
#endif
	.ci_cpl = IPL_HIGH,
	.ci_tlb_slot = -1,
#ifdef MULTIPROCESSOR
	.ci_flags = CPUF_PRIMARY|CPUF_PRESENT|CPUF_RUNNING,
#endif
#ifdef MIPS64_OCTEON
	.ci_softc = &octeon_cpu0_softc,
#endif
};

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &mips_fpu_ops,
#if (MIPS32R2 + MIPS64R2) > 0
	[PCU_DSP] = &mips_dsp_ops,
#endif
};

#ifdef MULTIPROCESSOR
struct cpu_info * cpuid_infos[MAXCPUS] = {
	[0] = &cpu_info_store,
};

kcpuset_t *cpus_halted;
kcpuset_t *cpus_hatched;
kcpuset_t *cpus_paused;
kcpuset_t *cpus_resumed;
kcpuset_t *cpus_running;

static void cpu_ipi_wait(const char *, const kcpuset_t *, const kcpuset_t *);

struct cpu_info *
cpu_info_alloc(struct pmap_tlb_info *ti, cpuid_t cpu_id, cpuid_t cpu_package_id,
	cpuid_t cpu_core_id, cpuid_t cpu_smt_id)
{
	KASSERT(cpu_id < MAXCPUS);

#ifdef MIPS64_OCTEON
	vaddr_t exc_page = MIPS_UTLB_MISS_EXC_VEC + 0x1000*cpu_id;
	__CTASSERT(sizeof(struct cpu_info) + sizeof(struct pmap_tlb_info) <= 0x1000 - 0x280);

	struct cpu_info * const ci = ((struct cpu_info *)(exc_page + 0x1000)) - 1;
	memset((void *)exc_page, 0, PAGE_SIZE);

	if (ti == NULL) {
		ti = ((struct pmap_tlb_info *)ci) - 1;
		pmap_tlb_info_init(ti);
	}
#else
	const vaddr_t cpu_info_offset = (vaddr_t)&cpu_info_store & PAGE_MASK;
	struct pglist pglist;
	int error;

	/*
	* Grab a page from the first 512MB (mappable by KSEG0) to use to store
	* exception vectors and cpu_info for this cpu.
	*/
	error = uvm_pglistalloc(PAGE_SIZE,
	    0, MIPS_KSEG1_START - MIPS_KSEG0_START,
	    PAGE_SIZE, PAGE_SIZE, &pglist, 1, false);
	if (error)
		return NULL;

	const paddr_t pa = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
	const vaddr_t va = MIPS_PHYS_TO_KSEG0(pa);
	struct cpu_info * const ci = (void *) (va + cpu_info_offset);
	memset((void *)va, 0, PAGE_SIZE);

	/*
	 * If we weren't passed a pmap_tlb_info to use, the caller wants us
	 * to take care of that for him.  Since we have room left over in the
	 * page we just allocated, just use a piece of that for it.
	 */
	if (ti == NULL) {
		if (cpu_info_offset >= sizeof(*ti)) {
			ti = (void *) va;
		} else {
			KASSERT(PAGE_SIZE - cpu_info_offset + sizeof(*ci) >= sizeof(*ti));
			ti = (struct pmap_tlb_info *)(va + PAGE_SIZE) - 1;
		}
		pmap_tlb_info_init(ti);
	}

	/*
	 * Attach its TLB info (which must be direct-mapped)
	 */
#ifdef _LP64
	KASSERT(MIPS_KSEG0_P(ti) || MIPS_XKPHYS_P(ti));
#else
	KASSERT(MIPS_KSEG0_P(ti));
#endif
#endif /* MIPS64_OCTEON */

	KASSERT(cpu_id != 0);
	ci->ci_cpuid = cpu_id;
	ci->ci_data.cpu_package_id = cpu_package_id;
	ci->ci_data.cpu_core_id = cpu_core_id;
	ci->ci_data.cpu_smt_id = cpu_smt_id;
	ci->ci_cpu_freq = cpu_info_store.ci_cpu_freq;
	ci->ci_cctr_freq = cpu_info_store.ci_cctr_freq;
        ci->ci_cycles_per_hz = cpu_info_store.ci_cycles_per_hz;
        ci->ci_divisor_delay = cpu_info_store.ci_divisor_delay;
        ci->ci_divisor_recip = cpu_info_store.ci_divisor_recip;
	ci->ci_cpuwatch_count = cpu_info_store.ci_cpuwatch_count;

#ifndef _LP64
	/*
	 * If we have more memory than can be mapped by KSEG0, we need to
	 * allocate enough VA so we can map pages with the right color
	 * (to avoid cache alias problems).
	 */
	if (pmap_limits.avail_end > MIPS_KSEG1_START - MIPS_KSEG0_START) {
		ci->ci_pmap_dstbase = uvm_km_alloc(kernel_map,
		    uvmexp.ncolors * PAGE_SIZE, 0, UVM_KMF_VAONLY);
		KASSERT(ci->ci_pmap_dstbase);
		ci->ci_pmap_srcbase = uvm_km_alloc(kernel_map,
		    uvmexp.ncolors * PAGE_SIZE, 0, UVM_KMF_VAONLY);
		KASSERT(ci->ci_pmap_srcbase);
	}
#endif

	mi_cpu_attach(ci);

	pmap_tlb_info_attach(ti, ci);

	return ci;
}
#endif /* MULTIPROCESSOR */

static void
cpu_hwrena_setup(void)
{
#if (MIPS32R2 + MIPS64R2) > 0
	const int cp0flags = mips_options.mips_cpu->cpu_cp0flags;
	if ((cp0flags & MIPS_CP0FL_USE) == 0)
		return;

	if (cp0flags & MIPS_CP0FL_HWRENA) {
		mipsNN_cp0_hwrena_write(
		    MIPS_HWRENA_UL
		    |MIPS_HWRENA_CCRES
		    |MIPS_HWRENA_CC
		    |MIPS_HWRENA_SYNCI_STEP
		    |MIPS_HWRENA_CPUNUM);
		if (cp0flags & MIPS_CP0FL_USERLOCAL) {
			mipsNN_cp0_userlocal_write(curlwp->l_private);
		}
	}
#endif
}

void
cpu_attach_common(device_t self, struct cpu_info *ci)
{
	const char * const xname = device_xname(self);

	/*
	 * Cross link cpu_info and its device together
	 */
	ci->ci_dev = self;
	self->dv_private = ci;
	KASSERT(ci->ci_idepth == 0);

	evcnt_attach_dynamic(&ci->ci_ev_count_compare,
		EVCNT_TYPE_INTR, NULL, xname,
		"int 5 (clock)");
	evcnt_attach_dynamic(&ci->ci_ev_count_compare_missed,
		EVCNT_TYPE_INTR, NULL, xname,
		"int 5 (clock) missed");
	evcnt_attach_dynamic(&ci->ci_ev_fpu_loads,
		EVCNT_TYPE_MISC, NULL, xname,
		"fpu loads");
	evcnt_attach_dynamic(&ci->ci_ev_fpu_saves,
		EVCNT_TYPE_MISC, NULL, xname,
		"fpu saves");
	evcnt_attach_dynamic(&ci->ci_ev_dsp_loads,
		EVCNT_TYPE_MISC, NULL, xname,
		"dsp loads");
	evcnt_attach_dynamic(&ci->ci_ev_dsp_saves,
		EVCNT_TYPE_MISC, NULL, xname,
		"dsp saves");
	evcnt_attach_dynamic(&ci->ci_ev_tlbmisses,
		EVCNT_TYPE_TRAP, NULL, xname,
		"tlb misses");

#ifdef MULTIPROCESSOR
	if (ci != &cpu_info_store) {
		/*
		 * Tail insert this onto the list of cpu_info's.
		 */
		KASSERT(cpuid_infos[ci->ci_cpuid] == NULL);
		cpuid_infos[ci->ci_cpuid] = ci;
		membar_producer();
	}
	KASSERT(cpuid_infos[ci->ci_cpuid] != NULL);
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_activate_rqst,
	    EVCNT_TYPE_MISC, NULL, xname,
	    "syncicache activate request");
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_deferred_rqst,
	    EVCNT_TYPE_MISC, NULL, xname,
	    "syncicache deferred request");
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_ipi_rqst,
	    EVCNT_TYPE_MISC, NULL, xname,
	    "syncicache ipi request");
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_onproc_rqst,
	    EVCNT_TYPE_MISC, NULL, xname,
	    "syncicache onproc request");

	/*
	 * Initialize IPI framework for this cpu instance
	 */
	ipi_init(ci);
#endif
}

void
cpu_startup_common(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];	/* "99999 MB" */

	pmap_tlb_info_evcnt_attach(&pmap_tlb0_info);

#ifdef MULTIPROCESSOR
	kcpuset_create(&cpus_halted, true);
		KASSERT(cpus_halted != NULL);
	kcpuset_create(&cpus_hatched, true);
		KASSERT(cpus_hatched != NULL);
	kcpuset_create(&cpus_paused, true);
		KASSERT(cpus_paused != NULL);
	kcpuset_create(&cpus_resumed, true);
		KASSERT(cpus_resumed != NULL);
	kcpuset_create(&cpus_running, true);
		KASSERT(cpus_running != NULL);
	kcpuset_set(cpus_hatched, cpu_number());
	kcpuset_set(cpus_running, cpu_number());
#endif

	cpu_hwrena_setup();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_getmodel());
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG/XKPHYS to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe *tf = l->l_md.md_utf;
	__greg_t *gr = mcp->__gregs;
	__greg_t ras_pc;

	/* Save register context. Dont copy R0 - it is always 0 */
	memcpy(&gr[_REG_AT], &tf->tf_regs[_R_AST], sizeof(mips_reg_t) * 31);

	gr[_REG_MDLO]  = tf->tf_regs[_R_MULLO];
	gr[_REG_MDHI]  = tf->tf_regs[_R_MULHI];
	gr[_REG_CAUSE] = tf->tf_regs[_R_CAUSE];
	gr[_REG_EPC]   = tf->tf_regs[_R_PC];
	gr[_REG_SR]    = tf->tf_regs[_R_SR];
	mcp->_mc_tlsbase = (intptr_t)l->l_private;

	if ((ras_pc = (intptr_t)ras_lookup(l->l_proc,
	    (void *) (intptr_t)gr[_REG_EPC])) != -1)
		gr[_REG_EPC] = ras_pc;

	*flags |= _UC_CPU | _UC_TLSBASE;

	/* Save floating point register context, if any. */
	KASSERT(l == curlwp);
	if (fpu_used_p()) {
		size_t fplen;
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 */
		fpu_save();

		/*
		 * The PCB FP regs struct includes the FP CSR, so use the
		 * size of __fpregs.__fp_r when copying.
		 */
#if !defined(__mips_o32)
		if (_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi)) {
#endif
			fplen = sizeof(struct fpreg);
#if !defined(__mips_o32)
		} else {
			fplen = sizeof(struct fpreg_oabi);
		}
#endif
		struct pcb * const pcb = lwp_getpcb(l);
		memcpy(&mcp->__fpregs, &pcb->pcb_fpregs, fplen);
		*flags |= _UC_FPU;
	}
}

int
cpu_mcontext_validate(struct lwp *l, const mcontext_t *mcp)
{
	/* XXX:  Do we validate the addresses?? */
	return 0;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_utf;
	struct proc *p = l->l_proc;
	const __greg_t *gr = mcp->__gregs;
	int error;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		error = cpu_mcontext_validate(l, mcp);
		if (error)
			return error;

		/* Save register context. */

#ifdef __mips_n32
		CTASSERT(_R_AST == _REG_AT);
		if (__predict_false(p->p_md.md_abi == _MIPS_BSD_API_O32)) {
			const mcontext_o32_t *mcp32 = (const mcontext_o32_t *)mcp;
			const __greg32_t *gr32 = mcp32->__gregs;
			for (size_t i = _R_AST; i < 32; i++) {
				tf->tf_regs[i] = gr32[i];
			}
		} else
#endif
		memcpy(&tf->tf_regs[_R_AST], &gr[_REG_AT],
		       sizeof(mips_reg_t) * 31);

		tf->tf_regs[_R_MULLO] = gr[_REG_MDLO];
		tf->tf_regs[_R_MULHI] = gr[_REG_MDHI];
		tf->tf_regs[_R_CAUSE] = gr[_REG_CAUSE];
		tf->tf_regs[_R_PC]    = gr[_REG_EPC];
		/* Do not restore SR. */
	}

	/* Restore the private thread context */
	if (flags & _UC_TLSBASE) {
		lwp_setprivate(l, (void *)(intptr_t)mcp->_mc_tlsbase);
	}

	/* Restore floating point register context, if any. */
	if (flags & _UC_FPU) {
		size_t fplen;

		/* Disable the FPU contents. */
		fpu_discard();

#if !defined(__mips_o32)
		if (_MIPS_SIM_NEWABI_P(l->l_proc->p_md.md_abi)) {
#endif
			fplen = sizeof(struct fpreg);
#if !defined(__mips_o32)
		} else {
			fplen = sizeof(struct fpreg_oabi);
		}
#endif
		/*
		 * The PCB FP regs struct includes the FP CSR, so use the
		 * proper size of fpreg when copying.
		 */
		struct pcb * const pcb = lwp_getpcb(l);
		memcpy(&pcb->pcb_fpregs, &mcp->__fpregs, fplen);
	}

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return (0);
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	struct lwp * const l = ci->ci_data.cpu_onproc;
#ifdef MULTIPROCESSOR
	struct cpu_info * const cur_ci = curcpu();
#endif

	KASSERT(kpreempt_disabled());

	ci->ci_want_resched |= flags;

	if (__predict_false((l->l_pflag & LP_INTR) != 0)) {
		/*
		 * No point doing anything, it will switch soon.
		 * Also here to prevent an assertion failure in
		 * kpreempt() due to preemption being set on a
		 * soft interrupt LWP.
		 */
		return;
	}

	if (__predict_false(l == ci->ci_data.cpu_idlelwp)) {
#ifdef MULTIPROCESSOR
		/*
		 * If the other CPU is idling, it must be waiting for an
		 * interrupt.  So give it one.
		 */
		if (__predict_false(ci != cur_ci))
			cpu_send_ipi(ci, IPI_NOP);
#endif
		return;
	}

#ifdef MULTIPROCESSOR
	atomic_or_uint(&ci->ci_want_resched, flags);
#else
	ci->ci_want_resched |= flags;
#endif

	if (flags & RESCHED_KPREEMPT) {
#ifdef __HAVE_PREEMPTION
		atomic_or_uint(&l->l_dopreempt, DOPREEMPT_ACTIVE);
		if (ci == cur_ci) {
			softint_trigger(SOFTINT_KPREEMPT);
                } else {
                        cpu_send_ipi(ci, IPI_KPREEMPT);
                }
#endif
		return;
	}
	l->l_md.md_astpending = 1;		/* force call to ast() */
#ifdef MULTIPROCESSOR
	if (ci != cur_ci && (flags & RESCHED_IMMED)) {
		cpu_send_ipi(ci, IPI_AST);
	}
#endif
}

void
cpu_signotify(struct lwp *l)
{
	KASSERT(kpreempt_disabled());
#ifdef __HAVE_FAST_SOFTINTS
	KASSERT(lwp_locked(l, NULL));
#endif
	KASSERT(l->l_stat == LSONPROC || l->l_stat == LSRUN || l->l_stat == LSSTOP);

	l->l_md.md_astpending = 1; 		/* force call to ast() */
}

void
cpu_need_proftick(struct lwp *l)
{
	KASSERT(kpreempt_disabled());
	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	l->l_md.md_astpending = 1;		/* force call to ast() */
}

void
cpu_set_curpri(int pri)
{
	kpreempt_disable();
	curcpu()->ci_schedstate.spc_curpriority = pri;
	kpreempt_enable();
}


#ifdef __HAVE_PREEMPTION
bool
cpu_kpreempt_enter(uintptr_t where, int s)
{
        KASSERT(kpreempt_disabled());

#if 0
	if (where == (intptr_t)-2) {
		KASSERT(curcpu()->ci_mtx_count == 0);
		/*
		 * We must be called via kern_intr (which already checks for
		 * IPL_NONE so of course we call be preempted).
		 */
		return true;
	}
	/*
	 * We are called from KPREEMPT_ENABLE().  If we are at IPL_NONE,
	 * of course we can be preempted.  If we aren't, ask for a
	 * softint so that kern_intr can call kpreempt.
	 */
	if (s == IPL_NONE) {
		KASSERT(curcpu()->ci_mtx_count == 0);
		return true;
	}
	softint_trigger(SOFTINT_KPREEMPT);
#endif
	return false;
}

void
cpu_kpreempt_exit(uintptr_t where)
{

	/* do nothing */
}

/*
 * Return true if preemption is disabled for MD reasons.  Must be called
 * with preemption disabled, and thus is only for diagnostic checks.
 */
bool
cpu_kpreempt_disabled(void)
{
	/*
	 * Any elevated IPL disables preemption.
	 */
	return curcpu()->ci_cpl > IPL_NONE;
}
#endif /* __HAVE_PREEMPTION */

void
cpu_idle(void)
{
	void (*const mach_idle)(void) = mips_locoresw.lsw_cpu_idle;
	struct cpu_info * const ci = curcpu();

	while (!ci->ci_want_resched) {
#ifdef __HAVE_FAST_SOFTINTS
		KASSERT(ci->ci_data.cpu_softints == 0);
#endif
		(*mach_idle)();
	}
}

bool
cpu_intr_p(void)
{
	bool rv;
	kpreempt_disable();
	rv = (curcpu()->ci_idepth != 0);
	kpreempt_enable();
	return rv;
}

#ifdef MULTIPROCESSOR

void
cpu_broadcast_ipi(int tag)
{
	// No reason to remove ourselves since multicast_ipi will do that for us
	cpu_multicast_ipi(cpus_running, tag);
}

void
cpu_multicast_ipi(const kcpuset_t *kcp, int tag)
{
	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp2;

	if (kcpuset_match(cpus_running, ci->ci_data.cpu_kcpuset))
		return;

	kcpuset_clone(&kcp2, kcp);
	kcpuset_remove(kcp2, ci->ci_data.cpu_kcpuset);
	for (cpuid_t cii; (cii = kcpuset_ffs(kcp2)) != 0; ) {
		kcpuset_clear(kcp2, --cii);
		(void)cpu_send_ipi(cpu_lookup(cii), tag);
	}
	kcpuset_destroy(kcp2);
}

int
cpu_send_ipi(struct cpu_info *ci, int tag)
{

	return (*mips_locoresw.lsw_send_ipi)(ci, tag);
}

static void
cpu_ipi_wait(const char *s, const kcpuset_t *watchset, const kcpuset_t *wanted)
{
	bool done = false;
	kcpuset_t *kcp;
	kcpuset_create(&kcp, false);

	/* some finite amount of time */

	for (u_long limit = curcpu()->ci_cpu_freq/10; !done && limit--; ) {
		kcpuset_copy(kcp, watchset);
		kcpuset_intersect(kcp, wanted);
		done = kcpuset_match(kcp, wanted);
	}

	if (!done) {
		cpuid_t cii;
		kcpuset_copy(kcp, wanted);
		kcpuset_remove(kcp, watchset);
		if ((cii = kcpuset_ffs(kcp)) != 0) {
			printf("Failed to %s:", s);
			do {
				kcpuset_clear(kcp, --cii);
				printf(" cpu%lu", cii);
			} while ((cii = kcpuset_ffs(kcp)) != 0);
			printf("\n");
		}
	}

	kcpuset_destroy(kcp);
}

/*
 * Halt this cpu
 */
void
cpu_halt(void)
{
	cpuid_t cii = cpu_index(curcpu());

	printf("cpu%lu: shutting down\n", cii);
	kcpuset_atomic_set(cpus_halted, cii);
	spl0();		/* allow interrupts e.g. further ipi ? */
	for (;;) ;	/* spin */

	/* NOTREACHED */
}

/*
 * Halt all running cpus, excluding current cpu.
 */
void
cpu_halt_others(void)
{
	kcpuset_t *kcp;

	// If we are the only CPU running, there's nothing to do.
	if (kcpuset_match(cpus_running, curcpu()->ci_data.cpu_kcpuset))
		return;

	// Get all running CPUs
	kcpuset_clone(&kcp, cpus_running);
	// Remove ourself
	kcpuset_remove(kcp, curcpu()->ci_data.cpu_kcpuset);
	// Remove any halted CPUs
	kcpuset_remove(kcp, cpus_halted);
	// If there are CPUs left, send the IPIs
	if (!kcpuset_iszero(kcp)) {
		cpu_multicast_ipi(kcp, IPI_HALT);
		cpu_ipi_wait("halt", cpus_halted, kcp);
	}
	kcpuset_destroy(kcp);

	/*
	 * TBD
	 * Depending on available firmware methods, other cpus will
	 * either shut down themselves, or spin and wait for us to
	 * stop them.
	 */
}

/*
 * Pause this cpu
 */
void
cpu_pause(struct reg *regsp)
{
	int s = splhigh();
	cpuid_t cii = cpu_index(curcpu());

	if (__predict_false(cold))
		return;

	do {
		kcpuset_atomic_set(cpus_paused, cii);
		do {
			;
		} while (kcpuset_isset(cpus_paused, cii));
		kcpuset_atomic_set(cpus_resumed, cii);
#if defined(DDB)
		if (ddb_running_on_this_cpu_p())
			cpu_Debugger();
		if (ddb_running_on_any_cpu_p())
			continue;
#endif
	} while (false);

	splx(s);
}

/*
 * Pause all running cpus, excluding current cpu.
 */
void
cpu_pause_others(void)
{
	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp;

	if (cold || kcpuset_match(cpus_running, ci->ci_data.cpu_kcpuset))
		return;

	kcpuset_clone(&kcp, cpus_running);
	kcpuset_remove(kcp, ci->ci_data.cpu_kcpuset);
	kcpuset_remove(kcp, cpus_paused);

	cpu_broadcast_ipi(IPI_SUSPEND);
	cpu_ipi_wait("pause", cpus_paused, kcp);

	kcpuset_destroy(kcp);
}

/*
 * Resume a single cpu
 */
void
cpu_resume(cpuid_t cii)
{
	kcpuset_t *kcp;

	if (__predict_false(cold))
		return;

	kcpuset_create(&kcp, true);
	kcpuset_set(kcp, cii);
	kcpuset_atomicly_remove(cpus_resumed, cpus_resumed);
	kcpuset_atomic_clear(cpus_paused, cii);

	cpu_ipi_wait("resume", cpus_resumed, kcp);

	kcpuset_destroy(kcp);
}

/*
 * Resume all paused cpus.
 */
void
cpu_resume_others(void)
{
	kcpuset_t *kcp;

	if (__predict_false(cold))
		return;

	kcpuset_atomicly_remove(cpus_resumed, cpus_resumed);
	kcpuset_clone(&kcp, cpus_paused);
	kcpuset_atomicly_remove(cpus_paused, cpus_paused);

	/* CPUs awake on cpus_paused clear */
	cpu_ipi_wait("resume", cpus_resumed, kcp);

	kcpuset_destroy(kcp);
}

bool
cpu_is_paused(cpuid_t cii)
{

	return !cold && kcpuset_isset(cpus_paused, cii);
}

#ifdef DDB
void
cpu_debug_dump(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	char running, hatched, paused, resumed, halted;

	db_printf("CPU CPUID STATE CPUINFO            CPL INT MTX IPIS\n");
	for (CPU_INFO_FOREACH(cii, ci)) {
		hatched = (kcpuset_isset(cpus_hatched, cpu_index(ci)) ? 'H' : '-');
		running = (kcpuset_isset(cpus_running, cpu_index(ci)) ? 'R' : '-');
		paused  = (kcpuset_isset(cpus_paused,  cpu_index(ci)) ? 'P' : '-');
		resumed = (kcpuset_isset(cpus_resumed, cpu_index(ci)) ? 'r' : '-');
		halted  = (kcpuset_isset(cpus_halted,  cpu_index(ci)) ? 'h' : '-');
		db_printf("%3d 0x%03lx %c%c%c%c%c %p "
			"%3d %3d %3d "
			"0x%02" PRIx64 "/0x%02" PRIx64 "\n",
			cpu_index(ci), ci->ci_cpuid,
			running, hatched, paused, resumed, halted,
			ci, ci->ci_cpl, ci->ci_idepth, ci->ci_mtx_count,
			ci->ci_active_ipis, ci->ci_request_ipis);
	}
}
#endif

void
cpu_hatch(struct cpu_info *ci)
{
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;

	/*
	 * Invalidate all the TLB enties (even wired ones) and then reserve
	 * space for the wired TLB entries.
	 */
	mips3_cp0_wired_write(0);
	tlb_invalidate_all();
	mips3_cp0_wired_write(ti->ti_wired);

	/*
	 * Setup HWRENA and USERLOCAL COP0 registers (MIPSxxR2).
	 */
	cpu_hwrena_setup();

	/*
	 * If we are using register zero relative addressing to access cpu_info
	 * in the exception vectors, enter that mapping into TLB now.
	 */
	if (ci->ci_tlb_slot >= 0) {
		const uint32_t tlb_lo = MIPS3_PG_G|MIPS3_PG_V
		    | mips3_paddr_to_tlbpfn((vaddr_t)ci);

		tlb_enter(ci->ci_tlb_slot, -PAGE_SIZE, tlb_lo);
	}

	/*
	 * Flush the icache just be sure.
	 */
	mips_icache_sync_all();

	/*
	 * Let this CPU do its own initialization (for things that have to be
	 * done on the local CPU).
	 */
	(*mips_locoresw.lsw_cpu_init)(ci);

	// Show this CPU as present.
	atomic_or_ulong(&ci->ci_flags, CPUF_PRESENT);

	/*
	 * Announce we are hatched
	 */
	kcpuset_atomic_set(cpus_hatched, cpu_index(ci));

	/*
	 * Now wait to be set free!
	 */
	while (! kcpuset_isset(cpus_running, cpu_index(ci))) {
		/* spin, spin, spin */
	}

	/*
	 * initialize the MIPS count/compare clock
	 */
	mips3_cp0_count_write(ci->ci_data.cpu_cc_skew);
	KASSERT(ci->ci_cycles_per_hz != 0);
	ci->ci_next_cp0_clk_intr = ci->ci_data.cpu_cc_skew + ci->ci_cycles_per_hz;
	mips3_cp0_compare_write(ci->ci_next_cp0_clk_intr);
	ci->ci_data.cpu_cc_skew = 0;

	/*
	 * Let this CPU do its own post-running initialization
	 * (for things that have to be done on the local CPU).
	 */
	(*mips_locoresw.lsw_cpu_run)(ci);

	/*
	 * Now turn on interrupts (and verify they are on).
	 */
	spl0();
	KASSERTMSG(ci->ci_cpl == IPL_NONE, "cpl %d", ci->ci_cpl);
	KASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);

	kcpuset_atomic_set(pmap_kernel()->pm_onproc, cpu_index(ci));
	kcpuset_atomic_set(pmap_kernel()->pm_active, cpu_index(ci));

	/*
	 * And do a tail call to idle_loop
	 */
	idle_loop(NULL);
}

void
cpu_boot_secondary_processors(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (CPU_IS_PRIMARY(ci))
			continue;
		KASSERT(ci->ci_data.cpu_idlelwp);

		/*
		 * Skip this CPU if it didn't sucessfully hatch.
		 */
		if (!kcpuset_isset(cpus_hatched, cpu_index(ci)))
			continue;

		ci->ci_data.cpu_cc_skew = mips3_cp0_count_read();
		atomic_or_ulong(&ci->ci_flags, CPUF_RUNNING);
		kcpuset_set(cpus_running, cpu_index(ci));
		// Spin until the cpu calls idle_loop
		for (u_int i = 0; i < 100; i++) {
			if (kcpuset_isset(cpus_running, cpu_index(ci)))
				break;
			delay(1000);
		}
	}
}

void
xc_send_ipi(struct cpu_info *ci)
{

	(*mips_locoresw.lsw_send_ipi)(ci, IPI_XCALL);
}

void
cpu_ipi(struct cpu_info *ci)
{
	(*mips_locoresw.lsw_send_ipi)(ci, IPI_GENERIC);
}

#endif /* MULTIPROCESSOR */

void
cpu_offline_md(void)
{

	(*mips_locoresw.lsw_cpu_offline_md)();
}

#ifdef _LP64
void
cpu_vmspace_exec(lwp_t *l, vaddr_t start, vaddr_t end)
{
	/*
	 * We need to turn on/off UX so that copyout/copyin will work
	 * well before setreg gets called.
	 */
	uint32_t sr = mips_cp0_status_read();
	if (end != (uint32_t) end) {
		mips_cp0_status_write(sr | MIPS3_SR_UX);
	} else {
		mips_cp0_status_write(sr & ~MIPS3_SR_UX);
	}
}
#endif

int
cpu_lwp_setprivate(lwp_t *l, void *v)
{
#if (MIPS32R2 + MIPS64R2) > 0
	if (l == curlwp
	    && (mips_options.mips_cpu->cpu_cp0flags & MIPS_CP0FL_USERLOCAL)) {
		mipsNN_cp0_userlocal_write(v);
	}
#endif
	return 0;
}


#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0

#if (CPUWATCH_MAX != 8)
# error CPUWATCH_MAX
#endif

/*
 * cpuwatch_discover - determine how many COP0 watchpoints this CPU supports
 */
u_int
cpuwatch_discover(void)
{
	int i;

	for (i=0; i < CPUWATCH_MAX; i++) {
		uint32_t watchhi = mipsNN_cp0_watchhi_read(i);
		if ((watchhi & __BIT(31)) == 0)	/* test 'M' bit */
			break;
	}
	return i + 1;
}

void
cpuwatch_free(cpu_watchpoint_t *cwp)
{
#ifdef DIAGNOSTIC
	struct cpu_info * const ci = curcpu();
	KASSERT(cwp >= &ci->ci_cpuwatch_tab[0] &&
		cwp <= &ci->ci_cpuwatch_tab[ci->ci_cpuwatch_count-1]);
#endif
	cwp->cw_mode = 0;
	cwp->cw_asid = 0;
	cwp->cw_addr = 0;
	cpuwatch_clr(cwp);
}

/*
 * cpuwatch_alloc
 * 	find an empty slot
 *	no locking for the table since it is CPU private
 */
cpu_watchpoint_t *
cpuwatch_alloc(void)
{
	struct cpu_info * const ci = curcpu();
	cpu_watchpoint_t *cwp;

	for (int i=0; i < ci->ci_cpuwatch_count; i++) {
		cwp = &ci->ci_cpuwatch_tab[i];
		if ((cwp->cw_mode & CPUWATCH_RWX) == 0)
			return cwp;
	}
	return NULL;
}


void
cpuwatch_set_all(void)
{
	struct cpu_info * const ci = curcpu();
	cpu_watchpoint_t *cwp;
	int i;

	for (i=0; i < ci->ci_cpuwatch_count; i++) {
		cwp = &ci->ci_cpuwatch_tab[i];
		if ((cwp->cw_mode & CPUWATCH_RWX) != 0)
			cpuwatch_set(cwp);
	}
}

void
cpuwatch_clr_all(void)
{
	struct cpu_info * const ci = curcpu();
	cpu_watchpoint_t *cwp;
	int i;

	for (i=0; i < ci->ci_cpuwatch_count; i++) {
		cwp = &ci->ci_cpuwatch_tab[i];
		if ((cwp->cw_mode & CPUWATCH_RWX) != 0)
			cpuwatch_clr(cwp);
	}
}

/*
 * cpuwatch_set - establish a MIPS COP0 watchpoint
 */
void
cpuwatch_set(cpu_watchpoint_t *cwp)
{
	struct cpu_info * const ci = curcpu();
	uint32_t watchhi;
	register_t watchlo;
	int cwnum = cwp - &ci->ci_cpuwatch_tab[0];

	KASSERT(cwp >= &ci->ci_cpuwatch_tab[0] &&
		cwp <= &ci->ci_cpuwatch_tab[ci->ci_cpuwatch_count-1]);

	watchlo = cwp->cw_addr;
	if (cwp->cw_mode & CPUWATCH_WRITE)
		watchlo |= __BIT(0);
	if (cwp->cw_mode & CPUWATCH_READ)
		watchlo |= __BIT(1);
	if (cwp->cw_mode & CPUWATCH_EXEC)
		watchlo |= __BIT(2);

	if (cwp->cw_mode & CPUWATCH_ASID)
		watchhi = cwp->cw_asid << 16;	/* addr qualified by asid */
	else
		watchhi = __BIT(30);		/* addr not qual. by asid (Global) */
	if (cwp->cw_mode & CPUWATCH_MASK)
		watchhi |= cwp->cw_mask;	/* set "dont care" addr match bits */

	mipsNN_cp0_watchhi_write(cwnum, watchhi);
	mipsNN_cp0_watchlo_write(cwnum, watchlo);
}

/*
 * cpuwatch_clr - disestablish a MIPS COP0 watchpoint
 */
void
cpuwatch_clr(cpu_watchpoint_t *cwp)
{
	struct cpu_info * const ci = curcpu();
	int cwnum = cwp - &ci->ci_cpuwatch_tab[0];

	KASSERT(cwp >= &ci->ci_cpuwatch_tab[0] &&
		cwp <= &ci->ci_cpuwatch_tab[ci->ci_cpuwatch_count-1]);

	mipsNN_cp0_watchhi_write(cwnum, 0);
	mipsNN_cp0_watchlo_write(cwnum, 0);
}

#endif	/* (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */
