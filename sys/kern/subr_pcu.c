/*	$NetBSD: subr_pcu.c,v 1.3 2011/02/19 20:19:54 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: subr_pcu.c,v 1.3 2011/02/19 20:19:54 matt Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/lwp.h>
#include <sys/pcu.h>
#include <sys/xcall.h>

#if PCU_UNIT_COUNT > 0

#define	PCU_SAVE		0x01	/* Save PCU state to the LWP. */
#define	PCU_RELEASE		0x02	/* Release PCU state on the CPU. */

#if 0
/*
 * pcu_init_lwp: initialize PCU structures for LWP.
 */
void
pcu_init_lwp(lwp_t *l)
{

	memset(l->l_pcu_cpu, 0, sizeof(uint32_t) * PCU_UNIT_COUNT);
	l->l_pcu_used = 0;
}
#endif

/*
 * pcu_cpu_op: save/release PCU state on the current CPU.
 *
 * => Must be called at IPL_SOFTCLOCK or from the soft-interrupt.
 */
static void
pcu_cpu_op(const pcu_ops_t *pcu, const int flags)
{
	const u_int id = pcu->pcu_id;
	struct cpu_info *ci = curcpu();
	lwp_t *l = ci->ci_pcu_curlwp[id];

	/* If no state - nothing to do. */
	if (l == NULL) {
		return;
	}
	if (flags & PCU_SAVE) {
		pcu->pcu_state_save(l, (flags & PCU_RELEASE) != 0);
	}
	if (flags & PCU_RELEASE) {
		ci->ci_pcu_curlwp[id] = NULL;
		l->l_pcu_cpu[id] = NULL;
	}
}

/*
 * pcu_lwp_op: perform PCU state save, release or both operations on LWP.
 */
static void
pcu_lwp_op(const pcu_ops_t *pcu, lwp_t *l, int flags)
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
		KASSERT(ci->ci_pcu_curlwp[id] == l);

		if (flags & PCU_SAVE) {
			pcu->pcu_state_save(l, (flags & PCU_RELEASE) != 0);
		}
		if (flags & PCU_RELEASE) {
			ci->ci_pcu_curlwp[id] = NULL;
			l->l_pcu_cpu[id] = NULL;
		}
		splx(s);
		return;
	}
	splx(s);

	if (__predict_false(ci == NULL)) {
		/* Cross-call has won the race - no state to manage. */
		return;
	}

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
	lwp_t *l = curlwp;
	uint64_t where;
	int s;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	s = splsoftclock();
	curci = curcpu();
	ci = l->l_pcu_cpu[id];

	/* Does this CPU already have our PCU state loaded? */
	if (ci == curci) {
		KASSERT(curci->ci_pcu_curlwp[id] == l);
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
	pcu->pcu_state_load(l, ((1 << id) & l->l_pcu_used) != 0);
	curci->ci_pcu_curlwp[id] = l;
	l->l_pcu_cpu[id] = curci;
	l->l_pcu_used |= (1 << id);
	splx(s);
}

/*
 * pcu_discard: discard the PCU state of current LWP.
 */
void
pcu_discard(const pcu_ops_t *pcu)
{
	const u_int id = pcu->pcu_id;
	lwp_t *l = curlwp;

	KASSERT(!cpu_intr_p() && !cpu_softintr_p());

	if (__predict_true(l->l_pcu_cpu[id] == NULL)) {
		return;
	}
	pcu_lwp_op(pcu, l, PCU_RELEASE);
	l->l_pcu_used &= ~(1 << id);
}

/*
 * pcu_save_lwp: save PCU state to the given LWP.
 */
void
pcu_save_lwp(const pcu_ops_t *pcu, lwp_t *l)
{
	const u_int id = pcu->pcu_id;

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
pcu_used(const pcu_ops_t *pcu, lwp_t *l)
{
	const u_int id = pcu->pcu_id;

	return l->l_pcu_used & (1 << id);
}

#endif /* PCU_UNIT_COUNT > 0 */
