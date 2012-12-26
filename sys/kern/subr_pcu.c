/*	$NetBSD: subr_pcu.c,v 1.13 2012/12/26 18:30:23 matt Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 * Per CPU Unit (PCU) - is an interface to manage synchronization of any
 * per CPU context (unit) tied with LWP context.  Typical use: FPU state.
 *
 * Concurrency notes:
 *
 *	PCU state may be loaded only by the current LWP, that is, curlwp.
 *	Therefore, only LWP itself can set a CPU for lwp_t::l_pcu_cpu[id].
 *
 *	Request for a PCU release can be from owner LWP (whether PCU state
 *	is on current CPU or remote CPU) or any other LWP running on that
 *	CPU (in such case, owner LWP is on a remote CPU or sleeping).
 *
 *	In any case, PCU state can only be changed from the running CPU.
 *	If said PCU state is on the remote CPU, a cross-call will be sent
 *	by the owner LWP.  Therefore struct cpu_info::ci_pcu_curlwp[id]
 *	may only be changed by current CPU, and lwp_t::l_pcu_cpu[id] may
 *	only be unset by the CPU which has PCU state loaded.
 *
 *	There is a race condition: LWP may have a PCU state on a remote CPU,
 *	which it requests to be released via cross-call.  At the same time,
 *	other LWP on remote CPU might release existing PCU state and load
 *	its own one.  Cross-call may arrive after this and release different
 *	PCU state than intended.  In such case, such LWP would re-load its
 *	PCU state again.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_pcu.c,v 1.13 2012/12/26 18:30:23 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/lwp.h>
#include <sys/pcu.h>
#include <sys/xcall.h>

#if PCU_UNIT_COUNT > 0

static inline void pcu_do_op(const pcu_ops_t *, lwp_t * const, const int);
static void pcu_cpu_op(const pcu_ops_t *, const int);
static void pcu_lwp_op(const pcu_ops_t *, lwp_t *, const int);

__CTASSERT(PCU_KERNEL == 1);

#define	PCU_SAVE	(PCU_LOADED << 1) /* Save PCU state to the LWP. */
#define	PCU_RELEASE	(PCU_SAVE << 1)	/* Release PCU state on the CPU. */
#define	PCU_CLAIM	(PCU_RELEASE << 1)	/* CLAIM a PCU for a LWP. */

/* XXX */
extern const pcu_ops_t * const	pcu_ops_md_defs[];

/*
 * pcu_switchpoint: release PCU state if the LWP is being run on another CPU.
 *
 * On each context switches, called by mi_switch() with IPL_SCHED.
 * 'l' is an LWP which is just we switched to.  (the new curlwp)
 */

void
pcu_switchpoint(lwp_t *l)
{
	const uint32_t pcu_kernel_inuse = l->l_pcu_used[PCU_KERNEL];
	uint32_t pcu_user_inuse = l->l_pcu_used[PCU_USER];
	/* int s; */

	KASSERTMSG(l == curlwp, "l %p != curlwp %p", l, curlwp);

	if (__predict_false(pcu_kernel_inuse != 0)) {
		for (u_int id = 0; id < PCU_UNIT_COUNT; id++) {
			if ((pcu_kernel_inuse & (1 << id)) == 0) {
				continue;
			}
			struct cpu_info * const pcu_ci = l->l_pcu_cpu[id];
			if (pcu_ci == NULL || pcu_ci == l->l_cpu) {
				continue;
			}
			const pcu_ops_t * const pcu = pcu_ops_md_defs[id];
			/*
			 * Steal the PCU away from the current owner and
			 * take ownership of it.
			 */
			pcu_cpu_op(pcu, PCU_SAVE | PCU_RELEASE);
			pcu_do_op(pcu, l, PCU_KERNEL | PCU_CLAIM | PCU_RELOAD);
			pcu_user_inuse &= ~(1 << id);
		}
	}

	if (__predict_true(pcu_user_inuse == 0)) {
		/* PCUs are not in use. */
		return;
	}
	/* commented out as we know we are already at IPL_SCHED */
	/* s = splsoftclock(); */
	for (u_int id = 0; id < PCU_UNIT_COUNT; id++) {
		if ((pcu_user_inuse & (1 << id)) == 0) {
			continue;
		}
		struct cpu_info * const pcu_ci = l->l_pcu_cpu[id];
		if (pcu_ci == NULL || pcu_ci == l->l_cpu) {
			continue;
		}
		const pcu_ops_t * const pcu = pcu_ops_md_defs[id];
		pcu->pcu_state_release(l, 0);
	}
	/* splx(s); */
}

/*
 * pcu_discard_all: discard PCU state of the given LWP.
 *
 * Used by exec and LWP exit.
 */

void
pcu_discard_all(lwp_t *l)
{
	const uint32_t pcu_inuse = l->l_pcu_used[PCU_USER];

	KASSERT(l == curlwp || ((l->l_flag & LW_SYSTEM) && pcu_inuse == 0));
	KASSERT(l->l_pcu_used[PCU_KERNEL] == 0);

	if (__predict_true(pcu_inuse == 0)) {
		/* PCUs are not in use. */
		return;
	}
	const int s = splsoftclock();
	for (u_int id = 0; id < PCU_UNIT_COUNT; id++) {
		if ((pcu_inuse & (1 << id)) == 0) {
			continue;
		}
		if (__predict_true(l->l_pcu_cpu[id] == NULL)) {
			continue;
		}
		const pcu_ops_t * const pcu = pcu_ops_md_defs[id];
		/*
		 * We aren't releasing since this LWP isn't giving up PCU,
		 * just saving it.
		 */
		pcu_lwp_op(pcu, l, PCU_RELEASE);
	}
	l->l_pcu_used[PCU_USER] = 0;
	splx(s);
}

/*
 * pcu_save_all: save PCU state of the given LWP so that eg. coredump can
 * examine it.
 */

void
pcu_save_all(lwp_t *l)
{
	const uint32_t pcu_inuse = l->l_pcu_used[PCU_USER];
	/*
	 * Unless LW_WCORE, we aren't releasing since this LWP isn't giving
	 * up PCU, just saving it.
	 */
	const int flags = PCU_SAVE | (l->l_flag & LW_WCORE ? PCU_RELEASE : 0);

	/*
	 * Normally we save for the current LWP, but sometimes we get called
	 * with a different LWP (forking a system LWP or doing a coredump of
	 * a process with multiple threads) and we need to deal with that.
	 */
	KASSERT(l == curlwp
	    || (((l->l_flag & LW_SYSTEM)
		 || (curlwp->l_proc == l->l_proc && l->l_stat == LSSUSPENDED))
	        && pcu_inuse == 0));
	KASSERT(l->l_pcu_used[PCU_KERNEL] == 0);

	if (__predict_true(pcu_inuse == 0)) {
		/* PCUs are not in use. */
		return;
	}
	const int s = splsoftclock();
	for (u_int id = 0; id < PCU_UNIT_COUNT; id++) {
		if ((pcu_inuse & (1 << id)) == 0) {
			continue;
		}
		if (__predict_true(l->l_pcu_cpu[id] == NULL)) {
			continue;
		}
		const pcu_ops_t * const pcu = pcu_ops_md_defs[id];
		pcu_lwp_op(pcu, l, flags);
	}
	splx(s);
}

/*
 * pcu_do_op: save/release PCU state on the current CPU.
 *
 * => Must be called at IPL_SOFTCLOCK or from the soft-interrupt.
 */
static inline void
pcu_do_op(const pcu_ops_t *pcu, lwp_t * const l, const int flags)
{
	struct cpu_info * const ci = curcpu();
	const u_int id = pcu->pcu_id;
	u_int state_flags = flags & (PCU_KERNEL|PCU_RELOAD|PCU_ENABLE);
	uint32_t id_mask = 1 << id;
	const bool kernel_p = (l->l_pcu_used[PCU_KERNEL] & id_mask) != 0;

	KASSERT(l->l_pcu_cpu[id] == (flags & PCU_CLAIM ? NULL : ci));

	if (flags & PCU_SAVE) {
		pcu->pcu_state_save(l, (kernel_p ? PCU_KERNEL : 0));
	}
	if (flags & PCU_RELEASE) {
		pcu->pcu_state_release(l, state_flags);
		if (flags & PCU_KERNEL) {
			l->l_pcu_used[PCU_KERNEL] &= ~id_mask;
		}
		ci->ci_pcu_curlwp[id] = NULL;
		l->l_pcu_cpu[id] = NULL;
	}
	if (flags & PCU_CLAIM) {
		if (l->l_pcu_used[(flags & PCU_KERNEL)] & id_mask)
			state_flags |= PCU_LOADED;
		pcu->pcu_state_load(l, state_flags);
		l->l_pcu_cpu[id] = ci;
		ci->ci_pcu_curlwp[id] = l;
		l->l_pcu_used[flags & PCU_KERNEL] |= id_mask;
	}
	if (flags == PCU_KERNEL) {
		KASSERT(ci->ci_pcu_curlwp[id] == l);
		pcu->pcu_state_save(l, 0);
		l->l_pcu_used[PCU_KERNEL] |= id_mask;
	}
}

/*
 * pcu_cpu_op: helper routine to call pcu_do_op() via xcall(9) or
 * by pcu_load.
 */
static void
pcu_cpu_op(const pcu_ops_t *pcu, const int flags)
{
	const u_int id = pcu->pcu_id;
	lwp_t * const l = curcpu()->ci_pcu_curlwp[id];

	//KASSERT(cpu_softintr_p());

	/* If no state - nothing to do. */
	if (l == NULL) {
		return;
	}
	pcu_do_op(pcu, l, flags);
}

/*
 * pcu_lwp_op: perform PCU state save, release or both operations on LWP.
 */
static void
pcu_lwp_op(const pcu_ops_t *pcu, lwp_t *l, const int flags)
{
	const u_int id = pcu->pcu_id;
	struct cpu_info *ci;
	uint64_t where;
	int s;

	/*
	 * Caller should have re-checked if there is any state to manage.
	 * Block the interrupts and inspect again, since cross-call sent
	 * by remote CPU could have changed the state.
	 */
	s = splsoftclock();
	ci = l->l_pcu_cpu[id];
	if (ci == curcpu()) {
		/*
		 * State is on the current CPU - just perform the operations.
		 */
		KASSERT((flags & PCU_CLAIM) == 0);
		KASSERTMSG(ci->ci_pcu_curlwp[id] == l,
		    "%s: cpu%u: pcu_curlwp[%u] (%p) != l (%p)",
		     __func__, cpu_index(ci), id, ci->ci_pcu_curlwp[id], l);
		pcu_do_op(pcu, l, flags);
		splx(s);
		return;
	}

	if (__predict_false(ci == NULL)) {
		if (flags & PCU_CLAIM) {
			pcu_do_op(pcu, l, flags);
		}
		/* Cross-call has won the race - no state to manage. */
		splx(s);
		return;
	}

	splx(s);

	/*
	 * State is on the remote CPU - perform the operations there.
	 * Note: there is a race condition; see description in the top.
	 */
	where = xc_unicast(XC_HIGHPRI, (xcfunc_t)pcu_cpu_op,
	    __UNCONST(pcu), (void *)(uintptr_t)flags, ci);
	xc_wait(where);

	KASSERT((flags & PCU_RELEASE) == 0 || l->l_pcu_cpu[id] == NULL);
}

/*
 * pcu_load: load/initialize the PCU state of current LWP on current CPU.
 */
void
pcu_load(const pcu_ops_t *pcu)
{
	const u_int id = pcu->pcu_id;
	struct cpu_info *ci, *curci;
	lwp_t * const l = curlwp;
	uint64_t where;
	int s;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	s = splsoftclock();
	curci = curcpu();
	ci = l->l_pcu_cpu[id];

	/* Does this CPU already have our PCU state loaded? */
	if (ci == curci) {
		KASSERT(curci->ci_pcu_curlwp[id] == l);
		pcu->pcu_state_load(l, PCU_ENABLE);	/* Re-enable */
		splx(s);
		return;
	}

	/* If PCU state of this LWP is on the remote CPU - save it there. */
	if (ci) {
		splx(s);
		/* Note: there is a race; see description in the top. */
		where = xc_unicast(XC_HIGHPRI, (xcfunc_t)pcu_cpu_op,
		    __UNCONST(pcu), (void *)(PCU_SAVE | PCU_RELEASE), ci);
		xc_wait(where);

		/* Enter IPL_SOFTCLOCK and re-fetch the current CPU. */
		s = splsoftclock();
		curci = curcpu();
	}
	KASSERT(l->l_pcu_cpu[id] == NULL);

	/* Save the PCU state on the current CPU, if there is any. */
	pcu_cpu_op(pcu, PCU_SAVE | PCU_RELEASE);
	KASSERT(curci->ci_pcu_curlwp[id] == NULL);

	/*
	 * Finally, load the state for this LWP on this CPU.  Indicate to
	 * load function whether PCU was used before.  Note the usage.
	 */
	pcu_do_op(pcu, l, PCU_CLAIM | PCU_ENABLE | PCU_RELOAD);
	splx(s);
}

/*
 * pcu_discard: discard the PCU state of current LWP.
 */
void
pcu_discard(const pcu_ops_t *pcu)
{
	const u_int id = pcu->pcu_id;
	lwp_t * const l = curlwp;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (__predict_true(l->l_pcu_cpu[id] == NULL)) {
		return;
	}
	pcu_lwp_op(pcu, l, PCU_RELEASE);
	l->l_pcu_used[PCU_USER] &= ~(1 << id);
}

/*
 * pcu_save_lwp: save PCU state to the given LWP.
 */
void
pcu_save(const pcu_ops_t *pcu)
{
	const u_int id = pcu->pcu_id;
	lwp_t * const l = curlwp;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (__predict_true(l->l_pcu_cpu[id] == NULL)) {
		return;
	}
	pcu_lwp_op(pcu, l, PCU_SAVE | PCU_RELEASE);
}

/*
 * pcu_used: return true if PCU was used (pcu_load() case) by the LWP.
 */
bool
pcu_used_p(const pcu_ops_t *pcu)
{
	const u_int id = pcu->pcu_id;
	lwp_t * const l = curlwp;

	return l->l_pcu_used[0] & (1 << id);
}

void
pcu_kernel_acquire(const pcu_ops_t *pcu)
{
	struct cpu_info * const ci = curcpu();
	lwp_t * const l = curlwp;
	const u_int id = pcu->pcu_id;

	/*
	 * If we own the PCU, save our user state.
	 */
	if (ci == l->l_pcu_cpu[id]) {
		pcu_lwp_op(pcu, l, PCU_KERNEL);
		return;
	}
	if (ci->ci_data.cpu_pcu_curlwp[id] != NULL) {
		/*
		 * The PCU is owned by another LWP so save its state.
		 */
		pcu_cpu_op(pcu, PCU_SAVE | PCU_RELEASE);
	}
	/*
	 * Mark the PCU as hijacked and take ownership of it.
	 */
	printf("!");
	pcu_lwp_op(pcu, l, PCU_KERNEL | PCU_CLAIM | PCU_ENABLE | PCU_RELOAD);
}

void
pcu_kernel_release(const pcu_ops_t *pcu)
{
	lwp_t * const l = curlwp;

	KASSERT(l->l_pcu_used[PCU_KERNEL] & (1 << pcu->pcu_id));

	/*
	 * Release the PCU, if the curlwp wants to use it, it will have incur
	 * a trap to reenable it.
	 */
	pcu_lwp_op(pcu, l, PCU_KERNEL | PCU_RELEASE);
}

#endif /* PCU_UNIT_COUNT > 0 */
