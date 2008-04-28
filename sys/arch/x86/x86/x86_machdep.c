/*	$NetBSD: x86_machdep.c,v 1.17 2008/04/28 22:47:37 ad Exp $	*/

/*-
 * Copyright (c) 2005, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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
__KERNEL_RCSID(0, "$NetBSD: x86_machdep.c,v 1.17 2008/04/28 22:47:37 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/atomic.h>

#include <x86/cpu_msr.h>

#include <machine/bootinfo.h>
#include <machine/vmparam.h>

#include <uvm/uvm_extern.h>

int check_pa_acc(paddr_t, vm_prot_t);

/* --------------------------------------------------------------------- */

/*
 * Main bootinfo structure.  This is filled in by the bootstrap process
 * done in locore.S based on the information passed by the boot loader.
 */
struct bootinfo bootinfo;

/* --------------------------------------------------------------------- */

/*
 * Given the type of a bootinfo entry, looks for a matching item inside
 * the bootinfo structure.  If found, returns a pointer to it (which must
 * then be casted to the appropriate bootinfo_* type); otherwise, returns
 * NULL.
 */
void *
lookup_bootinfo(int type)
{
	bool found;
	int i;
	struct btinfo_common *bic;

	bic = (struct btinfo_common *)(bootinfo.bi_data);
	found = FALSE;
	for (i = 0; i < bootinfo.bi_nentries && !found; i++) {
		if (bic->type == type)
			found = TRUE;
		else
			bic = (struct btinfo_common *)
			    ((uint8_t *)bic + bic->len);
	}

	return found ? bic : NULL;
}

/*
 * check_pa_acc: check if given pa is accessible.
 */
int
check_pa_acc(paddr_t pa, vm_prot_t prot)
{
	extern phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
	extern int mem_cluster_cnt;
	int i;

	for (i = 0; i < mem_cluster_cnt; i++) {
		const phys_ram_seg_t *seg = &mem_clusters[i];
		paddr_t lstart = seg->start;

		if (lstart <= pa && pa - lstart <= seg->size) {
			return 0;
		}
	}

	return kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL);
}

/*
 * This function is to initialize the mutex used by x86/msr_ipifuncs.c.
 */
void
x86_init(void)
{
#ifndef XEN
	msr_cpu_broadcast_initmtx();
#endif
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	struct cpu_info *cur;
	lwp_t *l;

	cur = curcpu();
	l = ci->ci_data.cpu_onproc;
	ci->ci_want_resched = 1;

	if (__predict_false((l->l_pflag & LP_INTR) != 0)) {
		/*
		 * No point doing anything, it will switch soon.
		 * Also here to prevent an assertion failure in
		 * kpreempt() due to preemption being set on a
		 * soft interrupt LWP.
		 */
		return;
	}

	if (l == ci->ci_data.cpu_idlelwp) {
		if (ci == cur)
			return;
		if ((ci->ci_feature2_flags & CPUID2_MONITOR) == 0) {
			x86_send_ipi(ci, 0);
		}
		return;
	}

	if ((flags & RESCHED_KPREEMPT) != 0) {
#ifdef __HAVE_PREEMPTION
		atomic_or_uint(&l->l_dopreempt, DOPREEMPT_ACTIVE);
		if (ci == cur) {
			softint_trigger(1 << SIR_PREEMPT);
		} else {
			x86_send_ipi(ci, X86_IPI_KPREEMPT);
		}
#endif
	} else {
		aston(l);
		if (ci == cur) {
			return;
		}
		if ((flags & RESCHED_IMMED) != 0) {
			x86_send_ipi(ci, 0);
		}
	}
}

void
cpu_signotify(struct lwp *l)
{

	aston(l);
	if (l->l_cpu != curcpu())
		x86_send_ipi(l->l_cpu, 0);
}

void
cpu_need_proftick(struct lwp *l)
{

	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	aston(l);
}

bool
cpu_intr_p(void)
{

	return (curcpu()->ci_idepth >= 0);
}

#ifdef __HAVE_PREEMPTION
/*
 * Called to check MD conditions that would prevent preemption, and to
 * arrange for those conditions to be rechecked later.
 */
bool
cpu_kpreempt_enter(uintptr_t where, int s)
{
	struct cpu_info *ci;
	lwp_t *l;

	KASSERT(kpreempt_disabled());

	l = curlwp;
	ci = curcpu();

	/*
	 * If SPL raised, can't go.  Note this implies that spin
	 * mutexes at IPL_NONE are _not_ valid to use.
	 */
	if (s > IPL_PREEMPT) {
		softint_trigger(1 << SIR_PREEMPT);
		aston(l);	/* paranoid */
		return false;
	}

	/* Must save cr2 or it could be clobbered. */
	((struct pcb *)l->l_addr)->pcb_cr2 = rcr2();

	return true;
}

/*
 * Called after returning from a kernel preemption, and called with
 * preemption disabled.
 */
void
cpu_kpreempt_exit(uintptr_t where)
{
	extern char x86_copyfunc_start, x86_copyfunc_end;

	KASSERT(kpreempt_disabled());

	/*
	 * If we interrupted any of the copy functions we must reload
	 * the pmap when resuming, as they cannot tolerate it being
	 * swapped out.
	 */
	if (where >= (uintptr_t)&x86_copyfunc_start &&
	    where < (uintptr_t)&x86_copyfunc_end) {
		pmap_load();
	}

	/* Restore cr2 only after the pmap, as pmap_load can block. */
	lcr2(((struct pcb *)curlwp->l_addr)->pcb_cr2);
}

/*
 * Return true if preemption is disabled for MD reasons.  Must be called
 * with preemption disabled, and thus is only for diagnostic checks.
 */
bool
cpu_kpreempt_disabled(void)
{

	return curcpu()->ci_ilevel > IPL_NONE;
}
#endif	/* __HAVE_PREEMPTION */

