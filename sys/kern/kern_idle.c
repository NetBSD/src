/*	$NetBSD: kern_idle.c,v 1.17 2008/05/24 12:59:06 ad Exp $	*/

/*-
 * Copyright (c)2002, 2006, 2007 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: kern_idle.c,v 1.17 2008/05/24 12:59:06 ad Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/idle.h>
#include <sys/kthread.h>
#include <sys/lockdebug.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#if MAXCPUS > 32
#error fix this code
#endif

static volatile uint32_t *idle_cpus;

void
idle_loop(void *dummy)
{
	struct cpu_info *ci = curcpu();
	struct lwp *l = curlwp;
	uint32_t mask = 1 << cpu_index(ci);
	bool set = false;
	int s;

	ci->ci_data.cpu_onproc = l;

	/* Update start time for this thread. */
	lwp_lock(l);
	binuptime(&l->l_stime);
	lwp_unlock(l);

	s = splsched();
	ci->ci_schedstate.spc_flags |= SPCF_RUNNING;
	splx(s);

	KERNEL_UNLOCK_ALL(l, NULL);
	l->l_stat = LSONPROC;
	while (1 /* CONSTCOND */) {
		LOCKDEBUG_BARRIER(NULL, 0);
		KASSERT((l->l_flag & LW_IDLE) != 0);
		KASSERT(ci == curcpu());
		KASSERT(l == curlwp);
		KASSERT(CURCPU_IDLE_P());
		KASSERT(l->l_priority == PRI_IDLE);

		if (uvm.page_idle_zero) {
			if (sched_curcpu_runnable_p()) {
				goto schedule;
			}
			if (!set) {
				set = true;
				atomic_or_32(idle_cpus, mask);
			}
			uvm_pageidlezero();
		}
		if (!sched_curcpu_runnable_p()) {
			if (!set) {
				set = true;
				atomic_or_32(idle_cpus, mask);
			}
			cpu_idle();
			if (!sched_curcpu_runnable_p() &&
			    !ci->ci_want_resched) {
				continue;
			}
		}
schedule:
		if (set) {
			set = false;
			atomic_and_32(idle_cpus, ~mask);
		}
		KASSERT(l->l_mutex == l->l_cpu->ci_schedstate.spc_lwplock);
		lwp_lock(l);
		mi_switch(l);
		KASSERT(curlwp == l);
		KASSERT(l->l_stat == LSONPROC);
	}
}

/*
 * Find an idle CPU and remove from the idle bitmask.  The bitmask
 * is not always accurate but is "good enough" for the purpose of finding
 * a CPU to run a job on.
 */
struct cpu_info *
idle_pick(void)
{
	uint32_t mask;
	u_int index;

	do {
		if ((mask = *idle_cpus) == 0)
			return NULL;
		index = ffs(mask) - 1;
	} while (atomic_cas_32(idle_cpus, mask, mask ^ (1 << index)) != mask);

	return cpu_lookup_byindex(index);
}

int
create_idle_lwp(struct cpu_info *ci)
{
	static bool again;
	uintptr_t addr;
	lwp_t *l;
	int error;

	if (!again) {
		again = true;
		addr = (uintptr_t)kmem_alloc(coherency_unit * 2, KM_SLEEP);
		addr = roundup(addr, coherency_unit);
		idle_cpus = (uint32_t *)addr;
	}

	KASSERT(ci->ci_data.cpu_idlelwp == NULL);
	error = kthread_create(PRI_IDLE, KTHREAD_MPSAFE | KTHREAD_IDLE,
	    ci, idle_loop, NULL, &l, "idle/%u", ci->ci_index);
	if (error != 0)
		panic("create_idle_lwp: error %d", error);
	lwp_lock(l);
	l->l_flag |= LW_IDLE;
	lwp_unlock(l);
	l->l_cpu = ci;
	ci->ci_data.cpu_idlelwp = l;

	return error;
}
