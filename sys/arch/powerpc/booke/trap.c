/*	$NetBSD: trap.c,v 1.24 2014/07/30 23:56:01 joerg Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include "opt_ddb.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: trap.c,v 1.24 2014/07/30 23:56:01 joerg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/siginfo.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/kauth.h>
#include <sys/ras.h>

#include <uvm/uvm_extern.h>

#include <powerpc/pcb.h>
#include <powerpc/userret.h>
#include <powerpc/psl.h>
#include <powerpc/instr.h>
#include <powerpc/altivec.h>		/* use same interface for SPE */

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#include <powerpc/booke/cpuvar.h>

#include <powerpc/fpu/fpu_extern.h>

#include <powerpc/db_machdep.h>
#include <ddb/db_interface.h>

#include <powerpc/trap.h>
#include <powerpc/booke/trap.h>
#include <powerpc/booke/pte.h>

void trap(enum ppc_booke_exceptions, struct trapframe *);

static const char trap_names[][8] = {
	[T_CRITIAL_INPUT] = "CRIT",
	[T_EXTERNAL_INPUT] = "EXT",
	[T_DECREMENTER] = "DECR",
	[T_FIXED_INTERVAL] = "FIT",
	[T_WATCHDOG] = "WDOG",
	[T_SYSTEM_CALL] = "SC",
	[T_MACHINE_CHECK] = "MCHK",
	[T_DSI] = "DSI",
	[T_ISI] = "ISI",
	[T_ALIGNMENT] = "ALN",
	[T_PROGRAM] = "PGM",
	[T_FP_UNAVAILABLE] = "FP",
	[T_AP_UNAVAILABLE] = "AP",
	[T_DATA_TLB_ERROR] = "DTLB",
	[T_INSTRUCTION_TLB_ERROR] = "ITLB",
	[T_DEBUG] = "DEBUG",
	[T_SPE_UNAVAILABLE] = "SPE",
	[T_EMBEDDED_FP_DATA] = "FPDATA",
	[T_EMBEDDED_FP_ROUND] = "FPROUND",
	[T_EMBEDDED_PERF_MONITOR] = "PERFMON",
	[T_AST] = "AST",
};

static inline bool
usertrap_p(struct trapframe *tf)
{
	return (tf->tf_srr1 & PSL_PR) != 0;
}

static int
mchk_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	const bool usertrap = usertrap_p(tf);
	const vaddr_t faultva = tf->tf_mcar;
	struct cpu_info * const ci = curcpu();
	int rv = EFAULT;

	if (usertrap)
		ci->ci_ev_umchk.ev_count++;

	if (rv != 0 && usertrap) {
		KSI_INIT_TRAP(ksi);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_DSI;
		ksi->ksi_code = SEGV_ACCERR;
		ksi->ksi_addr = (void *)faultva;
	}

	return rv;
}

static inline vm_prot_t
get_faulttype(const struct trapframe * const tf)
{
	return VM_PROT_READ | (tf->tf_esr & ESR_ST ? VM_PROT_WRITE : 0);
}

static inline struct vm_map *
get_faultmap(const struct trapframe * const tf, register_t psl_mask)
{
	return (tf->tf_srr1 & psl_mask)
	    ? &curlwp->l_proc->p_vmspace->vm_map
	    : kernel_map;
}

/*
 * We could use pmap_pte_lookup but this slightly faster since we already
 * the segtab pointers in cpu_info.
 */
static inline pt_entry_t *
trap_pte_lookup(struct trapframe *tf, vaddr_t va, register_t psl_mask)
{
	pmap_segtab_t ** const stps = &curcpu()->ci_pmap_kern_segtab;
	pmap_segtab_t * const stp = stps[(tf->tf_srr1 / psl_mask) & 1];
	if (__predict_false(stp == NULL))
		return NULL;
	pt_entry_t * const ptep = stp->seg_tab[va >> SEGSHIFT];
	if (__predict_false(ptep == NULL))
		return NULL;
	return ptep + ((va & SEGOFSET) >> PAGE_SHIFT);
}

static int
pagefault(struct vm_map *map, vaddr_t va, vm_prot_t ftype, bool usertrap)
{
	struct lwp * const l = curlwp;
	int rv;

//	printf("%s(%p,%#lx,%u,%u)\n", __func__, map, va, ftype, usertrap);

	if (usertrap) {
		rv = uvm_fault(map, trunc_page(va), ftype);
		if (rv == 0)
			uvm_grow(l->l_proc, trunc_page(va));
		if (rv == EACCES)
			rv = EFAULT;
	} else {
		if (cpu_intr_p())
			return EFAULT;

		struct pcb * const pcb = lwp_getpcb(l);
		struct faultbuf * const fb = pcb->pcb_onfault;
		pcb->pcb_onfault = NULL;
		rv = uvm_fault(map, trunc_page(va), ftype);
		pcb->pcb_onfault = fb;
		if (map != kernel_map) {
			if (rv == 0)
				uvm_grow(l->l_proc, trunc_page(va));
		}
		if (rv == EACCES)
			rv = EFAULT;
	}
	return rv;
}

static int
dsi_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	const vaddr_t faultva = tf->tf_dear;
	const vm_prot_t ftype = get_faulttype(tf);
	struct vm_map * const faultmap = get_faultmap(tf, PSL_DS);
	const bool usertrap = usertrap_p(tf);

	kpreempt_disable();
	struct cpu_info * const ci = curcpu();

	if (usertrap)
		ci->ci_ev_udsi.ev_count++;
	else
		ci->ci_ev_kdsi.ev_count++;

	/*
	 * If we had a TLB entry (which we must have had to get this exception),
	 * we certainly have a PTE.
	 */
	pt_entry_t * const ptep = trap_pte_lookup(tf, trunc_page(faultva),
	    PSL_DS);
	KASSERT(ptep != NULL);
	pt_entry_t pte = *ptep;

	if ((ftype & VM_PROT_WRITE)
	    && ((pte & (PTE_xW|PTE_UNMODIFIED)) == (PTE_xW|PTE_UNMODIFIED))) {
		const paddr_t pa = pte_to_paddr(pte);
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg);
		struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

		if (!VM_PAGEMD_MODIFIED_P(mdpg)) {
			pmap_page_set_attributes(mdpg, VM_PAGEMD_MODIFIED);
		}
		pte &= ~PTE_UNMODIFIED;
		*ptep = pte;
		pmap_tlb_update_addr(faultmap->pmap, trunc_page(faultva),
		    pte, 0);
		kpreempt_enable();
		return 0;
	}
	kpreempt_enable();

	int rv = pagefault(faultmap, faultva, ftype, usertrap);

	/*
	 * We can't get a MAPERR here since that's a different exception.
	 */
	if (__predict_false(rv != 0 && usertrap)) {
		ci->ci_ev_udsi_fatal.ev_count++;
		KSI_INIT_TRAP(ksi);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_DSI;
		ksi->ksi_code = SEGV_ACCERR;
		ksi->ksi_addr = (void *)faultva;
	}
	return rv;
}

static int
isi_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	const vaddr_t faultva = trunc_page(tf->tf_srr0);
	struct vm_map * const faultmap = get_faultmap(tf, PSL_IS);
	const bool usertrap = usertrap_p(tf);

	kpreempt_disable();
	struct cpu_info * const ci = curcpu();

	if (usertrap)
		ci->ci_ev_isi.ev_count++;
	else
		ci->ci_ev_kisi.ev_count++;

	/*
	 * If we had a TLB entry (which we must have had to get this exception),
	 * we certainly have a PTE.
	 */
	pt_entry_t * const ptep = trap_pte_lookup(tf, trunc_page(faultva),
	    PSL_IS);
	if (ptep == NULL)
		dump_trapframe(tf, NULL);
	KASSERT(ptep != NULL);
	pt_entry_t pte = *ptep;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmapexechist);

	if ((pte & PTE_UNSYNCED) == PTE_UNSYNCED) {
		const paddr_t pa = pte_to_paddr(pte);
		struct vm_page * const pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg);
		struct vm_page_md * const mdpg = VM_PAGE_TO_MD(pg);

		UVMHIST_LOG(pmapexechist,
		    "srr0=%#x pg=%p (pa %#"PRIxPADDR"): %s", 
		    tf->tf_srr0, pg, pa, 
		    (VM_PAGEMD_EXECPAGE_P(mdpg)
			? "no syncicache (already execpage)"
			: "performed syncicache (now execpage)"));

		if (!VM_PAGEMD_EXECPAGE_P(mdpg)) {
			ci->ci_softc->cpu_ev_exec_trap_sync.ev_count++;
			dcache_wb_page(pa);
			icache_inv_page(pa);
			pmap_page_set_attributes(mdpg, VM_PAGEMD_EXECPAGE);
		}
		pte &= ~PTE_UNSYNCED;
		pte |= PTE_xX;
		*ptep = pte;

		pmap_tlb_update_addr(faultmap->pmap, trunc_page(faultva),
		    pte, 0);
		kpreempt_enable();
		UVMHIST_LOG(pmapexechist, "<- 0", 0,0,0,0);
		return 0;
	}
	kpreempt_enable();

	int rv = pagefault(faultmap, faultva, VM_PROT_READ|VM_PROT_EXECUTE,
	    usertrap);

	if (__predict_false(rv != 0 && usertrap)) {
		/*
		 * We can't get a MAPERR here since
		 * that's a different exception.
		 */
		ci->ci_ev_isi_fatal.ev_count++;
		KSI_INIT_TRAP(ksi);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_ISI;
		ksi->ksi_code = SEGV_ACCERR;
		ksi->ksi_addr = (void *)tf->tf_srr0; /* not truncated */
	}
	UVMHIST_LOG(pmapexechist, "<- %d", rv, 0,0,0);
	return rv;
}

static int
dtlb_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	const vaddr_t faultva = tf->tf_dear;
	const vm_prot_t ftype = get_faulttype(tf);
	struct vm_map * const faultmap = get_faultmap(tf, PSL_DS);
	struct cpu_info * const ci = curcpu();
	const bool usertrap = usertrap_p(tf);

#if 0
	/*
	 * This is what pte_load in trap_subr.S does for us.
	 */
	const pt_entry_t * const ptep =
	    trap_pte_lookup(tf, trunc_page(faultva), PSL_DS);
	if (ptep != NULL && !usertrap && pte_valid_p(*ptep)) {
		tlb_update_addr(trunc_page(faultva), KERNEL_PID, *ptep, true);
		ci->ci_ev_tlbmiss_soft.ev_count++;
		return 0;
	}
#endif

	ci->ci_ev_dtlbmiss_hard.ev_count++;

//	printf("pagefault(%p,%#lx,%u,%u)", faultmap, faultva, ftype, usertrap);
	int rv = pagefault(faultmap, faultva, ftype, usertrap);
//	printf(": %d\n", rv);

	if (__predict_false(rv != 0 && usertrap)) {
		ci->ci_ev_udsi_fatal.ev_count++;
		KSI_INIT_TRAP(ksi);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_DSI;
		ksi->ksi_code = (rv == EACCES ? SEGV_ACCERR : SEGV_MAPERR);
		ksi->ksi_addr = (void *)faultva;
	}
	return rv;
}

static int
itlb_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct vm_map * const faultmap = get_faultmap(tf, PSL_IS);
	const vaddr_t faultva = tf->tf_srr0;
	struct cpu_info * const ci = curcpu();
	const bool usertrap = usertrap_p(tf);

	ci->ci_ev_itlbmiss_hard.ev_count++;

	int rv = pagefault(faultmap, faultva, VM_PROT_READ|VM_PROT_EXECUTE,
	    usertrap);

	if (__predict_false(rv != 0 && usertrap)) {
		ci->ci_ev_isi_fatal.ev_count++;
		KSI_INIT_TRAP(ksi);
		ksi->ksi_signo = SIGSEGV;
		ksi->ksi_trap = EXC_ISI;
		ksi->ksi_code = (rv == EACCES ? SEGV_ACCERR : SEGV_MAPERR);
		ksi->ksi_addr = (void *)tf->tf_srr0;
	}
	return rv;
}

static int
spe_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct cpu_info * const ci = curcpu();

	if (!usertrap_p(tf))
		return EPERM;

	ci->ci_ev_vec.ev_count++;

#ifdef PPC_HAVE_SPE
	vec_load();
	return 0;
#else
	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = SIGILL;
	ksi->ksi_trap = EXC_PGM;
	ksi->ksi_code = ILL_ILLOPC;
	ksi->ksi_addr = (void *)tf->tf_srr0;
	return EPERM;
#endif
}

static bool
emulate_opcode(struct trapframe *tf, ksiginfo_t *ksi)
{
	uint32_t opcode;
        if (copyin((void *)tf->tf_srr0, &opcode, sizeof(opcode)) != 0)
		return false;

	if (opcode == OPC_LWSYNC)
		return true;

	if (OPC_MFSPR_P(opcode, SPR_PVR)) {
		__asm ("mfpvr %0" : "=r"(tf->tf_fixreg[OPC_MFSPR_REG(opcode)]));
		return true;
	}

	if (OPC_MFSPR_P(opcode, SPR_PIR)) {
		__asm ("mfspr %0, 286" : "=r"(tf->tf_fixreg[OPC_MFSPR_REG(opcode)]));
		return true;
	}

	if (OPC_MFSPR_P(opcode, SPR_SVR)) {
		__asm ("mfspr %0,%1"
		    :	"=r"(tf->tf_fixreg[OPC_MFSPR_REG(opcode)])
		    :	"n"(SPR_SVR));
		return true;
	}

	/*
	 * If we bothered to emulate FP, we would try to do so here.
	 */
	return false;
}

static int
pgm_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct cpu_info * const ci = curcpu();
	int rv = EPERM;

	if (!usertrap_p(tf))
		return rv;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pmapexechist);

	UVMHIST_LOG(pmapexechist, " srr0/1=%#x/%#x esr=%#x pte=%#x", 
	    tf->tf_srr0, tf->tf_srr1, tf->tf_esr,
	    *trap_pte_lookup(tf, trunc_page(tf->tf_srr0), PSL_IS));

	ci->ci_ev_pgm.ev_count++;

	if (tf->tf_esr & ESR_PTR) {
		struct proc *p = curlwp->l_proc;
		if (p->p_raslist != NULL
		    && ras_lookup(p, (void *)tf->tf_srr0) != (void *) -1) {
			tf->tf_srr0 += 4;
			return 0;
		}
	}

	if (tf->tf_esr & (ESR_PIL|ESR_PPR)) {
		if (emulate_opcode(tf, ksi)) {
			tf->tf_srr0 += 4;
			return 0;
		}
	}

	if (tf->tf_esr & ESR_PIL) {
		struct pcb * const pcb = lwp_getpcb(curlwp);
		if (__predict_false(!fpu_used_p(curlwp))) {
			memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
			fpu_mark_used(curlwp);
		}
		if (fpu_emulate(tf, &pcb->pcb_fpu, ksi)) {
			if (ksi->ksi_signo == 0) {
				ci->ci_ev_fpu.ev_count++;
				return 0;
			}
			return EFAULT;
		}
	}

	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = SIGILL;
	ksi->ksi_trap = EXC_PGM;
	if (tf->tf_esr & ESR_PIL) {
		ksi->ksi_code = ILL_ILLOPC;
	} else if (tf->tf_esr & ESR_PPR) {
		ksi->ksi_code = ILL_PRVOPC;
	} else if (tf->tf_esr & ESR_PTR) {
		ksi->ksi_signo = SIGTRAP;
		ksi->ksi_code = TRAP_BRKPT;
	} else {
		ksi->ksi_code = 0;
	}
	ksi->ksi_addr = (void *)tf->tf_srr0;
	return rv;
}

static int
debug_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct cpu_info * const ci = curcpu();
	int rv = EPERM;

	if (!usertrap_p(tf))
		return rv;

	ci->ci_ev_debug.ev_count++;

	/*
	 * Ack the interrupt.
	 */
	mtspr(SPR_DBSR, tf->tf_esr);
	KASSERT(tf->tf_esr & (DBSR_IAC1|DBSR_IAC2));
	KASSERT((tf->tf_srr1 & PSL_SE) == 0);

	/*
	 * Disable debug events
	 */
	mtspr(SPR_DBCR1, 0);
	mtspr(SPR_DBCR0, 0);

	/*
	 * Tell the debugger ...
	 */
	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = SIGTRAP;
	ksi->ksi_trap = EXC_TRC;
	ksi->ksi_addr = (void *)tf->tf_srr0;
	ksi->ksi_code = TRAP_TRACE;
	return rv;
}

static int
ali_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct cpu_info * const ci = curcpu();
	int rv = EFAULT;

	ci->ci_ev_ali.ev_count++;

	if (rv != 0 && usertrap_p(tf)) {
		ci->ci_ev_ali_fatal.ev_count++;
		KSI_INIT_TRAP(ksi);
		ksi->ksi_signo = SIGILL;
		ksi->ksi_trap = EXC_PGM;
		if (tf->tf_esr & ESR_PIL)
			ksi->ksi_code = ILL_ILLOPC;
		else if (tf->tf_esr & ESR_PPR)
			ksi->ksi_code = ILL_PRVOPC;
		else if (tf->tf_esr & ESR_PTR)
			ksi->ksi_code = ILL_ILLTRP;
		else
			ksi->ksi_code = 0;
		ksi->ksi_addr = (void *)tf->tf_srr0;
	}
	return rv;
}

static int
embedded_fp_data_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct cpu_info * const ci = curcpu();
	int rv = EFAULT;

	ci->ci_ev_fpu.ev_count++;

	if (rv != 0 && usertrap_p(tf)) {
		KSI_INIT_TRAP(ksi);
#ifdef PPC_HAVE_SPE
		ksi->ksi_signo = SIGFPE;
		ksi->ksi_trap = tf->tf_exc;
		ksi->ksi_code = vec_siginfo_code(tf);
#else
		ksi->ksi_signo = SIGILL;
		ksi->ksi_trap = EXC_PGM;
		ksi->ksi_code = ILL_ILLOPC;
#endif
		ksi->ksi_addr = (void *)tf->tf_srr0;
	}
	return rv;
}

static int
embedded_fp_round_exception(struct trapframe *tf, ksiginfo_t *ksi)
{
	struct cpu_info * const ci = curcpu();
	int rv = EDOM;

	ci->ci_ev_fpu.ev_count++;

	if (rv != 0 && usertrap_p(tf)) {
		KSI_INIT_TRAP(ksi);
#ifdef PPC_HAVE_SPE
		ksi->ksi_signo = SIGFPE;
		ksi->ksi_trap = tf->tf_exc;
		ksi->ksi_code = vec_siginfo_code(tf);
#else
		ksi->ksi_signo = SIGILL;
		ksi->ksi_trap = EXC_PGM;
		ksi->ksi_code = ILL_ILLOPC;
#endif
		ksi->ksi_addr = (void *)tf->tf_srr0;
	}
	return rv;
}

void
dump_trapframe(const struct trapframe *tf, void (*pr)(const char *, ...))
{
	if (pr == NULL)
		pr = printf;
	(*pr)("trapframe %p (exc=%x srr0/1=%#lx/%#lx esr/dear=%#x/%#lx)\n",
	    tf, tf->tf_exc, tf->tf_srr0, tf->tf_srr1, tf->tf_esr, tf->tf_dear);
	(*pr)("lr =%08lx ctr=%08lx cr =%08x xer=%08x\n",
	    tf->tf_lr, tf->tf_ctr, tf->tf_cr, tf->tf_xer);
	for (u_int r = 0; r < 32; r += 4) {
		(*pr)("r%02u=%08lx r%02u=%08lx r%02u=%08lx r%02u=%08lx\n",
		    r+0, tf->tf_fixreg[r+0], r+1, tf->tf_fixreg[r+1],
		    r+2, tf->tf_fixreg[r+2], r+3, tf->tf_fixreg[r+3]);
	}
}

static bool
ddb_exception(struct trapframe *tf)
{
#if 0
	const register_t ddb_trapfunc = (uintptr_t) cpu_Debugger;
	if ((tf->tf_esr & ESR_PTR) == 0)
		return false;
	if (ddb_trapfunc <= tf->tf_srr0 && tf->tf_srr0 <= ddb_trapfunc+16) {
		register_t srr0 = tf->tf_srr0;
		if (kdb_trap(tf->tf_exc, tf)) {
			if (srr0 == tf->tf_srr0)
				tf->tf_srr0 += 4;
			return true;
		}
	}
	return false;
#else
#if 0
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	printf("CPL stack:");
	if (ci->ci_idepth >= 0) {
		for (u_int i = 0; i <= ci->ci_idepth; i++) {
			printf(" [%u]=%u", i, cpu->cpu_pcpls[i]);
		}
	}
	printf(" %u\n", ci->ci_cpl);
	dump_trapframe(tf, NULL);
#endif
	if (kdb_trap(tf->tf_exc, tf)) {
		tf->tf_srr0 += 4;
		return true;
	}
	return false;
#endif
}

static bool
onfaulted(struct trapframe *tf, register_t rv)
{
	struct lwp * const l = curlwp;
	struct pcb * const pcb = lwp_getpcb(l);
	struct faultbuf * const fb = pcb->pcb_onfault;
	if (fb == NULL)
		return false;
	tf->tf_srr0 = fb->fb_pc;
	tf->tf_srr1 = fb->fb_msr;
	tf->tf_cr = fb->fb_cr;
	tf->tf_fixreg[1] = fb->fb_sp;
	tf->tf_fixreg[2] = fb->fb_r2;
	tf->tf_fixreg[3] = rv;
	pcb->pcb_onfault = NULL;
	return true;
}

void
trap(enum ppc_booke_exceptions trap_code, struct trapframe *tf)
{
	const bool usertrap = usertrap_p(tf);
	struct cpu_info * const ci = curcpu();
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	ksiginfo_t ksi;
	int rv = EACCES;

	ci->ci_ev_traps.ev_count++;
	ci->ci_data.cpu_ntrap++;

	KASSERTMSG(!usertrap || tf == trapframe(l),
	    "trap: tf=%p is invalid: trapframe(%p)=%p", tf, l, trapframe(l));

#if 0
	if (trap_code != T_PROGRAM || usertrap)
		printf("trap(enter): %s (tf=%p, esr/dear=%#x/%#lx, srr0/1=%#lx/%#lx, lr=%#lx)\n",
		    trap_names[trap_code], tf, tf->tf_esr, tf->tf_dear,
		    tf->tf_srr0, tf->tf_srr1, tf->tf_lr);
#endif
#if 0
	if ((register_t)tf >= (register_t)l->l_addr + USPACE
	    || (register_t)tf < (register_t)l->l_addr + PAGE_SIZE) {
		printf("%s(entry): pid %d.%d (%s): invalid tf addr %p\n",
		    __func__, p->p_pid, l->l_lid, p->p_comm, tf);
		dump_trapframe(tf, NULL);
		Debugger();
	}
#endif
#if 0
	if ((mfmsr() & PSL_CE) == 0) {
		printf("%s(entry): pid %d.%d (%s): %s: PSL_CE (%#lx) not set\n",
		    __func__, p->p_pid, l->l_lid, p->p_comm,
		    trap_names[trap_code], mfmsr());
		dump_trapframe(tf, NULL);
	}
#endif

	if ((VM_MAX_ADDRESS & 0x80000000) == 0
	    && usertrap && (tf->tf_fixreg[1] & 0x80000000)) {
		printf("%s(entry): pid %d.%d (%s): %s invalid sp %#lx (sprg1=%#lx)\n",
		    __func__, p->p_pid, l->l_lid, p->p_comm,
		    trap_names[trap_code], tf->tf_fixreg[1], mfspr(SPR_SPRG1));
		dump_trapframe(tf, NULL);
		Debugger();
	}

	if (usertrap && (tf->tf_srr1 & (PSL_DS|PSL_IS)) != (PSL_DS|PSL_IS)) {
		printf("%s(entry): pid %d.%d (%s): %s invalid PSL %#lx\n",
		    __func__, p->p_pid, l->l_lid, p->p_comm,
		    trap_names[trap_code], tf->tf_srr1);
		dump_trapframe(tf, NULL);
		Debugger();
	}

	switch (trap_code) {
	case T_CRITIAL_INPUT:
	case T_EXTERNAL_INPUT:
	case T_DECREMENTER:
	case T_FIXED_INTERVAL:
	case T_WATCHDOG:
	case T_SYSTEM_CALL:
	default:
		panic("trap: unexcepted trap code %d! (tf=%p, srr0/1=%#lx/%#lx)",
		    trap_code, tf, tf->tf_srr0, tf->tf_srr1);
	case T_MACHINE_CHECK:
		rv = mchk_exception(tf, &ksi);
		break;
	case T_DSI:
		rv = dsi_exception(tf, &ksi);
		break;
	case T_ISI:
		rv = isi_exception(tf, &ksi);
		break;
	case T_ALIGNMENT:
		rv = ali_exception(tf, &ksi);
		break;
	case T_SPE_UNAVAILABLE:
		rv = spe_exception(tf, &ksi);
		break;
	case T_PROGRAM:
#ifdef DDB
		if (!usertrap && ddb_exception(tf))
			return;
#endif
		rv = pgm_exception(tf, &ksi);
		break;
	case T_FP_UNAVAILABLE:
	case T_AP_UNAVAILABLE:
		panic("trap: unexcepted trap code %d! (tf=%p, srr0/1=%#lx/%#lx)",
		    trap_code, tf, tf->tf_srr0, tf->tf_srr1);
	case T_DATA_TLB_ERROR:
		rv = dtlb_exception(tf, &ksi);
		break;
	case T_INSTRUCTION_TLB_ERROR:
		rv = itlb_exception(tf, &ksi);
		break;
	case T_DEBUG:
#ifdef DDB
		if (!usertrap && ddb_exception(tf))
			return;
#endif
		rv = debug_exception(tf, &ksi);
		break;
	case T_EMBEDDED_FP_DATA:
		rv = embedded_fp_data_exception(tf, &ksi);
		break;
	case T_EMBEDDED_FP_ROUND:
		rv = embedded_fp_round_exception(tf, &ksi);
		break;
	case T_EMBEDDED_PERF_MONITOR:
		//db_stack_trace_print(tf->tf_fixreg[1], true, 40, "", printf);
		dump_trapframe(tf, NULL);
		rv = EPERM;
		break;
	case T_AST:
		KASSERT(usertrap);
		cpu_ast(l, ci);
		if ((VM_MAX_ADDRESS & 0x80000000) == 0
		   && (tf->tf_fixreg[1] & 0x80000000)) {
			printf("%s(ast-exit): pid %d.%d (%s): invalid sp %#lx\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    tf->tf_fixreg[1]);
			dump_trapframe(tf, NULL);
			Debugger();
		}
		if ((tf->tf_srr1 & (PSL_DS|PSL_IS)) != (PSL_DS|PSL_IS)) {
			printf("%s(entry): pid %d.%d (%s): %s invalid PSL %#lx\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    trap_names[trap_code], tf->tf_srr1);
			dump_trapframe(tf, NULL);
			Debugger();
		}
#if 0
		if ((mfmsr() & PSL_CE) == 0) {
			printf("%s(exit): pid %d.%d (%s): %s: PSL_CE (%#lx) not set\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    trap_names[trap_code], mfmsr());
			dump_trapframe(tf, NULL);
		}
#endif
		userret(l, tf);
		return;
	}
	if (!usertrap) {
		if (rv != 0) {
			if (!onfaulted(tf, rv)) {
				db_stack_trace_print(tf->tf_fixreg[1], true, 40, "", printf);
				dump_trapframe(tf, NULL);
				panic("%s: pid %d.%d (%s): %s exception in kernel mode"
				    " (tf=%p, dear=%#lx, esr=%#x,"
				    " srr0/1=%#lx/%#lx)",
				    __func__, p->p_pid, l->l_lid, p->p_comm,
				    trap_names[trap_code], tf, tf->tf_dear,
				    tf->tf_esr, tf->tf_srr0, tf->tf_srr1);
			}
		}
#if 0
		if (tf->tf_fixreg[1] >= (register_t)l->l_addr + USPACE
		    || tf->tf_fixreg[1] < (register_t)l->l_addr + PAGE_SIZE) {
			printf("%s(exit): pid %d.%d (%s): invalid kern sp %#lx\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    tf->tf_fixreg[1]);
			dump_trapframe(tf, NULL);
			Debugger();
		}
#endif
#if 0
		if ((mfmsr() & PSL_CE) == 0) {
			printf("%s(exit): pid %d.%d (%s): %s: PSL_CE (%#lx) not set\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    trap_names[trap_code], mfmsr());
			mtmsr(mfmsr()|PSL_CE);
			dump_trapframe(tf, NULL);
		}
#endif
	} else {
		if (rv == ENOMEM) {
			printf("UVM: pid %d.%d (%s), uid %d killed: "
			    "out of swap\n",
			    p->p_pid, l->l_lid, p->p_comm,
			    l->l_cred ?  kauth_cred_geteuid(l->l_cred) : -1);
			ksi.ksi_signo = SIGKILL;
		}
		if (rv != 0) {
			/*
			 * Only print a fatal trap if the signal will be
			 * uncaught.
			 */
			if (cpu_printfataltraps
			    && (p->p_slflag & PSL_TRACED) == 0
			    && !sigismember(&p->p_sigctx.ps_sigcatch,
				    ksi.ksi_signo)) {
				printf("%s: pid %d.%d (%s):"
				    " %s exception in user mode\n",
				    __func__, p->p_pid, l->l_lid, p->p_comm,
				    trap_names[trap_code]);
				if (cpu_printfataltraps > 1)
					dump_trapframe(tf, NULL);
			}
			(*p->p_emul->e_trapsignal)(l, &ksi);
		}
#ifdef DEBUG
		if ((tf->tf_srr1 & (PSL_DS|PSL_IS)) != (PSL_DS|PSL_IS)) {
			printf("%s(exit): pid %d.%d (%s): %s invalid PSL %#lx\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    trap_names[trap_code], tf->tf_srr1);
			dump_trapframe(tf, NULL);
			Debugger();
		}
#endif
#if 0
		if ((mfmsr() & PSL_CE) == 0) {
			printf("%s(exit): pid %d.%d (%s): %s: PSL_CE (%#lx) not set\n",
			    __func__, p->p_pid, l->l_lid, p->p_comm,
			    trap_names[trap_code], mfmsr());
			dump_trapframe(tf, NULL);
		}
#endif
		userret(l, tf);
	}
}
