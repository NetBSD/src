/*	$NetBSD: cpu_subr.c,v 1.60 2022/03/03 06:27:41 riastradh Exp $	*/

/*-
 * Copyright (c) 2010, 2019 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.60 2022/03/03 06:27:41 riastradh Exp $");

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bitops.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/idle.h>
#include <sys/intr.h>
#include <sys/ipi.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/ras.h>
#include <sys/reboot.h>
#include <sys/xcall.h>

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
#include <mips/cavium/octeonvar.h>
extern struct cpu_softc octeon_cpu_softc[];
#endif

struct cpu_info cpu_info_store
#if defined(MULTIPROCESSOR) && !defined(MIPS64_OCTEON)
	__section(".data1")
	__aligned(1LU << ilog2((2*sizeof(struct cpu_info)-1)))
#endif
    = {
	.ci_curlwp = &lwp0,
	.ci_tlb_info = &pmap_tlb0_info,
	.ci_pmap_kern_segtab = &pmap_kern_segtab,
	.ci_pmap_user_segtab = NULL,
#ifdef _LP64
	.ci_pmap_user_seg0tab = NULL,
#endif
	.ci_cpl = IPL_HIGH,
	.ci_tlb_slot = -1,
#ifdef MULTIPROCESSOR
	.ci_flags = CPUF_PRIMARY|CPUF_PRESENT|CPUF_RUNNING,
#endif
#ifdef MIPS64_OCTEON
	.ci_softc = &octeon_cpu_softc[0],
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
	const int exc_step = 1 << MIPS_EBASE_EXC_BASE_SHIFT;
	vaddr_t exc_page = MIPS_UTLB_MISS_EXC_VEC + exc_step * cpu_id;
	__CTASSERT(sizeof(struct cpu_info) + sizeof(struct pmap_tlb_info)
	    <= exc_step - 0x280);

	struct cpu_info * const ci = ((struct cpu_info *)(exc_page + exc_step)) - 1;
	memset((void *)exc_page, 0, exc_step);

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
	ci->ci_pmap_kern_segtab = &pmap_kern_segtab,
	ci->ci_cpu_freq = cpu_info_store.ci_cpu_freq;
	ci->ci_cctr_freq = cpu_info_store.ci_cctr_freq;
	ci->ci_cycles_per_hz = cpu_info_store.ci_cycles_per_hz;
	ci->ci_divisor_delay = cpu_info_store.ci_divisor_delay;
	ci->ci_divisor_recip = cpu_info_store.ci_divisor_recip;
	ci->ci_cpuwatch_count = cpu_info_store.ci_cpuwatch_count;

	cpu_topology_set(ci, cpu_package_id, cpu_core_id, cpu_smt_id, 0);

	pmap_md_alloc_ephemeral_address_space(ci);

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

	if (CPUISMIPSNNR2) {
		mipsNN_cp0_hwrena_write(
		    (MIPS_HAS_USERLOCAL ? MIPS_HWRENA_ULR : 0)
		    | MIPS_HWRENA_CCRES
		    | MIPS_HWRENA_CC
		    | MIPS_HWRENA_SYNCI_STEP
		    | MIPS_HWRENA_CPUNUM);
		if (MIPS_HAS_USERLOCAL) {
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
	 *
	 * XXX autoconf abuse: Can't use device_set_private here
	 * because some callers already do so -- and some callers
	 * (sbmips cpu_attach) already have a softc allocated by
	 * autoconf.
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

	kcpuset_create(&ci->ci_shootdowncpus, true);
	kcpuset_create(&ci->ci_multicastcpus, true);
	kcpuset_create(&ci->ci_watchcpus, true);
	kcpuset_create(&ci->ci_ddbcpus, true);
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

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvm_availmem(false)));
	printf("avail memory = %s\n", pbuf);

#if defined(__mips_n32)
	module_machine = "mips-n32";
#endif
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
	if (fpu_used_p(l)) {
		size_t fplen;
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 */
		fpu_save(l);

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
		fpu_discard(l);

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
cpu_need_resched(struct cpu_info *ci, struct lwp *l, int flags)
{

	KASSERT(kpreempt_disabled());

	if ((flags & RESCHED_KPREEMPT) != 0) {
#ifdef __HAVE_PREEMPTION
		if ((flags & RESCHED_REMOTE) != 0) {
			cpu_send_ipi(ci, IPI_KPREEMPT);
		} else {
			softint_trigger(SOFTINT_KPREEMPT);
		}
#endif
		return;
	}
	if ((flags & RESCHED_REMOTE) != 0) {
#ifdef MULTIPROCESSOR
		cpu_send_ipi(ci, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1;		/* force call to ast() */
	}
}

uint32_t
cpu_clkf_usermode_mask(void)
{

	return CPUISMIPS3 ? MIPS_SR_KSU_USER : MIPS_SR_KU_PREV;
}

void
cpu_signotify(struct lwp *l)
{

	KASSERT(kpreempt_disabled());
#ifdef __HAVE_FAST_SOFTINTS
	KASSERT(lwp_locked(l, NULL));
#endif

	if (l->l_cpu != curcpu()) {
#ifdef MULTIPROCESSOR
		cpu_send_ipi(l->l_cpu, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1; 	/* force call to ast() */
	}
}

void
cpu_need_proftick(struct lwp *l)
{

	KASSERT(kpreempt_disabled());
	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	l->l_md.md_astpending = 1;		/* force call to ast() */
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
	uint64_t ncsw;
	int idepth;
	lwp_t *l;

	l = curlwp;
	do {
		ncsw = l->l_ncsw;
		__insn_barrier();
		idepth = l->l_cpu->ci_idepth;
		__insn_barrier();
	} while (__predict_false(ncsw != l->l_ncsw));

	return idepth != 0;
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
	kcpuset_t *kcp2 = ci->ci_multicastcpus;

	if (kcpuset_match(cpus_running, ci->ci_data.cpu_kcpuset))
		return;

	kcpuset_copy(kcp2, kcp);
	kcpuset_remove(kcp2, ci->ci_data.cpu_kcpuset);
	for (cpuid_t cii; (cii = kcpuset_ffs(kcp2)) != 0; ) {
		kcpuset_clear(kcp2, --cii);
		(void)cpu_send_ipi(cpu_lookup(cii), tag);
	}
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
	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp = ci->ci_watchcpus;

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

	if (__predict_false(cold)) {
		splx(s);
		return;
	}

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

	if (cold || kcpuset_match(cpus_running, ci->ci_data.cpu_kcpuset))
		return;

	kcpuset_t *kcp = ci->ci_ddbcpus;

	kcpuset_copy(kcp, cpus_running);
	kcpuset_remove(kcp, ci->ci_data.cpu_kcpuset);
	kcpuset_remove(kcp, cpus_paused);

	cpu_broadcast_ipi(IPI_SUSPEND);
	cpu_ipi_wait("pause", cpus_paused, kcp);
}

/*
 * Resume a single cpu
 */
void
cpu_resume(cpuid_t cii)
{

	if (__predict_false(cold))
		return;

	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp = ci->ci_ddbcpus;

	kcpuset_set(kcp, cii);
	kcpuset_atomicly_remove(cpus_resumed, cpus_resumed);
	kcpuset_atomic_clear(cpus_paused, cii);

	cpu_ipi_wait("resume", cpus_resumed, kcp);
}

/*
 * Resume all paused cpus.
 */
void
cpu_resume_others(void)
{

	if (__predict_false(cold))
		return;

	struct cpu_info * const ci = curcpu();
	kcpuset_t *kcp = ci->ci_ddbcpus;

	kcpuset_atomicly_remove(cpus_resumed, cpus_resumed);
	kcpuset_copy(kcp, cpus_paused);
	kcpuset_atomicly_remove(cpus_paused, cpus_paused);

	/* CPUs awake on cpus_paused clear */
	cpu_ipi_wait("resume", cpus_resumed, kcp);
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
	db_printf("CPU CPUID STATE CPUINFO            CPL INT MTX IPIS(A/R)\n");
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
		const struct tlbmask tlbmask = {
			.tlb_hi = -PAGE_SIZE | KERNEL_PID,
#if (PGSHIFT & 1)
			.tlb_lo0 = tlb_lo,
			.tlb_lo1 = tlb_lo + MIPS3_PG_NEXT,
#else
			.tlb_lo0 = 0,
			.tlb_lo1 = tlb_lo,
#endif
			.tlb_mask = -1,
		};

		tlb_invalidate_addr(tlbmask.tlb_hi, KERNEL_PID);
		tlb_write_entry(ci->ci_tlb_slot, &tlbmask);
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

	if ((boothowto & RB_MD1) != 0)
		return;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (CPU_IS_PRIMARY(ci))
			continue;
		KASSERT(ci->ci_data.cpu_idlelwp);

		/*
		 * Skip this CPU if it didn't successfully hatch.
		 */
		if (!kcpuset_isset(cpus_hatched, cpu_index(ci)))
			continue;

		ci->ci_data.cpu_cc_skew = mips3_cp0_count_read();
		atomic_or_ulong(&ci->ci_flags, CPUF_RUNNING);
		kcpuset_set(cpus_running, cpu_index(ci));
		// Spin until the cpu calls idle_loop
		for (u_int i = 0; i < 10000; i++) {
			if (kcpuset_isset(kcpuset_running, cpu_index(ci)))
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
	if (l == curlwp && MIPS_HAS_USERLOCAL) {
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
