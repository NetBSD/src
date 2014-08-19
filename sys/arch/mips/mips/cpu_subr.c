/*	$NetBSD: cpu_subr.c,v 1.16.2.1 2014/08/20 00:03:12 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.16.2.1 2014/08/20 00:03:12 tls Exp $");

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
#include <sys/ipi.h>

#include <uvm/uvm.h>

#include <mips/locore.h>
#include <mips/regnum.h>
#include <mips/pcb.h>
#include <mips/cache.h>
#include <mips/frame.h>
#include <mips/userret.h>
#include <mips/pte.h>
#include <mips/cpuset.h>

#if defined(DDB) || defined(KGDB)
#ifdef DDB 
#include <mips/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#endif
#endif

struct cpu_info cpu_info_store
#ifdef MULTIPROCESSOR
	__section(".data1")
	__aligned(1LU << ilog2((2*sizeof(struct cpu_info)-1)))
#endif
    = {
	.ci_curlwp = &lwp0,
	.ci_tlb_info = &pmap_tlb0_info,
	.ci_pmap_seg0tab = (void *)(MIPS_KSEG2_START + 0x1eadbeef),
#ifdef _LP64
	.ci_pmap_segtab = (void *)(MIPS_KSEG2_START + 0x1eadbeef),
#endif
	.ci_cpl = IPL_HIGH,
	.ci_tlb_slot = -1,
#ifdef MULTIPROCESSOR
	.ci_flags = CPUF_PRIMARY|CPUF_PRESENT|CPUF_RUNNING,
#endif
};

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &mips_fpu_ops,
#if (MIPS32R2 + MIPS64R2) > 0
	[PCU_DSP] = &mips_dsp_ops,
#endif
};

#ifdef MULTIPROCESSOR

volatile __cpuset_t cpus_running = 1;
volatile __cpuset_t cpus_hatched = 1;
volatile __cpuset_t cpus_paused = 0;
volatile __cpuset_t cpus_resumed = 0;
volatile __cpuset_t cpus_halted = 0;

static int  cpu_ipi_wait(volatile __cpuset_t *, u_long);
static void cpu_ipi_error(const char *, __cpuset_t, __cpuset_t);

static struct cpu_info *cpu_info_last = &cpu_info_store;

struct cpu_info *
cpu_info_alloc(struct pmap_tlb_info *ti, cpuid_t cpu_id, cpuid_t cpu_package_id,
	cpuid_t cpu_core_id, cpuid_t cpu_smt_id)
{
	vaddr_t cpu_info_offset = (vaddr_t)&cpu_info_store & PAGE_MASK; 
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

	/*
	 * Attach its TLB info (which must be direct-mapped)
	 */
#ifdef _LP64
	KASSERT(MIPS_KSEG0_P(ti) || MIPS_XKPHYS_P(ti));
#else
	KASSERT(MIPS_KSEG0_P(ti));
#endif

#ifndef _LP64
	/*
	 * If we have more memory than can be mapped by KSEG0, we need to
	 * allocate enough VA so we can map pages with the right color
	 * (to avoid cache alias problems).
	 */
	if (mips_avail_end > MIPS_KSEG1_START - MIPS_KSEG0_START) {
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

	if (ci == &cpu_info_store)
		pmap_tlb_info_evcnt_attach(ci->ci_tlb_info);

#ifdef MULTIPROCESSOR
	if (ci != &cpu_info_store) {
		/*
		 * Tail insert this onto the list of cpu_info's.
		 */
		KASSERT(ci->ci_next == NULL);
		KASSERT(cpu_info_last->ci_next == NULL);
		cpu_info_last->ci_next = ci;
		cpu_info_last = ci;
	}
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

	cpu_hwrena_setup();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
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
	(void)cpu_multicast_ipi(
		CPUSET_EXCEPT(cpus_running, cpu_index(curcpu())), tag);
}

void
cpu_multicast_ipi(__cpuset_t cpuset, int tag)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	CPUSET_DEL(cpuset, cpu_index(curcpu()));
	if (CPUSET_EMPTY_P(cpuset))
		return;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (CPUSET_HAS_P(cpuset, cpu_index(ci))) {
			CPUSET_DEL(cpuset, cpu_index(ci));
			(void)cpu_send_ipi(ci, tag);
		}
	}
}

int
cpu_send_ipi(struct cpu_info *ci, int tag)
{

	return (*mips_locoresw.lsw_send_ipi)(ci, tag);
}

static void
cpu_ipi_error(const char *s, __cpuset_t succeeded, __cpuset_t expected)
{
	CPUSET_SUB(expected, succeeded);
	if (!CPUSET_EMPTY_P(expected)) {
		printf("Failed to %s:", s);
		do {
			int index = CPUSET_NEXT(expected);
			CPUSET_DEL(expected, index);
			printf(" cpu%d", index);
		} while (!CPUSET_EMPTY_P(expected));
		printf("\n");
	}
}

static int
cpu_ipi_wait(volatile __cpuset_t *watchset, u_long mask)
{
	u_long limit = curcpu()->ci_cpu_freq;	/* some finite amount of time */

	while (limit--)
		if (*watchset == mask)
			return 0;		/* success */

	return 1;				/* timed out */
}

/*
 * Halt this cpu
 */
void
cpu_halt(void)
{
	int index = cpu_index(curcpu());

	printf("cpu%d: shutting down\n", index);
	CPUSET_ADD(cpus_halted, index);
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
	__cpuset_t cpumask, cpuset;

	CPUSET_ASSIGN(cpuset, cpus_running);
	CPUSET_DEL(cpuset, cpu_index(curcpu()));
	CPUSET_ASSIGN(cpumask, cpuset);
	CPUSET_SUB(cpuset, cpus_halted);

	if (CPUSET_EMPTY_P(cpuset))
		return;

	cpu_multicast_ipi(cpuset, IPI_HALT);
	if (cpu_ipi_wait(&cpus_halted, cpumask))
		cpu_ipi_error("halt", cpumask, cpus_halted);

	/*
	 * TBD
	 * Depending on available firmware methods, other cpus will
	 * either shut down themselfs, or spin and wait for us to
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
	int index = cpu_index(curcpu());

	for (;;) {
		CPUSET_ADD(cpus_paused, index);
		do {
			;
		} while (CPUSET_HAS_P(cpus_paused, index));
		CPUSET_ADD(cpus_resumed, index);

#if defined(DDB)
		if (ddb_running_on_this_cpu_p())
			cpu_Debugger();
		if (ddb_running_on_any_cpu_p())
			continue;
#endif
		break;
	}

	splx(s);
}

/*
 * Pause all running cpus, excluding current cpu.
 */
void
cpu_pause_others(void)
{
	__cpuset_t cpuset;

	CPUSET_ASSIGN(cpuset, cpus_running);
	CPUSET_DEL(cpuset, cpu_index(curcpu()));

	if (CPUSET_EMPTY_P(cpuset))
		return;

	cpu_multicast_ipi(cpuset, IPI_SUSPEND);
	if (cpu_ipi_wait(&cpus_paused, cpuset))
		cpu_ipi_error("pause", cpus_paused, cpuset);
}

/*
 * Resume a single cpu
 */
void
cpu_resume(int index)
{
	CPUSET_CLEAR(cpus_resumed);
	CPUSET_DEL(cpus_paused, index);

	if (cpu_ipi_wait(&cpus_resumed, CPUSET_SINGLE(index)))
		cpu_ipi_error("resume", cpus_resumed, CPUSET_SINGLE(index));
}

/*
 * Resume all paused cpus.
 */
void
cpu_resume_others(void)
{
	__cpuset_t cpuset;

	CPUSET_CLEAR(cpus_resumed);
	CPUSET_ASSIGN(cpuset, cpus_paused);
	CPUSET_CLEAR(cpus_paused);

	/* CPUs awake on cpus_paused clear */
	if (cpu_ipi_wait(&cpus_resumed, cpuset))
		cpu_ipi_error("resume", cpus_resumed, cpuset);
}

int
cpu_is_paused(int index)
{

	return CPUSET_HAS_P(cpus_paused, index);
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
		hatched = (CPUSET_HAS_P(cpus_hatched, cpu_index(ci)) ? 'H' : '-');
		running = (CPUSET_HAS_P(cpus_running, cpu_index(ci)) ? 'R' : '-');
		paused  = (CPUSET_HAS_P(cpus_paused,  cpu_index(ci)) ? 'P' : '-');
		resumed = (CPUSET_HAS_P(cpus_resumed, cpu_index(ci)) ? 'r' : '-');
		halted  = (CPUSET_HAS_P(cpus_halted,  cpu_index(ci)) ? 'h' : '-');
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

	/*
	 * Announce we are hatched
	 */
	CPUSET_ADD(cpus_hatched, cpu_index(ci));

	/*
	 * Now wait to be set free!
	 */
	while (! CPUSET_HAS_P(cpus_running, cpu_index(ci))) {
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
	 * Now turn on interrupts.
	 */
	spl0();

	/*
	 * And do a tail call to idle_loop
	 */
	idle_loop(NULL);
}

void
cpu_boot_secondary_processors(void)
{
	for (struct cpu_info *ci = cpu_info_store.ci_next;
	     ci != NULL;
	     ci = ci->ci_next) {
		KASSERT(!CPU_IS_PRIMARY(ci));
		KASSERT(ci->ci_data.cpu_idlelwp);

		/*
		 * Skip this CPU if it didn't sucessfully hatch.
		 */
		if (! CPUSET_HAS_P(cpus_hatched, cpu_index(ci)))
			continue;

		ci->ci_data.cpu_cc_skew = mips3_cp0_count_read();
		atomic_or_ulong(&ci->ci_flags, CPUF_RUNNING);
		CPUSET_ADD(cpus_running, cpu_index(ci));
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
