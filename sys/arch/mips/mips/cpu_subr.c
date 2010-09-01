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

#include "opt_multiprocessor.h"
#include "opt_sa.h"

__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.1.2.12 2010/09/01 00:59:42 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/atomic.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ras.h>
#include <sys/bitops.h>
#include <sys/idle.h>
#ifdef KERN_SA
#include <sys/sa.h>
#include <sys/savar.h>
#endif

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/regnum.h>
#include <mips/cache.h>
#include <mips/frame.h>
#include <mips/userret.h>
#include <mips/pte.h>

struct cpu_info cpu_info_store
#ifdef MULTIPROCESSOR
	__section(".data1")
	__aligned(1LU << ilog2((2*sizeof(struct cpu_info)-1)))
#endif
    = {
	.ci_curlwp = &lwp0,
#ifndef NOFPU
	.ci_fpcurlwp = &lwp0,
#endif
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

#ifdef MULTIPROCESSOR

volatile u_long cpus_running = 1;
volatile u_long cpus_hatched = 1;
volatile u_long cpus_paused = 0;

static struct cpu_info *cpu_info_last = &cpu_info_store;

struct cpu_info *
cpu_info_alloc(struct pmap_tlb_info *ti, cpuid_t cpu_id, cpuid_t cpu_node_id,
	cpuid_t cpu_core_id, cpuid_t cpu_smt_id)
{
	vaddr_t cpu_info_offset = (vaddr_t)&cpu_info_store & PAGE_MASK; 
	struct pglist pglist;
	int error;

	/*
	* Grab a page from the first 256MB to use to store
	* exception vectors and cpu_info for this cpu.
	*/
	error = uvm_pglistalloc(PAGE_SIZE,
	    0, 0x10000000,
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
	ci->ci_data.cpu_node_id = cpu_node_id;
	ci->ci_data.cpu_core_id = cpu_core_id;
	ci->ci_data.cpu_smt_id = cpu_smt_id;
	ci->ci_cpu_freq = cpu_info_store.ci_cpu_freq;
	ci->ci_cctr_freq = cpu_info_store.ci_cctr_freq;
        ci->ci_cycles_per_hz = cpu_info_store.ci_cycles_per_hz;
        ci->ci_divisor_delay = cpu_info_store.ci_divisor_delay;
        ci->ci_divisor_recip = cpu_info_store.ci_divisor_recip;

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

	/*
	 * Switch the idle lwp to a direct mapped stack since we use its
	 * stack and we won't have a TLB entry for it.
	 */
	cpu_uarea_remap(ci->ci_data.cpu_idlelwp);

	return ci;
}
#endif /* MULTIPROCESSOR */

void
cpu_attach_common(device_t self, struct cpu_info *ci)
{
	/*
	 * Cross link cpu_info and its device together
	 */
	ci->ci_dev = self;
	self->dv_private = ci;
	KASSERT(ci->ci_idepth == 0);

	evcnt_attach_dynamic(&ci->ci_count_compare_evcnt,
		EVCNT_TYPE_INTR, NULL, device_xname(self),
		"int 5 (clock)");
	evcnt_attach_dynamic(&ci->ci_count_compare_missed_evcnt,
		EVCNT_TYPE_INTR, NULL, device_xname(self),
		"int 5 (clock) missed");

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
	    EVCNT_TYPE_MISC, NULL, device_xname(self),
	    "syncicache activate request");
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_deferred_rqst,
	    EVCNT_TYPE_MISC, NULL, device_xname(self),
	    "syncicache deferred request");
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_ipi_rqst,
	    EVCNT_TYPE_MISC, NULL, device_xname(self),
	    "syncicache ipi request");
	evcnt_attach_dynamic(&ci->ci_evcnt_synci_onproc_rqst,
	    EVCNT_TYPE_MISC, NULL, device_xname(self),
	    "syncicache onproc request");

	/*
	 * Initialize IPI framework for this cpu instance
	 */
	ipi_init(ci);
#endif

#ifndef NOFPU
	/*
	 * Indicate the CPU's FPU is owned by the CPU's idlelwp.
	 * (which really means it's free).
	 */
	ci->ci_fpcurlwp = ci->ci_data.cpu_idlelwp;
#endif
}

void
cpu_startup_common(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];	/* "99999 MB" */

	fpu_init();

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

#ifdef KERN_SA
/*
 * XXX This is a terrible name.
 */
void
upcallret(struct lwp *l)
{
	userret(l);
}

void 
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
    void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct trapframe *tf = l->l_md.md_utf;
	struct saframe frame;
#ifdef COMPAT_NETBSD32
	struct saframe32 frame32;
#endif
	void *ksf, *usf;
	size_t sfsz;

#if 0 /* First 4 args in regs (see below). */
	frame.sa_type = type;
	frame.sa_sas = sas;
	frame.sa_events = nevents;
	frame.sa_interrupted = ninterrupted;
#endif
#ifdef COMPAT_NETBSD32
	switch (l->l_proc->p_md.md_abi) {
	case _MIPS_BSD_API_O32:
	case _MIPS_BSD_API_N32:
		NETBSD32PTR32(frame32.sa_arg, ap);
		NETBSD32PTR32(frame32.sa_upcall, upcall);
		ksf = &frame32;
		usf = (struct saframe32 *)sp - 1;
		sfsz = sizeof(frame32);
		break;
	default:
#endif
		frame.sa_arg = ap;
		frame.sa_upcall = upcall;
		ksf = &frame;
		usf = (struct saframe *)sp - 1;
		sfsz = sizeof(frame);
#ifdef COMPAT_NETBSD32
		break;
	}
#endif

	if (copyout(ksf, usf, sfsz) != 0) {
		/* Copying onto the stack didn't work. Die. */
		mutex_enter(l->l_proc->p_lock);
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_regs[_R_PC] = (intptr_t)upcall;
	tf->tf_regs[_R_SP] = (intptr_t)usf;
	tf->tf_regs[_R_A0] = type;
	tf->tf_regs[_R_A1] = (intptr_t)sas;
	tf->tf_regs[_R_A2] = nevents;
	tf->tf_regs[_R_A3] = ninterrupted;
	tf->tf_regs[_R_S8] = 0;
	tf->tf_regs[_R_RA] = 0;
	tf->tf_regs[_R_T9] = (intptr_t)upcall;  /* t9=Upcall function*/
}
#endif /* KERN_SA */

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

	if ((ras_pc = (intptr_t)ras_lookup(l->l_proc,
	    (void *) (intptr_t)gr[_REG_EPC])) != -1)
		gr[_REG_EPC] = ras_pc;

	*flags |= _UC_CPU;

	/* Save floating point register context, if any. */
	if (l->l_md.md_flags & MDP_FPUSED) {
		size_t fplen;
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 */
		fpusave_lwp(l);

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
		memcpy(&mcp->__fpregs, &l->l_addr->u_pcb.pcb_fpregs, fplen);
		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_utf;
	struct proc *p = l->l_proc;
	const __greg_t *gr = mcp->__gregs;

	/* Restore register context, if any. */
	if (flags & _UC_CPU) {
		/* Save register context. */
		/* XXX:  Do we validate the addresses?? */
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

	/* Restore floating point register context, if any. */
	if (flags & _UC_FPU) {
		size_t fplen;

		/* Disable the FPU contents. */
		fpudiscard_lwp(l);

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
		memcpy(&l->l_addr->u_pcb.pcb_fpregs, &mcp->__fpregs, fplen);
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
	KASSERT(lwp_locked(l, NULL));
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

	if (where == 0) {
		/*
		 * We are called from KPREEMPT_ENABLE().  If we are at IPL_NONE,
		 * of course we can be preempted.  If we aren't, ask for a
		 * softint so that kern_intr can call kpreempt.
		 */
		if (s == IPL_NONE)
			return true;
		softint_trigger(SOFTINT_KPREEMPT);
		return false;
	}

	/*
	 * We must be called via kern_intr (which already checks for IPL_NONE
	 * so of course we call be preempted).
	 */
	return true;
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
int
cpu_send_ipi(struct cpu_info *ci, int tag)
{

	return (*mips_locoresw.lsw_send_ipi)(ci, tag);
}

void
cpu_hatch(struct cpu_info *ci)
{
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;
	const u_long cpu_mask = 1L << cpu_index(ci);

	/*
	 * Invalidate all the TLB enties (even wired ones) and then reserve
	 * space for the wired TLB entries.
	 */
	mips3_cp0_wired_write(0);
	tlb_invalidate_all();
	mips3_cp0_wired_write(ti->ti_wired);

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
	atomic_or_ulong(&cpus_hatched, cpu_mask);

	/*
	 * Now wait to be set free!
	 */
	while ((cpus_running & cpu_mask) == 0) {
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
		const u_long cpu_mask = 1L << cpu_index(ci);
		KASSERT(!CPU_IS_PRIMARY(ci));
		KASSERT(ci->ci_data.cpu_idlelwp);

		/*
		 * Skip this CPU if it didn't sucessfully hatch.
		 */
		if ((cpus_hatched & cpu_mask) == 0)
			continue;

		ci->ci_data.cpu_cc_skew = mips3_cp0_count_read();
		atomic_or_ulong(&ci->ci_flags, CPUF_RUNNING);
		atomic_or_ulong(&cpus_running, cpu_mask);
	}
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
